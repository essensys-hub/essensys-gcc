/*
 * i2c_bounded.c -- Attentes I2C bornees et recuperation de bus (BP SC944D).
 *
 * Feature : i2c-surge-protection-2026-10-007
 * Refs    : essensys-hub/essensys-board-SC944D#1
 *
 * Voir i2c_bounded.h. Aucune dependance MQX : teste sur l'hote (tests/i2c).
 */
#include "i2c_bounded.h"

#include <stddef.h>
#include <string.h>

static const i2cb_hal_t *s_hal = NULL;

static uint32_t s_wait_max_us = I2CB_WAIT_MAX_US;

static int      s_txn_active = 0;
static uint32_t s_txn_start_us = 0;
static int      s_txn_timeout_noted = 0;

static int      s_recovery_requested = 0;
static int      s_have_last_recovery = 0;
static uint32_t s_last_recovery_ms = 0;

static int      s_blocked = 0;
static uint32_t s_blocked_since_ms = 0;

static i2cb_stats_t s_stats;

/* ------------------------------------------------------------------------ */

void i2cb_init(const i2cb_hal_t *hal)
{
    s_hal = hal;
}

const i2cb_hal_t *i2cb_hal(void)
{
    return s_hal;
}

void i2cb_reset_state(void)
{
    s_wait_max_us = I2CB_WAIT_MAX_US;
    s_txn_active = 0;
    s_txn_start_us = 0;
    s_txn_timeout_noted = 0;
    s_recovery_requested = 0;
    s_have_last_recovery = 0;
    s_last_recovery_ms = 0;
    s_blocked = 0;
    s_blocked_since_ms = 0;
    memset(&s_stats, 0, sizeof(s_stats));
}

static uint32_t now_us(void)
{
    return (s_hal != NULL && s_hal->now_us != NULL) ? s_hal->now_us() : 0u;
}

static void delay_us(uint32_t us)
{
    if (s_hal != NULL && s_hal->delay_us != NULL) {
        s_hal->delay_us(us);
    }
}

/* ------------------------------------------------------------------------ */
/* Bornes                                                                    */
/* ------------------------------------------------------------------------ */

uint32_t i2cb_set_wait_max_us(uint32_t us)
{
    /* Jamais moins que la borne par defaut (2 ms), jamais plus que la borne
     * de transaction : la spec autorise seulement un relevement a 2 x
     * l'etirement mesure, la transaction restant bornee a 20 ms. */
    if (us < I2CB_WAIT_MAX_US) {
        us = I2CB_WAIT_MAX_US;
    }
    if (us > I2CB_TXN_MAX_US) {
        us = I2CB_TXN_MAX_US;
    }
    s_wait_max_us = us;
    return s_wait_max_us;
}

uint32_t i2cb_wait_max_us(void)
{
    return s_wait_max_us;
}

void i2cb_txn_begin(void)
{
    s_txn_active = 1;
    s_txn_timeout_noted = 0;
    s_txn_start_us = now_us();
}

void i2cb_txn_end(void)
{
    s_txn_active = 0;
}

uint32_t i2cb_txn_elapsed_us(void)
{
    if (!s_txn_active) {
        return 0u;
    }
    return (uint32_t)(now_us() - s_txn_start_us);
}

int i2cb_txn_expired(void)
{
    if (!s_txn_active) {
        return 0;
    }
    if (i2cb_txn_elapsed_us() >= I2CB_TXN_MAX_US) {
        if (!s_txn_timeout_noted) {
            s_txn_timeout_noted = 1;
            s_stats.txn_timeouts++;
        }
        return 1;
    }
    return 0;
}

void i2cb_wait_begin(i2cb_deadline_t *dl)
{
    uint32_t limit = s_wait_max_us;

    dl->start_us = now_us();
    if (s_txn_active) {
        uint32_t elapsed = (uint32_t)(dl->start_us - s_txn_start_us);
        uint32_t remaining = (elapsed >= I2CB_TXN_MAX_US) ? 0u : (I2CB_TXN_MAX_US - elapsed);
        if (remaining < limit) {
            limit = remaining;
        }
    }
    dl->limit_us = limit;
}

int i2cb_wait_expired(const i2cb_deadline_t *dl)
{
    uint32_t elapsed = (uint32_t)(now_us() - dl->start_us);

    if (elapsed > s_stats.max_wait_us && elapsed <= dl->limit_us) {
        s_stats.max_wait_us = elapsed;
    }
    return elapsed >= dl->limit_us;
}

void i2cb_note_timeout(int site)
{
    if (site > 0 && site < I2CB_SITE_COUNT) {
        s_stats.timeouts[site]++;
    }
    (void)i2cb_txn_expired();
    /* Une attente depassee signifie un bus suspect : la recuperation sera
     * tentee au prochain i2cb_poll(), dans les 100 ms. */
    s_recovery_requested = 1;
}

