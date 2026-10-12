# Tests hote de la resilience BP <-> BA (commandes, resynchronisation,
# sante, index 12, cycle TE2), horloge simulee.
# Feature : i2c-surge-protection-2026-10-007 -- Refs essensys-hub/essensys-board-SC944D#1
CC      = cc
CFLAGS  = -Wall -Wextra -Werror -g -std=c99 -DUNIT_TEST -I ../../bp/app -I ../unity/src
UNITY   = ../unity/src/unity.c

TESTS = test_ba_resil

.PHONY: run_tests clean

run_tests: $(TESTS)
	@failed=0; for t in $(TESTS); do echo "=== $$t ==="; ./$$t || failed=$$((failed+1)); done; \
	if [ $$failed -ne 0 ]; then echo "FAILED: $$failed"; exit 1; else echo "ALL PASSED"; fi

test_ba_resil: test_ba_resil.c ../../bp/app/ba_resil.c $(UNITY)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(TESTS)
