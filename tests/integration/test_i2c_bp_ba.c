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

/* BA 1.8 (feature i2c-surge-protection-2026-10-007) */
extern void vd_SSP_SurveillanceTick(void);
extern void vd_I2C_PurgerSiLibre(void);
extern void vd_I2C_SignalerRedemarrage(unsigned char);
extern unsigned char uc_I2C_EtatCom(void);
extern unsigned char uc_I2C_CommStat;
extern unsigned char uc_I2C_RxBloque;
extern unsigned char uc_SSP_NbReset;

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
    SSPCON = 0x36;
    PORTC = 0xC0;     /* SCL et SDA hautes */
    uc_SSP_NbReset = 0;
}

/* Lecture de n octets de reponse (le BP 1.8 en lit 6, 099-37 en lit 5) */
static void sim_read_n(unsigned char *resp, int n) {
    resp[0] = sim_i2c_read_addr();
    for (int i = 1; i < n; i++)
        resp[i] = sim_i2c_read_data();
}

/* NR: NR-gcc-13 essensys-hub/essensys-board-SC944D#1
 * BA 1.8 : 6e octet comm_stat (signature 0xA0) avec drapeau de redemarrage,
 * efface une fois lu ; les 5 premiers octets sont inchanges (BP 099-37). */
void test_NR_gcc_13_comm_stat_sixth_byte(void) {
    unsigned char payload[] = {0x05, 0x01};
    unsigned char r5[5], r6[7];

    /* BP 099-37 : lit 5 octets, le drapeau reste en attente */
    sim_send_frame(0x12, payload, 2);
    sim_read_n(r5, 5);
    TEST_ASSERT_EQUAL_UINT8(0x05, r5[0]);
    unsigned short crc = us_CalculerCRCSurTrame(r5, 3);
    TEST_ASSERT_EQUAL_UINT8(crc & 0xFF, r5[3]);
    TEST_ASSERT_EQUAL_UINT8((crc >> 8) & 0xFF, r5[4]);
    TEST_ASSERT_EQUAL_HEX8(0x01, uc_I2C_CommStat & 0x01);

    /* BP modifie : lit 6 octets */
    CmdBuf[0] = 0;
    sim_send_frame(0x12, payload, 2);
    sim_read_n(r6, 7);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(r5, r6, 5);         /* memes 5 octets */
    TEST_ASSERT_EQUAL_HEX8(0xA1, r6[5]);              /* signature + redemarrage */
    TEST_ASSERT_EQUAL_HEX8(0x55, r6[6]);              /* au-dela : octet neutre */

    CmdBuf[0] = 0;
    sim_send_frame(0x12, payload, 2);
    sim_read_n(r6, 6);
    TEST_ASSERT_EQUAL_HEX8(0xA0, r6[5]);              /* drapeau efface */

    /* redemarrage par chien de garde */
    vd_I2C_SignalerRedemarrage(1);
    CmdBuf[0] = 0;
    sim_send_frame(0x12, payload, 2);
    sim_read_n(r6, 6);
    TEST_ASSERT_EQUAL_HEX8(0xA3, r6[5]);
}

/* NR: NR-gcc-14 essensys-hub/essensys-board-SC944D#1
 * BA 1.8 : tampon de commande occupe -> la nouvelle trame est refusee
 * (SSPBUF laisse plein, NACK materiel), jamais acceptee puis perdue ; une
 * fois CmdBuf libre, la purge rouvre la reception et la retransmission
 * est appliquee. */
void test_NR_gcc_14_nack_when_cmdbuf_busy(void) {
    unsigned char first[] = {0x05, 0x01};
    unsigned char second[] = {0x05, 0x02};

    sim_send_frame(0x12, first, 2);
    TEST_ASSERT_EQUAL(5, CmdBuf[0]);
    TEST_ASSERT_EQUAL(1, CmdBuf[1]);

    /* deuxieme trame pendant que CmdBuf est occupe */
    SSPBUF = 0x24;            /* adresse 0x12 << 1 */
    SSPOV = 0;
    SSPSTAT = SSP_WRITE_ADDR;
    SSP_Handler();
    TEST_ASSERT_EQUAL_UINT8(1, uc_I2C_RxBloque);
    TEST_ASSERT_EQUAL_HEX8(0x24, SSPBUF);             /* non lu : BF reste a 1 */
    /* le module refuse les octets suivants (SSPOV) : rien n'est consomme */
    SSPOV = 1;
    SSPSTAT = SSP_WRITE_DATA;
    SSP_Handler();
    TEST_ASSERT_EQUAL(1, CmdBuf[1]);                  /* premiere trame intacte */
    TEST_ASSERT_EQUAL_UINT8(1, SSPOV);
    TEST_ASSERT_EQUAL_UINT8(2, uc_I2C_EtatCom());
    TEST_ASSERT_EQUAL_HEX8(0x08, uc_I2C_CommStat & 0x08);

    /* purge refusee tant que CmdBuf n'est pas traite */
    vd_I2C_PurgerSiLibre();
    TEST_ASSERT_EQUAL_UINT8(1, uc_I2C_RxBloque);

    /* traitement de la premiere commande puis purge */
    CmdBuf[0] = 0;
    vd_I2C_PurgerSiLibre();
    TEST_ASSERT_EQUAL_UINT8(0, uc_I2C_RxBloque);
    TEST_ASSERT_EQUAL_UINT8(0, SSPOV);

    /* retransmission par le BP : appliquee */
    sim_send_frame(0x12, second, 2);
    TEST_ASSERT_EQUAL(5, CmdBuf[0]);
    TEST_ASSERT_EQUAL(2, CmdBuf[1]);
    TEST_ASSERT_EQUAL_UINT(0, TxBuf.comm_stat.rxerror);
}

