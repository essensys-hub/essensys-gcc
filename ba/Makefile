XC8     ?= /opt/microchip/xc8/bin/xc8-cc
MCU      = 16F946
SRCDIR   = source

ifeq ($(BOARD),SC940)
  BOARD_TYPE = TYPE_PIECES_DE_VIE
  OUTPUT     = SC940
else ifeq ($(BOARD),SC941C)
  BOARD_TYPE = TYPE_PIECES_D_EAU
  OUTPUT     = SC941C
else ifeq ($(BOARD),SC942C)
  BOARD_TYPE = TYPE_CHAMBRES
  OUTPUT     = SC942C
else
  $(error BOARD must be SC940, SC941C, or SC942C)
endif

CFLAGS = --chip=$(MCU) -DBOARD_TYPE=$(BOARD_TYPE)
SRCS   = $(wildcard $(SRCDIR)/*.c)

$(OUTPUT).hex: $(SRCS)
	$(XC8) $(CFLAGS) -o $@ $^

.PHONY: all-boards clean
all-boards:
	$(MAKE) BOARD=SC940
	$(MAKE) BOARD=SC941C
	$(MAKE) BOARD=SC942C
clean:
	rm -f *.hex *.o *.lst *.map
