# ba/Makefile -- Build des BA (PIC16F946) avec XC8, un .hex par type de carte.
#
# Feature i2c-surge-protection-2026-10-007 (tache 5.10) : une seule macro
# TYPE_* par cible (hard.h refuse zero ou plusieurs macros), et un repertoire
# de build par carte pour que les fichiers intermediaires ne se melangent pas.
#
#   make BOARD=SC940    -> SC940.hex   (TYPE_PIECES_DE_VIE, adresse 0x11)
#   make BOARD=SC942C   -> SC942C.hex  (TYPE_CHAMBRES,      adresse 0x12)
#   make BOARD=SC941C   -> SC941C.hex  (TYPE_PIECES_D_EAU,  adresse 0x13)
#   make all-boards     -> les trois
#
# Chaque BA ne doit etre programme qu'avec le .hex de son type.

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
else ifneq ($(BOARD),)
  # Only error on an explicitly-invalid BOARD. Empty BOARD is allowed so that
  # `clean` and `all-boards` (which set BOARD per sub-make) parse without error.
  $(error BOARD must be SC940, SC941C, or SC942C)
endif

BUILDDIR = build/$(OUTPUT)
CFLAGS   = -mcpu=$(MCU) -D$(BOARD_TYPE)
SRCS     = $(wildcard $(SRCDIR)/*.c)

$(OUTPUT).hex: $(SRCS) $(wildcard $(SRCDIR)/*.h)
	@mkdir -p $(BUILDDIR)
	cd $(BUILDDIR) && $(XC8) $(CFLAGS) -o $(OUTPUT).hex $(addprefix ../../,$(SRCS))
	cp $(BUILDDIR)/$(OUTPUT).hex $@

.PHONY: all-boards clean check-board-types
all-boards:
	$(MAKE) BOARD=SC940
	$(MAKE) BOARD=SC941C
	$(MAKE) BOARD=SC942C

# Verification hote (sans XC8) : chaque cible ne definit qu'une macro TYPE_*
# et obtient la bonne adresse I2C ; zero ou deux macros font echouer hard.h.
check-board-types:
	@../tests/ba/check_board_types.sh

clean:
	rm -rf build *.hex *.o *.lst *.map
