/*
 * test_i2c_bounded.c -- Attentes bornees du pilote I2C du BP (tache 5.1).
 *
 * Feature : i2c-surge-protection-2026-10-007
 * Refs    : essensys-hub/essensys-board-SC944D#1
 *
 * Le vrai pilote bp/drivers/i2c/i2c_pol_mcf52xx.c est exerce sur un bus et
 * une horloge simules (sim_i2c.c), avec la sequence exacte de
 * sl_fct_write_polled() du BP 099-37.
 */
#include "unity.h"
#include "sim_i2c.h"

#include <string.h>

static sim_drv_t drv;

static const uint8_t frame_actions[4] = { 0x05, 0x00, 0x11, 0x22 };   /* trame ACTIONS */
static const uint8_t frame_forcage[11] = { 0x01, 1, 2, 3, 4, 5, 6, 7, 8, 0x33, 0x44 };

void setUp(void)
{
    sim_reset();
    sim_drv_open(&drv);
    sim.present_addr = 0x12;
    sim.rx_len = 5;
    sim.rx_bytes[0] = 0x05; sim.rx_bytes[1] = 0x11; sim.rx_bytes[2] = 0x22;
    sim.rx_bytes[3] = 0xAB; sim.rx_bytes[4] = 0xCD;
}

void tearDown(void) {}

/* Le bus simule repond correctement : la sequence 099-37 aboutit. */
static void test_nominal_transaction_succeeds(void)
{
    uint8_t rx[5] = { 0 };
    uint32_t dt = 0;

    TEST_ASSERT_EQUAL_INT(0, sim_transaction(&drv, 0x12, frame_actions, 4, rx, 5, &dt));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(sim.rx_bytes, rx, 5);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(frame_actions, sim.tx_log, 4);
    TEST_ASSERT_LESS_THAN_UINT32(I2CB_TXN_MAX_US, dt);
    TEST_ASSERT_EQUAL_UINT32(0, i2cb_stats()->txn_timeouts);
}

/* Lecture d'un sixieme octet (comm_stat BA 1.8) : meme sequence, rxlen = 6. */
static void test_six_byte_read_succeeds(void)
{
    uint8_t rx[6] = { 0 };
    sim.rx_len = 6;
    sim.rx_bytes[5] = 0xA1;
    TEST_ASSERT_EQUAL_INT(0, sim_transaction(&drv, 0x12, frame_actions, 4, rx, 6, NULL));
    TEST_ASSERT_EQUAL_HEX8(0xA1, rx[5]);
}

/* NR: NR-gcc-1 essensys-hub/essensys-board-SC944D#1
 * SDA forcee basse : chaque transaction se termine en erreur en 20 ms au
 * plus, et des transactions successives (60 s de defaut) ne bloquent jamais. */
static void test_NR_gcc_1_sda_stuck_low_each_transaction_fails_within_20ms(void)
{
    uint8_t rx[5];
    uint32_t dt;
    int i;

    sim.fault = SIM_FAULT_SDA_LOW;
    for (i = 0; i < 200; i++) {
        dt = 0;
        TEST_ASSERT_NOT_EQUAL(0, sim_transaction(&drv, 0x12, frame_forcage, 11, rx, 5, &dt));
        TEST_ASSERT_LESS_OR_EQUAL_UINT32(I2CB_TXN_MAX_US, dt);
    }
    TEST_ASSERT_GREATER_THAN_UINT32(0, i2cb_stats()->timeouts[I2CB_SITE_TX_ADDR]);
}

/* NR: NR-gcc-2 essensys-hub/essensys-board-SC944D#1
 * SCL forcee basse : meme garantie, attente STOP comprise. */
static void test_NR_gcc_2_scl_stuck_low_each_transaction_fails_within_20ms(void)
{
    uint8_t rx[5];
    uint32_t dt;
    int i;

    sim.fault = SIM_FAULT_SCL_LOW;
    for (i = 0; i < 200; i++) {
        dt = 0;
        TEST_ASSERT_NOT_EQUAL(0, sim_transaction(&drv, 0x12, frame_actions, 4, rx, 5, &dt));
        TEST_ASSERT_LESS_OR_EQUAL_UINT32(I2CB_TXN_MAX_US, dt);
    }
    TEST_ASSERT_GREATER_THAN_UINT32(0, i2cb_stats()->timeouts[I2CB_SITE_STOP_IDLE]);
}

/* NR: NR-gcc-2 essensys-hub/essensys-board-SC944D#1
 * Une ligne collee apparue au milieu d'une transaction : l'octet en cours
 * ne se termine jamais, l'attente est bornee a 2 ms et la transaction a 20 ms. */
