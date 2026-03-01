#include "unity.h"
#include <string.h>

/* TODO: #include "crc.h" once source is linked */

void setUp(void) {}
void tearDown(void) {}

void test_crc_empty(void) {
    /* CRC of empty/null buffer should return initial value (0xFFFF) */
    /* uint16_t result = calcul_crc(NULL, 0); */
    /* TEST_ASSERT_EQUAL_UINT16(0xFFFF, result); */
    TEST_IGNORE_MESSAGE("Waiting for crc.c source to be linked");
}

void test_crc_known_value(void) {
    /* CRC of {0x01, 0x02, 0x03, 0x04} should match firmware reference */
    /* unsigned char data[] = {0x01, 0x02, 0x03, 0x04}; */
    /* TEST_ASSERT_EQUAL_UINT16(expected, calcul_crc(data, 4)); */
    TEST_IGNORE_MESSAGE("Waiting for crc.c source to be linked");
}

void test_crc_table_exchange_size(void) {
    /* CRC over 953-byte exchange table should be non-zero */
    /* unsigned char table[953]; */
    /* memset(table, 0, sizeof(table)); */
    /* TEST_ASSERT_NOT_EQUAL(0, calcul_crc(table, 953)); */
    TEST_IGNORE_MESSAGE("Waiting for crc.c source to be linked");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_crc_empty);
    RUN_TEST(test_crc_known_value);
    RUN_TEST(test_crc_table_exchange_size);
    return UNITY_END();
}
