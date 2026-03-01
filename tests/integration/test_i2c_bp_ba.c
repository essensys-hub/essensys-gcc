/*
 * Integration test: I2C protocol between BP (master) and BA (slave)
 *
 * Simulates the I2C ISR state machine in slavenode.c by setting
 * PIC SSP registers and calling SSP_Handler() for each byte.
 *
 * Frame format (BP → BA):
 *   [slave_addr] [cmd_code] [data...] [CRC-16 low] [CRC-16 high]
 *   CRC computed over cmd_code + data bytes (MODBUS CRC-16)
 *
 * Response format (BA → BP):
 *   [cmd_echo] [CRC_rx_low] [CRC_rx_high] [CRC_resp_low] [CRC_resp_high]
 */
#include "unity.h"
#include <string.h>
#include <stdint.h>

/* ---- PIC register definitions (shared with slavenode.c via pic.h) ---- */
uint8_t PORTA, PORTB, PORTC, PORTD, PORTE, PORTF, PORTG;
uint8_t TRISA, TRISB, TRISC, TRISD, TRISE, TRISF, TRISG;
uint8_t LATA, LATB, LATC, LATD, LATE, LATF, LATG;
uint8_t TMR0, TMR1L, TMR1H, TMR2;
uint8_t INTCON, PIR1, PIE1;
uint8_t SSPBUF, SSPCON, SSPSTAT, SSPADD;
uint8_t PR2, CCP1CON, CCPR1L;
uint8_t ADRESH, ADRESL, ADCON0, ADCON1;
uint8_t CCP2CON, CCPR2L;
uint8_t ANSEL, LCDCON, OSCCON, OPTION_REG, LVDCON;
uint8_t T2CON, T0IF, T0IE, TMR2IF, TMR2IE;
uint8_t SSPIF, SSPIE, GIE, PEIE, IRVST, LVDIF, LVDIE;
uint8_t SSPOV, WCOL, CKP;

/* ---- BA types and constants ---- */
#include "constantes.h"
#include "struct.h"

/* ---- Global variable definitions (match global.h extern declarations) ---- */
unsigned char uc_CompteurCycle;
unsigned char uc_CompteurAcq;
unsigned char uc_CompteurBase1S;
unsigned char b1_BT_1S;
unsigned char uc_CompteurLED;
unsigned char uc_CompteurBiStable;
unsigned char uc_FlagITTimer;

unsigned char uc_NB_ES;
unsigned char uc_ENTREE_TOR_NB;
unsigned char uc_RM_NB;
unsigned char uc_ENTREE_VARIATEUR_NB;
unsigned char uc_VARIATEUR_NB;
unsigned char uc_ENTREE_VOLET_NB;
unsigned char uc_RB_NB;
unsigned char uc_VOLET_NB;

struct struct_Entree st_Entrees[NB_ES];
unsigned short us_EntreesTOR;
struct struct_EntreeVariateur st_EntreesVariateur[ENTREE_VARIATEUR_NB];
struct struct_EntreeVoletOuStore st_EntreesVoletRoulant[ENTREE_VOLET_NB];

unsigned short us_SortiesRelais;
struct struct_SortieRelaisBistable st_SortiesRelaisBistables[RB_NB];
struct struct_Variateur st_Variateurs[VARIATEUR_NB];
unsigned char uc_CdeRelaisBistables[RB_NB];
unsigned short us_CdeVariateurs[VARIATEUR_NB];

unsigned char s_uc_PortA, s_uc_PortB, s_uc_PortC;
unsigned char s_uc_PortD, s_uc_PortE, s_uc_PortF;

union struct_Cde_BP st_Commandes;
unsigned char uc_compteur_ecriture_eeprom;
unsigned char uc_compteur_lecture_eeprom;

/* ---- Stubs for extern functions referenced by slavenode.c ---- */
void vd_Sauve_Etat(void) {}

/* ---- Extern access to slavenode.c / crc.c functions ---- */
extern void SSP_Handler(void);
extern void vd_I2C_Setup(void);
extern unsigned short us_CalculerCRCSurTrame(unsigned char[], unsigned char);

/* Extern access to slavenode.c internal state for verification */
extern union TXBUF {
    struct {
        unsigned code:8;
        unsigned chkfail:1;
        unsigned rxerror:1;
        unsigned ovflw:1;
        unsigned sspov:1;
        unsigned noactivity:1;
        unsigned bit5:1;
        unsigned bit6:1;
        unsigned r_w:1;
    } comm_stat;
    unsigned char b[4];
} TxBuf;

extern char CmdBuf[];