/* NR: NR-gcc-15 essensys-hub/essensys-board-SC944D#1
 * BA 1.8 : timeout SSP -- ligne basse ou SCL retenue plus de 50 ms sans
 * activite : module SSP reinitialise (SSPEN 0 -> 1, CKP = 1, SSPOV/WCOL
 * purges), sans coupure d'alimentation. */
void test_NR_gcc_15_ssp_timeout_resets_module(void) {
    int i;

    /* SDA retenue basse, module fige */
    PORTC = 0x40;
    CKP = 1;
    SSPOV = 1; WCOL = 1;
    for (i = 0; i < 120; i++) vd_SSP_SurveillanceTick();   /* 49,9 ms */
    TEST_ASSERT_EQUAL_UINT8(0, uc_SSP_NbReset);
    vd_SSP_SurveillanceTick();                              /* 50,3 ms */
    TEST_ASSERT_EQUAL_UINT8(1, uc_SSP_NbReset);
    TEST_ASSERT_EQUAL_HEX8(0x20, SSPCON & 0x20);            /* SSPEN remis a 1 */
    TEST_ASSERT_EQUAL_UINT8(1, CKP);
    TEST_ASSERT_EQUAL_UINT8(0, SSPOV);
    TEST_ASSERT_EQUAL_UINT8(0, WCOL);
    TEST_ASSERT_EQUAL_HEX8(0x04, uc_I2C_CommStat & 0x04);

    /* SCL retenue par le BA (CKP = 0), lignes hautes */
    PORTC = 0xC0;
    CKP = 0;
    for (i = 0; i < 121; i++) vd_SSP_SurveillanceTick();
    TEST_ASSERT_EQUAL_UINT8(2, uc_SSP_NbReset);
    TEST_ASSERT_EQUAL_UINT8(1, CKP);

    /* activite SSP reguliere : jamais de reinitialisation */
    PORTC = 0x40;
    for (i = 0; i < 1000; i++) {
        vd_SSP_SurveillanceTick();
        if ((i % 100) == 0) sim_i2c_write_addr(0x24);       /* activite SSP adressee a ce BA (toutes les 42 ms) */
    }
    TEST_ASSERT_EQUAL_UINT8(2, uc_SSP_NbReset);

    /* bus libre : rien */
    PORTC = 0xC0; CKP = 1;
    for (i = 0; i < 1000; i++) vd_SSP_SurveillanceTick();
    TEST_ASSERT_EQUAL_UINT8(2, uc_SSP_NbReset);
}

/* NR: NR-gcc-15 essensys-hub/essensys-board-SC944D#1
 * BA 1.8 : debordement SSP pendant une trame : signale, puis la trame
 * suivante est acquittee sans cycle d'alimentation ; un etat SSP
 * incoherent reinitialise le module. */
void test_NR_gcc_15_ssp_overflow_then_next_frame_ok(void) {
    unsigned char payload[] = {0x05, 0x01};

    SSPBUF = 0x24; SSPOV = 1; SSPSTAT = SSP_WRITE_ADDR;
    SSP_Handler();
    TEST_ASSERT_EQUAL_UINT(1, TxBuf.comm_stat.sspov);
    TEST_ASSERT_EQUAL_UINT8(0, SSPOV);

    sim_send_frame(0x12, payload, 2);
    TEST_ASSERT_EQUAL(5, CmdBuf[0]);
    TEST_ASSERT_EQUAL_UINT(0, TxBuf.comm_stat.rxerror);

    /* etat incoherent : D_nA=1, R_nW=1, BF=1 */
    SSPSTAT = 0x25;
    SSP_Handler();
    TEST_ASSERT_EQUAL_UINT8(1, uc_SSP_NbReset);
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
    RUN_TEST(test_NR_gcc_13_comm_stat_sixth_byte);
    RUN_TEST(test_NR_gcc_14_nack_when_cmdbuf_busy);
    RUN_TEST(test_NR_gcc_15_ssp_timeout_resets_module);
    RUN_TEST(test_NR_gcc_15_ssp_overflow_then_next_frame_ok);
    return UNITY_END();
}
