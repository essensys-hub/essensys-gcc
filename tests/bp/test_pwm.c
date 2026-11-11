/* Test sur hôte du cœur du pilote PWM MCF5225x (feature bp-gcc-firmware-2026-11-006).
 * Valeurs attendues : séquence de Hard.c (099-37) et pilote d'origine désassemblé. */
#include "unity.h"
#include <string.h>
#include "../../bp/drivers/pwm/pwm_mcf5225.h"

static uint8_t pwm[0x30], gpio[0x80];

void setUp(void) { memset(pwm, 0, sizeof pwm); memset(gpio, 0xFF, sizeof gpio); pwm_core_bind(pwm, gpio); }
void tearDown(void) {}

static void enable_siren(void)
{
    PWM_DEV_ENABLE_PIN_DATA p = { .pwm_pin = 7, .polarity = 1, .xS_clock_select = 1,
                                  .period = 200, .duty = 100 };
    TEST_ASSERT_EQUAL_INT(0, pwm_core_ioctl(PWM_IOCTL_ENABLE_PIN, &p));
}

void test_config_programme_les_diviseurs(void)
{
    PWM_DEV_CONFIG_DATA c = { 0, 0, 60, 60 };
    TEST_ASSERT_EQUAL_INT(0, pwm_core_ioctl(PWM_IOCTL_CONFIG, &c));
    TEST_ASSERT_EQUAL_HEX8(0x00, pwm[0x03]);
    TEST_ASSERT_EQUAL_HEX8(60, pwm[0x08]);
    TEST_ASSERT_EQUAL_HEX8(60, pwm[0x09]);
}

void test_sirene_canal_7_active(void)
{
    enable_siren();
    TEST_ASSERT_EQUAL_HEX8(0x80, pwm[0x00]);   /* PWME */
    TEST_ASSERT_EQUAL_HEX8(0x80, pwm[0x01]);   /* PWMPOL */
    TEST_ASSERT_EQUAL_HEX8(0x80, pwm[0x02]);   /* PWMCLK */
    TEST_ASSERT_EQUAL_HEX8(0x00, pwm[0x04]);   /* PWMCAE */
    TEST_ASSERT_EQUAL_HEX8(200, pwm[0x14 + 7]);
    TEST_ASSERT_EQUAL_HEX8(100, pwm[0x1C + 7]);
}

void test_broche_7_multiplexee_sur_port_ta(void)
{
    memset(gpio, 0, sizeof gpio);
    gpio[0x26] = 0xFF;
    enable_siren();
    TEST_ASSERT_EQUAL_HEX8(0xF7, gpio[0x26]);   /* DDRTA : bit 3 effacé */
    TEST_ASSERT_EQUAL_HEX8(0xC0, gpio[0x6E]);   /* PTAPAR : champ 3 = 0b11 */
    TEST_ASSERT_EQUAL_HEX8(0x00, gpio[0x6F]);   /* port TC intact */
}

void test_changement_de_rapport_cyclique(void)
{
    enable_siren();
    PWM_DEV_CHANGE_DUTY_DATA d = { 7, 200, 50 };
    TEST_ASSERT_EQUAL_INT(0, pwm_core_ioctl(PWM_IOCTL_CHANGE_DUTY, &d));
    TEST_ASSERT_EQUAL_HEX8(50, pwm[0x1C + 7]);
}

void test_canal_invalide_refuse(void)
{
    PWM_DEV_CHANGE_DUTY_DATA d = { 8, 200, 50 };
    TEST_ASSERT_EQUAL_INT(PWM_ERROR_INVALID, pwm_core_ioctl(PWM_IOCTL_CHANGE_DUTY, &d));
    TEST_ASSERT_EQUAL_INT(PWM_ERROR_INVALID, pwm_core_ioctl(0x1804, &d));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_config_programme_les_diviseurs);
    RUN_TEST(test_sirene_canal_7_active);
    RUN_TEST(test_broche_7_multiplexee_sur_port_ta);
    RUN_TEST(test_changement_de_rapport_cyclique);
    RUN_TEST(test_canal_invalide_refuse);
    return UNITY_END();
}
