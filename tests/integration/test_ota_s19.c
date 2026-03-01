/*
 * Integration test: OTA circular S-Record encode/decode
 *
 * Circular pipeline:
 *   1. Create a 256-byte test binary block
 *   2. Encode it as S3 (4-byte address) S-Record lines
 *   3. Parse the S3 lines back to binary
 *   4. Compare parsed data with the original block
 *   5. Verify all S19 checksums
 *
 * Since Download.c (BP source) is not yet in the repo, this test uses
 * autonomous S-Record encoder/parser helpers.
 */
#include "unity.h"
#include <string.h>
#include <stdint.h>
#include <stdio.h>

#define TEST_BLOCK_SIZE    256
#define BYTES_PER_LINE     16
#define S3_LINES           (TEST_BLOCK_SIZE / BYTES_PER_LINE)
#define MAX_SREC_LINE      80

/* ================================================================
 * S-Record helper functions
 *
 * S3 line format:
 *   S3 <byte_count_hex> <addr_4bytes_hex> <data_hex> <checksum_hex>
 *
 * byte_count = 4 (addr) + data_len + 1 (checksum)
 * checksum   = one's complement of (sum of byte_count + addr bytes + data bytes)
 * ================================================================ */

static int hex_nibble(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

static uint8_t hex_byte(const char *s) {
    return (uint8_t)((hex_nibble(s[0]) << 4) | hex_nibble(s[1]));
}

static void byte_to_hex(uint8_t b, char *out) {
    static const char hex[] = "0123456789ABCDEF";
    out[0] = hex[(b >> 4) & 0x0F];
    out[1] = hex[b & 0x0F];
}

/*
 * Encode one S3 line.
 * addr: 32-bit start address for this chunk
 * data: pointer to binary data
 * data_len: number of data bytes (max ~32 per line)
 * out: output buffer (must be at least MAX_SREC_LINE chars)
 * Returns: length of the generated line (excluding NUL terminator)
 */
static int s3_encode_line(uint32_t addr, const uint8_t *data,
                          uint8_t data_len, char *out) {
    uint8_t byte_count = 4 + data_len + 1;
    uint8_t sum = byte_count;
    int pos = 0;

    out[pos++] = 'S'; out[pos++] = '3';

    byte_to_hex(byte_count, out + pos); pos += 2;

    uint8_t addr_bytes[4] = {
        (addr >> 24) & 0xFF, (addr >> 16) & 0xFF,
        (addr >> 8) & 0xFF, addr & 0xFF
    };
    for (int i = 0; i < 4; i++) {
        byte_to_hex(addr_bytes[i], out + pos); pos += 2;
        sum += addr_bytes[i];
    }

    for (uint8_t i = 0; i < data_len; i++) {
        byte_to_hex(data[i], out + pos); pos += 2;
        sum += data[i];
    }

    uint8_t checksum = ~sum;
    byte_to_hex(checksum, out + pos); pos += 2;

    out[pos] = '\0';
    return pos;
}

/*
 * Parse one S3 line.
 * line: NUL-terminated S-Record line (must start with "S3")
 * addr_out: receives the 32-bit address
 * data_out: receives the parsed data bytes
 * data_len_out: receives the number of data bytes
 * Returns: 0 on success, -1 on format error, -2 on checksum error
 */
static int s3_parse_line(const char *line, uint32_t *addr_out,
                         uint8_t *data_out, uint8_t *data_len_out) {
    if (line[0] != 'S' || line[1] != '3') return -1;

    uint8_t byte_count = hex_byte(line + 2);
    uint8_t sum = byte_count;

    const char *p = line + 4;

    /* 4 address bytes */
    uint32_t addr = 0;
    for (int i = 0; i < 4; i++) {
        uint8_t b = hex_byte(p); p += 2;
        addr = (addr << 8) | b;
        sum += b;
    }
    *addr_out = addr;

    /* Data bytes: byte_count - 4 (addr) - 1 (checksum) */
    uint8_t dlen = byte_count - 4 - 1;
    *data_len_out = dlen;
    for (uint8_t i = 0; i < dlen; i++) {
        data_out[i] = hex_byte(p); p += 2;
        sum += data_out[i];
    }

    /* Checksum byte */
    uint8_t file_cksum = hex_byte(p);
    sum += file_cksum;
    if ((uint8_t)(sum) != 0xFF) return -2;

    return 0;
}

/*
 * Verify checksum of a raw S-Record line (any type).
 * Returns 0 if valid, -1 on error.
 */
static int s19_verify_checksum(const char *line) {
    if (line[0] != 'S') return -1;
    uint8_t byte_count = hex_byte(line + 2);
    uint8_t sum = 0;
    const char *p = line + 2;
    for (uint8_t i = 0; i <= byte_count; i++) {
        sum += hex_byte(p); p += 2;
    }
    return (sum == 0xFF) ? 0 : -1;
}

/* ================================================================ */

static uint8_t test_block[TEST_BLOCK_SIZE];
static char    srec_lines[S3_LINES][MAX_SREC_LINE];

void setUp(void) {
    for (int i = 0; i < TEST_BLOCK_SIZE; i++)
        test_block[i] = (uint8_t)(i & 0xFF);
}

void tearDown(void) {}

/* ---- Step 1+2: Encode the test block into S3 lines ---- */
static void encode_block(void) {
    for (int i = 0; i < S3_LINES; i++) {
        uint32_t addr = 0x08000000 + (uint32_t)(i * BYTES_PER_LINE);
        s3_encode_line(addr, test_block + i * BYTES_PER_LINE,
                       BYTES_PER_LINE, srec_lines[i]);
    }
}

/* ---- Test: circular encode → parse → compare ---- */
void test_ota_circular(void) {
    encode_block();

    uint8_t reconstructed[TEST_BLOCK_SIZE];
    memset(reconstructed, 0, sizeof(reconstructed));

    for (int i = 0; i < S3_LINES; i++) {
        uint32_t addr;
        uint8_t data[32];
        uint8_t dlen;
        int rc = s3_parse_line(srec_lines[i], &addr, data, &dlen);

        TEST_ASSERT_EQUAL_INT_MESSAGE(0, rc, srec_lines[i]);
        TEST_ASSERT_EQUAL_UINT8(BYTES_PER_LINE, dlen);
        TEST_ASSERT_EQUAL_HEX32(0x08000000 + (uint32_t)(i * BYTES_PER_LINE), addr);

        memcpy(reconstructed + i * BYTES_PER_LINE, data, dlen);
    }

    TEST_ASSERT_EQUAL_MEMORY(test_block, reconstructed, TEST_BLOCK_SIZE);
}

/* ---- Test: all generated S3 lines have valid checksums ---- */
void test_ota_checksums(void) {
    encode_block();

    for (int i = 0; i < S3_LINES; i++) {
        TEST_ASSERT_EQUAL_INT_MESSAGE(
            0, s19_verify_checksum(srec_lines[i]), srec_lines[i]);
    }
}

/* ---- Test: corrupted checksum is detected ---- */
void test_ota_corrupted_checksum(void) {
    encode_block();

    /* Corrupt last two hex chars (checksum) of first line */
    char corrupted[MAX_SREC_LINE];
    strcpy(corrupted, srec_lines[0]);
    size_t len = strlen(corrupted);
    corrupted[len - 2] = '0';
    corrupted[len - 1] = '0';

    TEST_ASSERT_NOT_EQUAL(0, s19_verify_checksum(corrupted));

    uint32_t addr;
    uint8_t data[32];
    uint8_t dlen;
    int rc = s3_parse_line(corrupted, &addr, data, &dlen);
    TEST_ASSERT_EQUAL_INT(-2, rc);
}

/* ---- Test: parse rejects non-S3 record types ---- */
void test_ota_reject_non_s3(void) {
    uint32_t addr;
    uint8_t data[32];
    uint8_t dlen;

    TEST_ASSERT_EQUAL_INT(-1,
        s3_parse_line("S0030000FC", &addr, data, &dlen));
    TEST_ASSERT_EQUAL_INT(-1,
        s3_parse_line("S70500000000FA", &addr, data, &dlen));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_ota_circular);
    RUN_TEST(test_ota_checksums);
    RUN_TEST(test_ota_corrupted_checksum);
    RUN_TEST(test_ota_reject_non_s3);
    return UNITY_END();
}
