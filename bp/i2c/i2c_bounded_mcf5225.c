/*
 * i2c_bounded_mcf5225.c -- HAL cible de i2c_bounded pour le MCF52259 (SC944D).
 *
 * Feature : i2c-surge-protection-2026-10-007
 * Refs    : essensys-hub/essensys-board-SC944D#1
 *
 * - Horloge : ticks MQX + compteur materiel du tick (_time_get_elapsed_ticks
 *   puis _time_diff_microseconds), rebasee avant tout debordement.
 * - Recuperation : I2C0 sur PAS0 (SCL) et PAS1 (SDA). En GPIO, le drain
 *   ouvert est emule : niveau bas = sortie a 0 (DDR a 1, CLRAS), niveau haut =
 *   entree (DDR a 0), la resistance de tirage du bus fixant le niveau.
 *
 * Non teste sur cible : AUCUN binaire contenant ce fichier n'est qualifie
 * avant NR-board-sc944d-3 et -4 sur banc (tache 5.12).
 */
#include <mqx.h>
#include <bsp.h>

#include "i2c_bounded.h"

#define PAS_SCL_MASK   0x01u   /* PAS0 */
#define PAS_SDA_MASK   0x02u   /* PAS1 */
#define PASPAR_I2C0    0x05u   /* PASPA0 = PASPA1 = 01 : fonction I2C0 */
#define PASPAR_MASK    0x0Fu

static MQX_TICK_STRUCT s_origin;
static uint32_t        s_base_us = 0;
static int             s_origin_set = 0;

static uint32_t hal_now_us(void)
{
    MQX_TICK_STRUCT now;
    bool overflow = FALSE;
    int32_t diff;

    _time_get_elapsed_ticks(&now);
    if (!s_origin_set) {
        s_origin = now;
        s_origin_set = 1;
    }
    diff = _time_diff_microseconds(&now, &s_origin, &overflow);
    if (overflow || diff < 0 || diff > 1000000000L) {
        /* Rebase : seules les differences sont utilisees (arithmetique
         * modulo 2^32), le rebasage n'introduit donc aucun saut. */
        s_base_us += (overflow || diff < 0) ? 0u : (uint32_t)diff;
        s_origin = now;
        diff = 0;
    }
    return s_base_us + (uint32_t)diff;
}

static void hal_delay_us(uint32_t us)
{
    uint32_t start = hal_now_us();
    while ((uint32_t)(hal_now_us() - start) < us) {
        /* attente active courte (recuperation de bus uniquement) */
    }
}

static VMCF5225_STRUCT_PTR regs(void)
{
    return (VMCF5225_STRUCT_PTR)_PSP_GET_IPSBAR();
}

static void hal_pins_gpio(int gpio)
{
    VMCF5225_STRUCT_PTR r = regs();
    if (gpio) {
        r->GPIO.DDRAS &= (uint8_t)~(PAS_SCL_MASK | PAS_SDA_MASK);  /* entrees */
        r->GPIO.CLRAS = (uint8_t)~(PAS_SCL_MASK | PAS_SDA_MASK);   /* donnees a 0 */
        r->GPIO.PASPAR &= (uint8_t)~PASPAR_MASK;                   /* GPIO */
    } else {
        r->GPIO.DDRAS &= (uint8_t)~(PAS_SCL_MASK | PAS_SDA_MASK);
        r->GPIO.PASPAR = (uint8_t)((r->GPIO.PASPAR & (uint8_t)~PASPAR_MASK) | PASPAR_I2C0);
    }
}

static void drive(uint8_t mask, int low)
{
    VMCF5225_STRUCT_PTR r = regs();
    if (low) {
        r->GPIO.CLRAS = (uint8_t)~mask;
        r->GPIO.DDRAS |= mask;
    } else {
        r->GPIO.DDRAS &= (uint8_t)~mask;
    }
}

static void hal_scl_drive(int low) { drive(PAS_SCL_MASK, low); }
static void hal_sda_drive(int low) { drive(PAS_SDA_MASK, low); }

static int hal_scl_read(void) { return (regs()->GPIO.PORTASP_SETAS & PAS_SCL_MASK) != 0; }
static int hal_sda_read(void) { return (regs()->GPIO.PORTASP_SETAS & PAS_SDA_MASK) != 0; }

static void hal_i2cr_write(uint8_t v) { regs()->I2C1.I2CR = v;   /* I2C1 dans MQX = module I2C0 du MCF52259 */ }
static uint8_t hal_i2sr_read(void)    { return regs()->I2C1.I2SR; }

static const i2cb_hal_t s_mcf5225_hal = {
    hal_now_us,
    hal_delay_us,
    hal_pins_gpio,
    hal_scl_drive,
    hal_sda_drive,
    hal_scl_read,
    hal_sda_read,
    hal_i2cr_write,
    hal_i2sr_read,
};

/* A appeler une fois au demarrage, avant la premiere transaction I2C. */
void i2cb_init_mcf5225(void)
{
    i2cb_init(&s_mcf5225_hal);
}