/* ------------------------------------------------------------------------ */
/* Recuperation de bus                                                       */
/* ------------------------------------------------------------------------ */

static int scl_high(void) { return s_hal->scl_read() != 0; }
static int sda_high(void) { return s_hal->sda_read() != 0; }

static uint8_t lines_now(void)
{
    return (uint8_t)((scl_high() ? 0x01u : 0u) | (sda_high() ? 0x02u : 0u));
}

uint8_t i2cb_sample_lines(void)
{
    uint8_t lines;

    if (s_hal == NULL) {
        return 0x03u;
    }
    s_hal->scl_drive(0);
    s_hal->sda_drive(0);
    s_hal->pins_gpio(1);
    delay_us(I2CB_RECOVERY_HALF_PERIOD_US);
    lines = lines_now();
    s_hal->pins_gpio(0);
    return lines;
}

static void set_blocked(int blocked, uint32_t now_ms)
{
    if (blocked) {
        if (!s_blocked) {
            s_blocked = 1;
            s_blocked_since_ms = now_ms;
        }
    } else {
        s_blocked = 0;
    }
}

i2cb_rec_result_t i2cb_recover(uint32_t now_ms)
{
    unsigned int clocks = 0;
    uint8_t lines;

    if (s_hal == NULL) {
        return I2CB_REC_NOT_NEEDED;
    }
    if (s_have_last_recovery &&
        (uint32_t)(now_ms - s_last_recovery_ms) < I2CB_RECOVERY_MIN_INTERVAL_MS) {
        return I2CB_REC_RATE_LIMITED;
    }
    s_have_last_recovery = 1;
    s_last_recovery_ms = now_ms;
    s_recovery_requested = 0;
    s_stats.recoveries++;

    /* 1. PAS0/PAS1 en GPIO, lignes relachees, lecture des niveaux. */
    s_hal->scl_drive(0);
    s_hal->sda_drive(0);
    s_hal->pins_gpio(1);
    delay_us(I2CB_RECOVERY_HALF_PERIOD_US);

    /* 2. Jusqu'a 9 impulsions d'horloge tant que SDA est retenue basse et que
     *    SCL est libre (un esclave bloque au milieu d'un octet finit son
     *    octet et relache SDA). Si SCL est elle-meme tenue, les impulsions
     *    sont sans effet : on n'insiste pas. */
    while (clocks < I2CB_RECOVERY_CLOCKS && scl_high() && !sda_high()) {
        s_hal->scl_drive(1);
        delay_us(I2CB_RECOVERY_HALF_PERIOD_US);
        s_hal->scl_drive(0);
        delay_us(I2CB_RECOVERY_HALF_PERIOD_US);
        clocks++;
    }
    s_stats.recovery_clocks += clocks;

    /* 3. Condition STOP (SDA monte pendant que SCL est haute). */
    if (scl_high()) {
        s_hal->scl_drive(1);
        delay_us(I2CB_RECOVERY_HALF_PERIOD_US);
        s_hal->sda_drive(1);
        delay_us(I2CB_RECOVERY_HALF_PERIOD_US);
        s_hal->scl_drive(0);
        delay_us(I2CB_RECOVERY_HALF_PERIOD_US);
        s_hal->sda_drive(0);
        delay_us(I2CB_RECOVERY_HALF_PERIOD_US);
    }

    lines = lines_now();

    /* 4. Retour en fonction I2C, remise a zero du controleur puis IEN. */
    s_hal->pins_gpio(0);
    s_hal->i2cr_write(0x00u);
    s_hal->i2cr_write(I2CB_I2CR_IEN);

    if (lines == 0x03u) {
        s_stats.recoveries_ok++;
        set_blocked(0, now_ms);
        return I2CB_REC_RELEASED;
    }
    set_blocked(1, now_ms);
    return I2CB_REC_STILL_STUCK;
}

i2cb_rec_result_t i2cb_poll(uint32_t now_ms)
{
    uint8_t lines;

    if (s_hal == NULL) {
        return I2CB_REC_NOT_NEEDED;
    }
    lines = i2cb_sample_lines();
    if (lines == 0x03u && !s_recovery_requested) {
        set_blocked(0, now_ms);
        return I2CB_REC_NOT_NEEDED;
    }
    if (lines != 0x03u) {
        set_blocked(1, now_ms);
    }
    return i2cb_recover(now_ms);
}

int i2cb_bus_blocked(void)
{
    return s_blocked;
}

uint32_t i2cb_blocked_since_ms(void)
{
    return s_blocked_since_ms;
}

/* ------------------------------------------------------------------------ */

const i2cb_stats_t *i2cb_stats(void)
{
    return &s_stats;
}

void i2cb_stats_reset(void)
{
    memset(&s_stats, 0, sizeof(s_stats));
}
