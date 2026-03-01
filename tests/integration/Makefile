CC       = cc
CFLAGS   = -Wall -Wextra -g -DUNIT_TEST -DBOARD_TYPE=0x12 -funsigned-char \
           -I ../../ba/source -I ../stubs -I ../unity/src
UNITY    = ../unity/src/unity.c
BA_SRC   = ../../ba/source

TESTS = test_i2c_bp_ba test_http_protocol test_ota_s19

.PHONY: run_tests clean

run_tests: $(TESTS)
	@failed=0; for t in $(TESTS); do echo "=== $$t ==="; ./$$t || failed=$$((failed+1)); done; \
	if [ $$failed -ne 0 ]; then echo "FAILED: $$failed"; exit 1; else echo "ALL PASSED"; fi

test_i2c_bp_ba: test_i2c_bp_ba.c $(BA_SRC)/slavenode.c $(BA_SRC)/crc.c $(UNITY)
	$(CC) $(CFLAGS) -o $@ $^

test_http_protocol: test_http_protocol.c $(UNITY)
	$(CC) $(CFLAGS) -o $@ $^

test_ota_s19: test_ota_s19.c $(UNITY)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(TESTS)
