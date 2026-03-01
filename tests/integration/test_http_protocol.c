/*
 * Integration test: HTTP/JSON protocol format constraints
 *
 * The firmware produces non-standard JSON and HTTP responses.
 * These immutable constraints must be handled by server/client code.
 * Since BP source (Json.c) is not yet in the repo, this test uses
 * autonomous helper functions that simulate documented firmware behavior.
 */
#include "unity.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

/* ================================================================
 * Helper: parse firmware-style JSON with unquoted keys
 *
 * Firmware format: {k:613,v:"1"}  (keys without quotes)
 * Standard JSON:   {"k":613,"v":"1"}
 *
 * Returns 0 on success, -1 on parse error.
 * Extracts up to max_pairs key-value pairs.
 * ================================================================ */

#define MAX_KEY_LEN   32
#define MAX_VAL_LEN   64
#define MAX_PAIRS     8

typedef struct {
    char key[MAX_KEY_LEN];
    char value[MAX_VAL_LEN];
    int  is_string;
} JsonPair;

static int parse_firmware_json(const char *json, JsonPair *pairs, int max_pairs) {
    const char *p = json;
    int count = 0;

    while (*p && *p != '{') p++;
    if (*p != '{') return -1;
    p++;

    while (*p && *p != '}' && count < max_pairs) {
        while (*p && (isspace((unsigned char)*p) || *p == ',')) p++;
        if (*p == '}' || !*p) break;

        /* Key: unquoted identifier or _-prefixed */
        int ki = 0;
        while (*p && *p != ':' && ki < MAX_KEY_LEN - 1) {
            pairs[count].key[ki++] = *p++;
        }
        pairs[count].key[ki] = '\0';
        if (*p != ':') return -1;
        p++;

        /* Value: quoted string or numeric literal */
        while (*p && isspace((unsigned char)*p)) p++;
        if (*p == '"') {
            p++;
            pairs[count].is_string = 1;
            int vi = 0;
            while (*p && *p != '"' && vi < MAX_VAL_LEN - 1) {
                pairs[count].value[vi++] = *p++;
            }
            pairs[count].value[vi] = '\0';
            if (*p == '"') p++;
        } else {
            pairs[count].is_string = 0;
            int vi = 0;
            while (*p && *p != ',' && *p != '}' && vi < MAX_VAL_LEN - 1) {
                pairs[count].value[vi++] = *p++;
            }
            pairs[count].value[vi] = '\0';
        }
        count++;
    }
    return count;
}

/* ================================================================
 * Helper: validate Content-Type with optional space before semicolon
 *
 * Firmware sends: "application/json ;charset=UTF-8"
 * Standard would be: "application/json; charset=UTF-8"
 * ================================================================ */
static int validate_content_type(const char *ct) {
    const char *semi = strchr(ct, ';');
    if (!semi) return -1;

    /* Extract media type, stripping trailing spaces */
    size_t media_len = (size_t)(semi - ct);
    while (media_len > 0 && ct[media_len - 1] == ' ')
        media_len--;

    if (media_len != 16) return -1;
    if (strncmp(ct, "application/json", 16) != 0) return -1;

    /* Skip semicolon and optional spaces */
    const char *param = semi + 1;
    while (*param == ' ') param++;

    if (strncmp(param, "charset=UTF-8", 13) != 0) return -1;
    return 0;
}

/* ================================================================
 * Helper: check _de67f is the first field in JSON body
 * ================================================================ */
static int is_de67f_first_field(const char *json) {
    const char *brace = strchr(json, '{');
    if (!brace) return 0;
    brace++;
    while (*brace && isspace((unsigned char)*brace)) brace++;
    return (strncmp(brace, "_de67f", 6) == 0);
}

/* ================================================================
 * Helper: extract HTTP status code from response line
 *
 * e.g. "HTTP/1.1 201 Created" → 201
 * ================================================================ */
static int extract_http_status(const char *response_line) {
    const char *p = strstr(response_line, "HTTP/");
    if (!p) return -1;
    while (*p && *p != ' ') p++;
    while (*p == ' ') p++;
    return atoi(p);
}

/* ================================================================ */

void setUp(void) {}
void tearDown(void) {}

/* ---- Test: firmware JSON with unquoted keys is parseable ---- */
void test_json_unquoted_keys(void) {
    const char *fw_json = "{k:613,v:\"1\"}";
    JsonPair pairs[MAX_PAIRS];
    int n = parse_firmware_json(fw_json, pairs, MAX_PAIRS);

    TEST_ASSERT_EQUAL(2, n);
    TEST_ASSERT_EQUAL_STRING("k", pairs[0].key);
    TEST_ASSERT_EQUAL_STRING("613", pairs[0].value);
    TEST_ASSERT_EQUAL(0, pairs[0].is_string);
    TEST_ASSERT_EQUAL_STRING("v", pairs[1].key);
    TEST_ASSERT_EQUAL_STRING("1", pairs[1].value);
    TEST_ASSERT_EQUAL(1, pairs[1].is_string);
}

/* ---- Test: Content-Type with space before semicolon ---- */
void test_content_type_with_space(void) {
    TEST_ASSERT_EQUAL(0,
        validate_content_type("application/json ;charset=UTF-8"));
    TEST_ASSERT_EQUAL(0,
        validate_content_type("application/json; charset=UTF-8"));
    TEST_ASSERT_EQUAL(0,
        validate_content_type("application/json;charset=UTF-8"));
    TEST_ASSERT_NOT_EQUAL(0,
        validate_content_type("text/html;charset=UTF-8"));
}

/* ---- Test: _de67f must be the first JSON field ---- */
void test_de67f_first_field(void) {
    TEST_ASSERT_TRUE(is_de67f_first_field(
        "{_de67f:\"abc\",k:1}"));
    TEST_ASSERT_TRUE(is_de67f_first_field(
        "{ _de67f:\"abc\",k:1}"));
    TEST_ASSERT_FALSE(is_de67f_first_field(
        "{k:1,_de67f:\"abc\"}"));
    TEST_ASSERT_FALSE(is_de67f_first_field(
        "{other:1}"));
}

/* ---- Test: POST response uses HTTP 201, not 200 ---- */
void test_http_201_for_post(void) {
    TEST_ASSERT_EQUAL(201,
        extract_http_status("HTTP/1.1 201 Created"));
    TEST_ASSERT_NOT_EQUAL(200,
        extract_http_status("HTTP/1.1 201 Created"));
    TEST_ASSERT_EQUAL(200,
        extract_http_status("HTTP/1.1 200 OK"));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_json_unquoted_keys);
    RUN_TEST(test_content_type_with_space);
    RUN_TEST(test_de67f_first_field);
    RUN_TEST(test_http_201_for_post);
    return UNITY_END();
}
