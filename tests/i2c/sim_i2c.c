/*
 * sim_i2c.c -- Bus I2C et horloge simules (voir sim_i2c.h).
 *
 * Feature : i2c-surge-protection-2026-10-007
 * Refs    : essensys-hub/essensys-board-SC944D#1
 */
#include "sim_i2c.h"

#include <string.h>
#include <stdlib.h>

sim_t sim;
volatile MCF52XX_I2C_STRUCT sim_regs;

static int s_bus_active = 0;
static int s_is_addr_byte = 0;
static int s_rx_done = 0;

#define CR   (sim_regs.I2CR)
#define SR   (sim_regs.I2SR)

/* ------------------------------------------------------------------------ */
/* Stubs MQX utilises par le pilote                                          */
/* ------------------------------------------------------------------------ */
void *_mem_alloc_system_zero(_mqx_uint size) { return calloc(1, size); }
_mqx_uint _mem_free(void *ptr) { free(ptr); return MQX_OK; }
bool _mem_set_type(void *ptr, _mem_type type) { (void)ptr; (void)type; return TRUE; }
void *_bsp_get_i2c_base_address(uint8_t ch) { (void)ch; return (void *)&sim_regs; }
_mqx_int _bsp_i2c_io_init(uint8_t ch) { (void)ch; return 0; }
_mqx_uint _io_i2c_polled_install(char *id,
      _mqx_uint (_CODE_PTR_ a)(void *, void **, char *),
      _mqx_uint (_CODE_PTR_ b)(void *, void *),
      _mqx_int (_CODE_PTR_ c)(void *, char *, _mqx_int),
      _mqx_int (_CODE_PTR_ d)(void *, char *, _mqx_int),
      _mqx_int (_CODE_PTR_ e)(void *, _mqx_uint, _mqx_uint_ptr),
      void *f)
{
    (void)id; (void)a; (void)b; (void)c; (void)d; (void)e; (void)f;
    return MQX_OK;
}

/* ------------------------------------------------------------------------ */
/* Modele du controleur                                                      */
/* ------------------------------------------------------------------------ */
static int line_stuck(void)
{
    return sim.fault == SIM_FAULT_SDA_LOW || sim.fault == SIM_FAULT_SCL_LOW ||
           (sim.fault == SIM_FAULT_SLAVE_MID_BYTE && sim.mid_byte_clocks > 0);
}

static void sim_step(void)
{
    if (line_stuck()) {
        /* Bus inutilisable : aucun octet ne se termine, le bus parait occupe. */
        SR |= MCF52XX_I2C_I2SR_IBB;
        SR &= (uint8_t)~MCF52XX_I2C_I2SR_ICF;
        return;
    }

    if (0 == (CR & MCF52XX_I2C_I2CR_MSTA)) {
        if (s_bus_active) {
            s_bus_active = 0;          /* STOP */
            sim.byte_busy = 0;
            SR &= (uint8_t)~MCF52XX_I2C_I2SR_IBB;
        }
        return;
    }
    if (!s_bus_active) {               /* START */
        s_bus_active = 1;
        sim.next_is_address = 1;
        s_rx_done = 0;
        SR |= MCF52XX_I2C_I2SR_IBB;
    }

    if (sim.byte_busy) {
        if ((int32_t)(sim.now_us - sim.byte_done_at) >= 0) {
            sim.byte_busy = 0;
            SR |= (uint8_t)(MCF52XX_I2C_I2SR_IIF | MCF52XX_I2C_I2SR_ICF);
            if (s_is_addr_byte || (CR & MCF52XX_I2C_I2CR_MTX)) {
                if (sim.addressed_ok) SR &= (uint8_t)~MCF52XX_I2C_I2SR_RXAK;
                else                  SR |= MCF52XX_I2C_I2SR_RXAK;
            } else {
                sim_regs.I2DR = (sim.rx_idx < sim.rx_len) ? sim.rx_bytes[sim.rx_idx] : 0xFFu;
                sim.rx_idx++;
                if (CR & MCF52XX_I2C_I2CR_TXAK) {
                    s_rx_done = 1;     /* dernier octet non acquitte par le maitre */
                }
            }
        }
        return;
    }

    if (SR & MCF52XX_I2C_I2SR_IIF) {
        return;                        /* le pilote n'a pas encore traite l'octet */
    }
    if (0 == (CR & MCF52XX_I2C_I2CR_MTX) && s_rx_done) {
        return;                        /* reception terminee */
    }

    /* Debut d'un octet */
    sim.byte_busy = 1;
    sim.byte_done_at = sim.now_us + SIM_BYTE_US + sim.stretch_us;
    SR &= (uint8_t)~MCF52XX_I2C_I2SR_ICF;
    s_is_addr_byte = 0;
    if (sim.next_is_address || (CR & MCF52XX_I2C_I2CR_RSTA)) {
        uint8_t a = sim_regs.I2DR;
        CR &= (uint8_t)~MCF52XX_I2C_I2CR_RSTA;
        sim.next_is_address = 0;
        s_is_addr_byte = 1;
        sim.reading = (a & 1u) != 0;
        sim.addressed_ok = (sim.present_addr != 0) && ((a >> 1) == sim.present_addr);
        sim.rx_idx = 0;
        s_rx_done = 0;
    } else if (CR & MCF52XX_I2C_I2CR_MTX) {
        if (sim.tx_log_len < sizeof(sim.tx_log)) {
            sim.tx_log[sim.tx_log_len++] = sim_regs.I2DR;
        }
    }
}

