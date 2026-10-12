# Tests hote du pilote I2C a attentes bornees du BP (bus et horloge simules).
# Feature : i2c-surge-protection-2026-10-007 -- Refs essensys-hub/essensys-board-SC944D#1
CC      = cc
MQX     = ../../mqx/mqx/source
CFLAGS  = -Wall -Wextra -g -std=gnu99 -DUNIT_TEST \
          -I stubs -I ../../bp/i2c -I ../../bp/app -I ../unity/src \
          -I $(MQX)/io/i2c -I $(MQX)/io/i2c/polled -I $(MQX)/psp/coldfire -I $(MQX)/include
UNITY   = ../unity/src/unity.c
I2CB    = ../../bp/i2c/i2c_bounded.c
DRIVER  = ../../bp/drivers/i2c/i2c_pol_mcf52xx.c

TESTS = test_i2c_bounded test_i2c_recovery

.PHONY: run_tests clean

run_tests: $(TESTS)
	@failed=0; for t in $(TESTS); do echo "=== $$t ==="; ./$$t || failed=$$((failed+1)); done; \
	if [ $$failed -ne 0 ]; then echo "FAILED: $$failed"; exit 1; else echo "ALL PASSED"; fi

test_i2c_bounded: test_i2c_bounded.c sim_i2c.c $(DRIVER) $(I2CB) $(UNITY)
	$(CC) $(CFLAGS) -Wno-unused-parameter -Wno-sign-compare -o $@ $^

test_i2c_recovery: test_i2c_recovery.c sim_i2c.c $(DRIVER) $(I2CB) $(UNITY)
	$(CC) $(CFLAGS) -Wno-unused-parameter -Wno-sign-compare -o $@ $^

clean:
	rm -f $(TESTS)