/* ================================================================
 * I2C bus simulation helpers
 *
 * SSPSTAT bit layout (I2CSTAT in slavenode.c):
 *   bit 0: BF       bit 2: R_nW     bit 3: S      bit 5: D_nA
 *
 * STATE 1 - Master Write + Address:  BF=1, R_nW=0, S=1, D_nA=0
 * STATE 2 - Master Write + Data:     BF=1, R_nW=0,      D_nA=1
 * STATE 3 - Master Read  + Address:  BF=0, R_nW=1, S=1, D_nA=0
 * STATE 4 - Master Read  + Data:     BF=0, R_nW=1,      D_nA=1
 * ================================================================ */
#define SSP_WRITE_ADDR  0x09
#define SSP_WRITE_DATA  0x21
#define SSP_READ_ADDR   0x0C
#define SSP_READ_DATA   0x24

static void sim_i2c_write_addr(unsigned char addr) {
    SSPBUF = addr;
    SSPOV = 0;
    SSPSTAT = SSP_WRITE_ADDR;
    SSP_Handler();
}

static void sim_i2c_write_data(unsigned char data) {
    SSPBUF = data;
    SSPOV = 0;
    SSPSTAT = SSP_WRITE_DATA;
    SSP_Handler();
}

static unsigned char sim_i2c_read_addr(void) {
    WCOL = 0;
    SSPSTAT = SSP_READ_ADDR;
    SSP_Handler();
    return SSPBUF;
}

static unsigned char sim_i2c_read_data(void) {
    WCOL = 0;
    SSPSTAT = SSP_READ_DATA;
    SSP_Handler();
    return SSPBUF;
}

/*
 * Send a complete I2C command frame from BP to BA with correct CRC.
 * payload = [cmd_code, data_bytes...], payload_len includes cmd byte.
 */
static void sim_send_frame(unsigned char slave_addr,
                           const unsigned char *payload,
                           unsigned char payload_len) {
    unsigned short crc = us_CalculerCRCSurTrame(
        (unsigned char *)payload, payload_len);

    sim_i2c_write_addr(slave_addr);
    for (unsigned char i = 0; i < payload_len; i++)
        sim_i2c_write_data(payload[i]);
    sim_i2c_write_data(crc & 0xFF);
    sim_i2c_write_data((crc >> 8) & 0xFF);
}

/* Read the 5-byte BA response */
static void sim_read_response(unsigned char resp[5]) {
    resp[0] = sim_i2c_read_addr();
    for (int i = 1; i < 5; i++)
        resp[i] = sim_i2c_read_data();
}

/* ================================================================ */

void setUp(void) {
    vd_I2C_Setup();
    memset(&TxBuf, 0, sizeof(TxBuf));
    SSPOV = 0;
    WCOL = 0;
    CKP = 0;
}

void tearDown(void) {}

/* ---- Test: valid CRC → BA accepts command and responds correctly ---- */
void test_i2c_valid_crc(void) {
    /* C_ACTIONS (code 5): cmd + 1 data byte → RXByteCount=4 */
    unsigned char payload[] = {0x05, 0x01};
    sim_send_frame(0x12, payload, 2);

    TEST_ASSERT_EQUAL_UINT(0, TxBuf.comm_stat.chkfail);
    TEST_ASSERT_EQUAL_UINT(0, TxBuf.comm_stat.rxerror);
    TEST_ASSERT_EQUAL(5, CmdBuf[0]);

    unsigned char resp[5];
    sim_read_response(resp);

    TEST_ASSERT_EQUAL_UINT8(0x05, resp[0]);

    /* Verify the response CRC covers resp[0..2] */
    unsigned short resp_crc = us_CalculerCRCSurTrame(resp, 3);
    TEST_ASSERT_EQUAL_UINT8(resp_crc & 0xFF, resp[3]);
    TEST_ASSERT_EQUAL_UINT8((resp_crc >> 8) & 0xFF, resp[4]);
}

/* ---- Test: invalid CRC → BA signals chkfail ---- */
void test_i2c_invalid_crc(void) {
    sim_i2c_write_addr(0x12);
    sim_i2c_write_data(0x05);   /* C_ACTIONS */
    sim_i2c_write_data(0x01);   /* data */
    sim_i2c_write_data(0xAA);   /* wrong CRC low */
    sim_i2c_write_data(0xBB);   /* wrong CRC high */

    TEST_ASSERT_EQUAL_UINT(1, TxBuf.comm_stat.chkfail);
    TEST_ASSERT_EQUAL_UINT(1, TxBuf.comm_stat.rxerror);
}

/* ---- Test: unknown command 0xFF → BA signals error ---- */
void test_i2c_unknown_command(void) {
    sim_i2c_write_addr(0x12);
    sim_i2c_write_data(0xFF);   /* unknown command code */

    TEST_ASSERT_EQUAL_UINT(1, TxBuf.comm_stat.ovflw);
    TEST_ASSERT_EQUAL_UINT(1, TxBuf.comm_stat.rxerror);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_i2c_valid_crc);
    RUN_TEST(test_i2c_invalid_crc);
    RUN_TEST(test_i2c_unknown_command);
    return UNITY_END();
}
