/*
 * test_i2c_recovery.c -- Recuperation d'un bus I2C bloque (tache 5.2).
 *
 * Feature : i2c-surge-protection-2026-10-007
 * Refs    : essensys-hub/essensys-board-SC944D#1
 */
#include "unity.h"
#include "sim_i2c.h"

static sim_drv_t drv;
static const uint8_t frame_actions[4] = { 0x05, 0x00, 0x11, 0x22 };

void setUp(void)
{
    sim_reset();
    sim_drv_open(&drv);
    sim.present_addr = 0x11;
    sim.rx_len = 5;
}

void tearDown(void) {}

/* NR: NR-gcc-3 essensys-hub/essensys-board-SC944D#1
 * Esclave bloque au milieu d'un octet : SDA retenue, liberee par les coups
 * d'horloge (9 au plus), puis STOP, I2CR = 0 puis IEN. */
static void test_NR_gcc_3_slave_stuck_mid_byte_is_released(void)
{
    sim.fault = SIM_FAULT_SLAVE_MID_BYTE;
    sim.mid_byte_clocks = 5;

    TEST_ASSERT_EQUAL(I2CB_REC_RELEASED, i2cb_poll(10000u));
    TEST_ASSERT_EQUAL_UINT(5, sim.clock_pulses);
    TEST_ASSERT_GREATER_OR_EQUAL(1, sim.stop_seen);
    TEST_ASSERT_EQUAL_INT(0, sim.gpio_mode);              /* retour en fonction I2C */
    TEST_ASSERT_EQUAL_UINT(2, sim.i2cr_write_count);
    TEST_ASSERT_EQUAL_HEX8(0x00, sim.i2cr_writes[0]);    /* I2CR remis a zero */
    TEST_ASSERT_EQUAL_HEX8(0x80, sim.i2cr_writes[1]);    /* puis IEN */
    TEST_ASSERT_FALSE(i2cb_bus_blocked());

    /* La communication reprend sans redemarrage ni cycle d'alimentation. */
    sim_drv_open(&drv);
    {
        uint8_t rx[5];
        TEST_ASSERT_EQUAL_INT(0, sim_transaction(&drv, 0x11, frame_actions, 4, rx, 5, NULL));
    }
}

/* NR: NR-gcc-3 essensys-hub/essensys-board-SC944D#1
 * Jamais plus de 9 impulsions, meme si SDA reste basse. */
static void test_NR_gcc_3_at_most_nine_clocks(void)
{
    sim.fault = SIM_FAULT_SDA_LOW;
    TEST_ASSERT_EQUAL(I2CB_REC_STILL_STUCK, i2cb_recover(0u));
    TEST_ASSERT_EQUAL_UINT(9, sim.clock_pulses);
    TEST_ASSERT_TRUE(i2cb_bus_blocked());
}

/* NR: NR-gcc-3 essensys-hub/essensys-board-SC944D#1
 * SCL tenue basse : les impulsions sont inutiles, aucune n'est emise ;
 * le controleur est quand meme reinitialise. */
static void test_NR_gcc_3_scl_held_no_clocks_but_controller_reset(void)
{
    sim.fault = SIM_FAULT_SCL_LOW;
    TEST_ASSERT_EQUAL(I2CB_REC_STILL_STUCK, i2cb_recover(0u));
    TEST_ASSERT_EQUAL_UINT(0, sim.clock_pulses);
    TEST_ASSERT_EQUAL_UINT(2, sim.i2cr_write_count);
}

/* NR: NR-gcc-3 essensys-hub/essensys-board-SC944D#1
 * Defaut permanent : au plus une tentative par seconde. */
static void test_NR_gcc_3_rate_limited_to_one_per_second(void)
{
    uint32_t t;
    unsigned attempts = 0;

    sim.fault = SIM_FAULT_SDA_LOW;
    /* i2cb_poll appele toutes les 50 ms pendant 60 s */
    for (t = 0; t < 60000u; t += 50u) {
        i2cb_rec_result_t r = i2cb_poll(t);
        if (r == I2CB_REC_STILL_STUCK || r == I2CB_REC_RELEASED) {
            attempts++;
        }
    }
    TEST_ASSERT_EQUAL_UINT(60, attempts);
    TEST_ASSERT_EQUAL_UINT32(60, i2cb_stats()->recoveries);
    TEST_ASSERT_TRUE(i2cb_bus_blocked());
    TEST_ASSERT_EQUAL_UINT32(0u, i2cb_blocked_since_ms());
}

/* NR: NR-gcc-3 essensys-hub/essensys-board-SC944D#1
 * Ligne basse detectee bus au repos : tentative au premier poll (< 100 ms). */
static void test_NR_gcc_3_first_poll_attempts_immediately(void)
{
    sim.fault = SIM_FAULT_SDA_LOW;
    TEST_ASSERT_EQUAL(I2CB_REC_STILL_STUCK, i2cb_poll(42u));
    TEST_ASSERT_EQUAL_UINT32(42u, i2cb_blocked_since_ms());
}

/* NR: NR-gcc-3 essensys-hub/essensys-board-SC944D#1
 * Ligne forcee basse 500 ms puis relachee : le bus redevient libre au
 * poll suivant la fin du defaut (bien avant 10 s). */
static void test_NR_gcc_3_line_low_500ms_then_released(void)
{
    uint32_t t;
    uint32_t free_at = 0;

    for (t = 0; t < 3000u; t += 50u) {
        sim.fault = (t < 500u) ? SIM_FAULT_SDA_LOW : SIM_FAULT_NONE;
        (void)i2cb_poll(t);
        if (t >= 500u && !i2cb_bus_blocked() && free_at == 0) {
            free_at = t;
        }
    }
    TEST_ASSERT_NOT_EQUAL(0, free_at);
    TEST_ASSERT_LESS_OR_EQUAL_UINT32(500u + 1000u, free_at);
}

/* Bus libre et aucun depassement : pas de recuperation inutile. */
static void test_idle_bus_no_recovery(void)
{
    TEST_ASSERT_EQUAL(I2CB_REC_NOT_NEEDED, i2cb_poll(0u));
    TEST_ASSERT_EQUAL_UINT32(0, i2cb_stats()->recoveries);
    TEST_ASSERT_EQUAL_UINT(0, sim.i2cr_write_count);
}

/* Un depassement d'attente demande une reinitialisation du controleur
 * au poll suivant, meme si les lignes sont revenues hautes. */
static void test_timeout_requests_controller_reset(void)
{
    i2cb_note_timeout(I2CB_SITE_RXTX_BYTE);
    TEST_ASSERT_EQUAL(I2CB_REC_RELEASED, i2cb_poll(0u));
    TEST_ASSERT_EQUAL_UINT(2, sim.i2cr_write_count);
    TEST_ASSERT_EQUAL(I2CB_REC_NOT_NEEDED, i2cb_poll(2000u));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_NR_gcc_3_slave_stuck_mid_byte_is_released);
    RUN_TEST(test_NR_gcc_3_at_most_nine_clocks);
    RUN_TEST(test_NR_gcc_3_scl_held_no_clocks_but_controller_reset);
    RUN_TEST(test_NR_gcc_3_rate_limited_to_one_per_second);
    RUN_TEST(test_NR_gcc_3_first_poll_attempts_immediately);
    RUN_TEST(test_NR_gcc_3_line_low_500ms_then_released);
    RUN_TEST(test_idle_bus_no_recovery);
    RUN_TEST(test_timeout_requests_controller_reset);
    return UNITY_END();
}
