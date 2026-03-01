CC      = cc
CFLAGS  = -Wall -Wextra -g -DUNIT_TEST -I ../stubs -I ../unity/src
UNITY   = ../unity/src/unity.c

TESTS = test_crc test_json test_md5 test_base64 test_aes test_s19 test_table

.PHONY: run_tests clean

run_tests: $(TESTS)
	@failed=0; for t in $(TESTS); do echo "=== $$t ==="; ./$$t || failed=$$((failed+1)); done; \
	if [ $$failed -ne 0 ]; then echo "FAILED: $$failed"; exit 1; else echo "ALL PASSED"; fi

test_%: test_%.c $(UNITY)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(TESTS)
