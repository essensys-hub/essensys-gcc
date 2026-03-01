# Makefile — Top-level build for Essensys GCC firmware
#
# Delegates to sub-Makefiles:
#   bp/  — SC944D MCF52259 board (BP_MQX_ETH)
#   ba/  — PIC16F946 boards (SC940, SC941C, SC942C)

.PHONY: all bp ba test clean

all: bp ba

bp:
	$(MAKE) -C bp

ba:
	$(MAKE) -C ba all-boards

test:
	$(MAKE) -C tests/bp run_tests
	$(MAKE) -C tests/ba run_tests
	$(MAKE) -C tests/integration run_tests

clean:
	$(MAKE) -C bp clean
	$(MAKE) -C ba clean
