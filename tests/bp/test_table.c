#include "unity.h"
#include <string.h>
#include <stdint.h>

#include "mqx_stub.h"
/* TODO: #include "TableEchangeAcces.h" once source is linked */

/* Exchange table has 953 indices (0..952) with access rights:
   R = read-only, W = write-only, RW = read-write, RWS = read-write-save */

void setUp(void) {}
void tearDown(void) {}

void test_table_read_valid_index(void) {
    /* Reading index 0 should succeed */
    TEST_IGNORE_MESSAGE("Waiting for TableEchangeAcces.c source to be linked");
}

void test_table_write_valid_index(void) {
    /* Writing a writable index should succeed */
    TEST_IGNORE_MESSAGE("Waiting for TableEchangeAcces.c source to be linked");
}

void test_table_rw_roundtrip(void) {
    /* Write then read on a RW index should return the written value */
    TEST_IGNORE_MESSAGE("Waiting for TableEchangeAcces.c source to be linked");
}

void test_table_out_of_bounds_high(void) {
    /* Index >= 953 should be rejected */
    TEST_IGNORE_MESSAGE("Waiting for TableEchangeAcces.c source to be linked");
}

void test_table_rws_flag(void) {
    /* RWS indices should be marked for persistent save */
    TEST_IGNORE_MESSAGE("Waiting for TableEchangeAcces.c source to be linked");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_table_read_valid_index);
    RUN_TEST(test_table_write_valid_index);
    RUN_TEST(test_table_rw_roundtrip);
    RUN_TEST(test_table_out_of_bounds_high);
    RUN_TEST(test_table_rws_flag);
    return UNITY_END();
}
