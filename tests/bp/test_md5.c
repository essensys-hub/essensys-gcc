#include "unity.h"
#include <string.h>
#include <stdio.h>

/* TODO: #include "Cryptagemd5.h" once source is linked */

/* RFC 1321 Appendix A.5 test vectors */
static const char *md5_rfc_vectors[][2] = {
    { "",               "d41d8cd98f00b204e9800998ecf8427e" },
    { "a",              "0cc175b9c0f1b6a831c399e269772661" },
    { "abc",            "900150983cd24fb0d6963f7d28e17f72" },
    { "message digest", "f96b697d7cb7938d525a2f31aaf161d0" },
};

void setUp(void) {}
void tearDown(void) {}

void test_md5_empty_string(void) {
    /* MD5("") == d41d8cd98f00b204e9800998ecf8427e */
    TEST_IGNORE_MESSAGE("Waiting for Cryptagemd5.c source to be linked");
}

void test_md5_single_char(void) {
    /* MD5("a") == 0cc175b9c0f1b6a831c399e269772661 */
    TEST_IGNORE_MESSAGE("Waiting for Cryptagemd5.c source to be linked");
}

void test_md5_abc(void) {
    /* MD5("abc") == 900150983cd24fb0d6963f7d28e17f72 */
    TEST_IGNORE_MESSAGE("Waiting for Cryptagemd5.c source to be linked");
}

void test_md5_message_digest(void) {
    /* MD5("message digest") == f96b697d7cb7938d525a2f31aaf161d0 */
    TEST_IGNORE_MESSAGE("Waiting for Cryptagemd5.c source to be linked");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_md5_empty_string);
    RUN_TEST(test_md5_single_char);
    RUN_TEST(test_md5_abc);
    RUN_TEST(test_md5_message_digest);
    return UNITY_END();
}
