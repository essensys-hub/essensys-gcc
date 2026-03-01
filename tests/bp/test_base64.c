#include "unity.h"
#include <string.h>

/* TODO: #include "Cryptagecencode.h" once source is linked */

/* RFC 4648 test vectors */
static const char *base64_rfc_vectors[][2] = {
    { "",       "" },
    { "f",      "Zg==" },
    { "fo",     "Zm8=" },
    { "foo",    "Zm9v" },
    { "foob",   "Zm9vYg==" },
    { "foobar", "Zm9vYmFy" },
};

void setUp(void) {}
void tearDown(void) {}

void test_base64_empty(void) {
    /* Base64("") == "" */
    TEST_IGNORE_MESSAGE("Waiting for Cryptagecencode.c source to be linked");
}

void test_base64_f(void) {
    /* Base64("f") == "Zg==" */
    TEST_IGNORE_MESSAGE("Waiting for Cryptagecencode.c source to be linked");
}

void test_base64_foo(void) {
    /* Base64("foo") == "Zm9v" */
    TEST_IGNORE_MESSAGE("Waiting for Cryptagecencode.c source to be linked");
}

void test_base64_foobar(void) {
    /* Base64("foobar") == "Zm9vYmFy" */
    TEST_IGNORE_MESSAGE("Waiting for Cryptagecencode.c source to be linked");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_base64_empty);
    RUN_TEST(test_base64_f);
    RUN_TEST(test_base64_foo);
    RUN_TEST(test_base64_foobar);
    return UNITY_END();
}