static void test_NR_gcc_2_slow_transaction_is_bounded_to_20ms(void)
{
    uint8_t rx[5];
    uint32_t dt = 0;
    int step;

    /* L'esclave etire chaque octet de 1,7 ms (1,88 ms par octet) : chaque
     * attente reste sous la borne de 2 ms, mais 11 octets + adresse
     * depassent 20 ms. */
    sim.stretch_us = 1700u;
    step = sim_transaction(&drv, 0x12, frame_forcage, 11, rx, 5, &dt);
    TEST_ASSERT_NOT_EQUAL(0, step);
    TEST_ASSERT_LESS_OR_EQUAL_UINT32(I2CB_TXN_MAX_US + 2u * SIM_TICK_US * 4u, dt);
    TEST_ASSERT_GREATER_THAN_UINT32(0, i2cb_stats()->txn_timeouts);
}

/* NR: NR-gcc-1 essensys-hub/essensys-board-SC944D#1
 * Etirement d'horloge d'un BA 1.7 sous la borne : aucune erreur d'attente. */
static void test_NR_gcc_1_clock_stretch_below_bound_is_not_an_error(void)
{
    uint8_t rx[5];
    uint32_t dt = 0;

    sim.stretch_us = 1000u;   /* 1 ms par octet, < 2 ms */
    TEST_ASSERT_EQUAL_INT(0, sim_transaction(&drv, 0x12, frame_actions, 4, rx, 5, &dt));
    TEST_ASSERT_EQUAL_UINT32(0, i2cb_stats()->timeouts[I2CB_SITE_RXTX_BYTE]);
    TEST_ASSERT_LESS_THAN_UINT32(I2CB_TXN_MAX_US, dt);
}

/* NR: NR-gcc-1 essensys-hub/essensys-board-SC944D#1
 * Borne relevee a 2 x l'etirement mesure (> 1 ms), plafonnee a 20 ms. */
static void test_NR_gcc_1_wait_bound_can_be_raised_but_not_beyond_txn(void)
{
    uint8_t rx[5];

    TEST_ASSERT_EQUAL_UINT32(2000u, i2cb_wait_max_us());
    TEST_ASSERT_EQUAL_UINT32(2000u, i2cb_set_wait_max_us(500u));     /* jamais sous 2 ms */
    TEST_ASSERT_EQUAL_UINT32(3000u, i2cb_set_wait_max_us(3000u));    /* 2 x 1,5 ms */
    TEST_ASSERT_EQUAL_UINT32(20000u, i2cb_set_wait_max_us(50000u));  /* plafond */

    i2cb_set_wait_max_us(3000u);
    sim.stretch_us = 2500u;   /* sous la nouvelle borne, 4 + 5 octets < 20 ms ? non : */
    sim.rx_len = 5;
    /* trame ACTIONS (adresse + 4) + lecture (adresse + 5) = 11 octets x 2,68 ms > 20 ms :
     * le budget de transaction s'applique toujours. */
    TEST_ASSERT_NOT_EQUAL(0, sim_transaction(&drv, 0x12, frame_actions, 4, rx, 5, NULL));
    TEST_ASSERT_EQUAL_UINT32(0, i2cb_stats()->timeouts[I2CB_SITE_TX_ADDR]);
}

/* Esclave absent (NACK d'adresse) : erreur immediate, aucun depassement. */
static void test_absent_slave_nack_is_fast_error(void)
{
    uint8_t rx[5];
    uint32_t dt = 0;

    sim.present_addr = 0;
    TEST_ASSERT_EQUAL_INT(3, sim_transaction(&drv, 0x12, frame_actions, 4, rx, 5, &dt));
    TEST_ASSERT_LESS_THAN_UINT32(2000u, dt);
}

/* Apres la disparition du defaut, la transaction suivante reussit. */
static void test_recovers_after_fault_cleared(void)
{
    uint8_t rx[5];

    sim.fault = SIM_FAULT_SDA_LOW;
    TEST_ASSERT_NOT_EQUAL(0, sim_transaction(&drv, 0x12, frame_actions, 4, rx, 5, NULL));
    sim.fault = SIM_FAULT_NONE;
    TEST_ASSERT_EQUAL(I2CB_REC_RELEASED, i2cb_poll(5000u));
    sim_drv_open(&drv);
    TEST_ASSERT_EQUAL_INT(0, sim_transaction(&drv, 0x12, frame_actions, 4, rx, 5, NULL));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_nominal_transaction_succeeds);
    RUN_TEST(test_six_byte_read_succeeds);
    RUN_TEST(test_NR_gcc_1_sda_stuck_low_each_transaction_fails_within_20ms);
    RUN_TEST(test_NR_gcc_2_scl_stuck_low_each_transaction_fails_within_20ms);
    RUN_TEST(test_NR_gcc_2_slow_transaction_is_bounded_to_20ms);
    RUN_TEST(test_NR_gcc_1_clock_stretch_below_bound_is_not_an_error);
    RUN_TEST(test_NR_gcc_1_wait_bound_can_be_raised_but_not_beyond_txn);
    RUN_TEST(test_absent_slave_nack_is_fast_error);
    RUN_TEST(test_recovers_after_fault_cleared);
    return UNITY_END();
}
