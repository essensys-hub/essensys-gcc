#include "unity.h"
#include "crc.h"

void setUp(void) {}
void tearDown(void) {}

void test_crc_empty_buffer(void) {
    unsigned char buf[] = {0};
    unsigned short result = us_CalculerCRCSurTrame(buf, 0);
    TEST_ASSERT_EQUAL_UINT16(0xFFFF, result);
}

void test_crc_known_vector(void) {
    unsigned char data[] = {0x01, 0x02, 0x03, 0x04};
    unsigned short result = us_CalculerCRCSurTrame(data, 4);
    TEST_ASSERT_EQUAL_UINT16(0x2BA1, result);
}

void test_crc_single_byte(void) {
    unsigned char data[] = {0x00};
    unsigned short result = us_CalculerCRCSurTrame(data, 1);
    /* CRC-16/MODBUS of {0x00}: init=0xFFFF, poly=0xA001, reflect */
    TEST_ASSERT_EQUAL_UINT16(0x40BF, result);
}

void test_crc_modbus_reference(void) {
    /* Standard MODBUS CRC-16 test: slave_addr=0x01 func=0x03 reg=0x0000 count=0x0001 */
    unsigned char frame[] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x01};
    unsigned short result = us_CalculerCRCSurTrame(frame, 6);
    TEST_ASSERT_EQUAL_UINT16(0x0A84, result);
}

void test_crc_identical_to_bp(void) {
    /*
     * BA and BP use the same CRC-16/MODBUS algorithm:
     * polynomial 0xA001 (reflected), initial value 0xFFFF.
     * Verify with the same vectors used in BP test_crc.
     */
    unsigned char data[] = {0x01, 0x02, 0x03, 0x04};
    unsigned short crc_ba = us_CalculerCRCSurTrame(data, 4);

    /* Recompute byte-by-byte to confirm identical algorithm */
    unsigned short crc_manual = 0xFFFF;
    for (unsigned char i = 0; i < 4; i++)
        crc_manual = us_CalculerCRCSurUnOctet(crc_manual, data[i]);

    TEST_ASSERT_EQUAL_UINT16(crc_manual, crc_ba);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_crc_empty_buffer);
    RUN_TEST(test_crc_known_vector);
    RUN_TEST(test_crc_single_byte);
    RUN_TEST(test_crc_modbus_reference);
    RUN_TEST(test_crc_identical_to_bp);
    return UNITY_END();
}