void sim_advance_us(uint32_t us)
{
    while (us > 0) {
        uint32_t step = us > SIM_TICK_US ? SIM_TICK_US : us;
        sim.now_us += step;
        us -= step;
        sim_step();
    }
}

/* ------------------------------------------------------------------------ */
/* HAL simulee                                                               */
/* ------------------------------------------------------------------------ */
static uint32_t h_now_us(void)
{
    sim_advance_us(SIM_TICK_US);
    return sim.now_us;
}

static void h_delay_us(uint32_t us) { sim_advance_us(us); }

static int pin_scl(void)
{
    if (sim.fault == SIM_FAULT_SCL_LOW) return 0;
    return !sim.scl_driven_low;
}

static int pin_sda(void)
{
    if (sim.fault == SIM_FAULT_SDA_LOW) return 0;
    if (sim.fault == SIM_FAULT_SLAVE_MID_BYTE && sim.mid_byte_clocks > 0) return 0;
    return !sim.sda_driven_low;
}

static void h_pins_gpio(int gpio) { sim.gpio_mode = gpio; }

static void h_scl_drive(int low)
{
    int was_low = sim.scl_driven_low;
    sim.scl_driven_low = low ? 1 : 0;
    /* Un front montant de SCL avec SDA relachee par le maitre est une
     * impulsion de recuperation ; avec SDA tiree basse, c'est la
     * preparation d'un STOP. */
    if (sim.gpio_mode && was_low && !low && pin_scl() && !sim.sda_driven_low) {
        sim.clock_pulses++;
        if (sim.fault == SIM_FAULT_SLAVE_MID_BYTE && sim.mid_byte_clocks > 0) {
            sim.mid_byte_clocks--;
        }
    }
}

static void h_sda_drive(int low)
{
    int was_low = sim.sda_driven_low;
    sim.sda_driven_low = low ? 1 : 0;
    if (sim.gpio_mode && was_low && !low && pin_scl() && pin_sda()) {
        sim.stop_seen++;
    }
}

static int h_scl_read(void) { return pin_scl(); }
static int h_sda_read(void) { return pin_sda(); }

static void h_i2cr_write(uint8_t v)
{
    if (sim.i2cr_write_count < sizeof(sim.i2cr_writes)) {
        sim.i2cr_writes[sim.i2cr_write_count] = v;
    }
    sim.i2cr_write_count++;
    sim_regs.I2CR = v;
    if (v == 0) {
        sim_regs.I2SR = 0;
    }
}

static uint8_t h_i2sr_read(void) { return sim_regs.I2SR; }

const i2cb_hal_t sim_hal = {
    h_now_us, h_delay_us, h_pins_gpio, h_scl_drive, h_sda_drive,
    h_scl_read, h_sda_read, h_i2cr_write, h_i2sr_read,
};

