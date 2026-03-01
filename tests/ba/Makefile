CC      = cc
CFLAGS  = -Wall -Wextra -g -DUNIT_TEST -DBOARD_TYPE=0x12 -I ../stubs -I ../../ba/source -I ../unity/src
UNITY   = ../unity/src/unity.c
BA_SRC  = ../../ba/source

TESTS = test_crc_ba test_gestionentrees test_gestionsorties test_variateur

.PHONY: run_tests clean

run_tests: $(TESTS)
	@failed=0; for t in $(TESTS); do echo "=== $$t ==="; ./$$t || failed=$$((failed+1)); done; \
	if [ $$failed -ne 0 ]; then echo "FAILED: $$failed"; exit 1; else echo "ALL PASSED"; fi

test_crc_ba: test_crc_ba.c $(BA_SRC)/crc.c $(UNITY)
	$(CC) $(CFLAGS) -o $@ $^

test_gestionentrees: test_gestionentrees.c $(BA_SRC)/gestionentrees.c $(UNITY)
	$(CC) $(CFLAGS) -o $@ $^

test_gestionsorties: test_gestionsorties.c $(BA_SRC)/gestionsorties.c $(UNITY)
	$(CC) $(CFLAGS) -o $@ $^

test_variateur: test_variateur.c $(BA_SRC)/variateur.c $(UNITY)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(TESTS)