void sim_reset(void)
{
    memset(&sim, 0, sizeof(sim));
    memset((void *)&sim_regs, 0, sizeof(sim_regs));
    sim_regs.I2CR = MCF52XX_I2C_I2CR_IEN;
    s_bus_active = 0;
    s_is_addr_byte = 0;
    s_rx_done = 0;
    sim.now_us = 1000u;
    i2cb_reset_state();
    i2cb_init(&sim_hal);
}

/* ------------------------------------------------------------------------ */
/* Pilote de bout en bout                                                    */
/* ------------------------------------------------------------------------ */
void sim_drv_open(sim_drv_t *d)
{
    memset(d, 0, sizeof(*d));
    d->info.I2C_PTR = &sim_regs;
    d->info.MODE = I2C_MODE_MASTER;
    d->info.STATE = I2C_STATE_READY;
    d->dev.DEV_INFO_PTR = (void *)&d->info;
}

/* Reproduit la sequence de sl_fct_write_polled() du BP 099-37 :
 * SET_DESTINATION_ADDRESS, fwrite(0) (START + adresse), FLUSH (ACK),
 * fwrite(donnees), fflush, REPEATED_START, SET_RX_REQUEST, fread, STOP.
 * Entre deux etapes, le budget de transaction est verifie. */
int sim_transaction(sim_drv_t *d, uint8_t addr, const uint8_t *tx, unsigned txlen,
                    uint8_t *rx, unsigned rxlen, uint32_t *elapsed_us)
{
    VMCF52XX_I2C_INFO_STRUCT_PTR info = (VMCF52XX_I2C_INFO_STRUCT_PTR)&d->info;
    uint32_t param;
    uint32_t start = sim.now_us;
    int step = 0;
    uint8_t buf[32];

    memcpy(buf, tx, txlen);
    i2cb_txn_begin();

    param = addr;
    if (_mcf52xx_i2c_polled_ioctl(info, IO_IOCTL_I2C_SET_DESTINATION_ADDRESS, &param) != I2C_OK) { step = 1; goto out; }
    if (_mcf52xx_i2c_polled_tx(&d->dev, buf, 0) != 0)                                          { step = 2; goto out; }
    if (i2cb_txn_expired())                                                                     { step = 2; goto out; }
    param = 0;
    if (_mcf52xx_i2c_polled_ioctl(info, IO_IOCTL_FLUSH_OUTPUT, &param) != I2C_OK || param != 0) { step = 3; goto out; }
    if (i2cb_txn_expired())                                                                     { step = 3; goto out; }
    if (_mcf52xx_i2c_polled_tx(&d->dev, buf, txlen) != txlen)                                   { step = 4; goto out; }
    if (i2cb_txn_expired())                                                                     { step = 4; goto out; }
    if (_mcf52xx_i2c_polled_ioctl(info, IO_IOCTL_FLUSH_OUTPUT, NULL) != I2C_OK)                 { step = 5; goto out; }
    if (i2cb_txn_expired())                                                                     { step = 5; goto out; }
    if (_mcf52xx_i2c_polled_ioctl(info, IO_IOCTL_I2C_REPEATED_START, NULL) != I2C_OK)           { step = 6; goto out; }
    param = rxlen;
    if (_mcf52xx_i2c_polled_ioctl(info, IO_IOCTL_I2C_SET_RX_REQUEST, &param) != I2C_OK)         { step = 7; goto out; }
    if (_mcf52xx_i2c_polled_rx(&d->dev, rx, rxlen) != rxlen)                                    { step = 8; goto out; }
    if (i2cb_txn_expired())                                                                     { step = 8; goto out; }
out:
    /* STOP dans tous les cas, comme 099-37 */
    if (_mcf52xx_i2c_polled_ioctl(info, IO_IOCTL_I2C_STOP, NULL) != I2C_OK && step == 0) {
        step = 9;
    }
    i2cb_txn_end();
    if (elapsed_us != NULL) {
        *elapsed_us = sim.now_us - start;
    }
    return step;
}
