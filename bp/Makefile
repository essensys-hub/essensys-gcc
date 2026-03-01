# bp/Makefile — Build SC944D (MCF52259) BP firmware with m68k-elf-gcc
#
# Produces:
#   psp.a          — MQX PSP ColdFire + Kernel
#   bsp.a          — BSP m52259evb + IO drivers + compat stubs
#   rtcs.a         — RTCS TCP/IP stack
#   BP_MQX_ETH.elf — Linked firmware
#   BP_MQX_ETH.s19 — Motorola S-record (S3, 32 bytes/line)

# ---------------------------------------------------------------------------
# Toolchain
# ---------------------------------------------------------------------------
CROSS   ?= m68k-elf-
CC       = $(CROSS)gcc
AS       = $(CROSS)gcc
AR       = $(CROSS)ar
OBJCOPY  = $(CROSS)objcopy
SIZE     = $(CROSS)size

# ---------------------------------------------------------------------------
# Directories (relative to bp/)
# ---------------------------------------------------------------------------
MQX_SRC  = ../mqx/mqx/source
RTCS_SRC = ../mqx/rtcs/source
IO       = $(MQX_SRC)/io
BSP_DIR  = bsp/m52259evb
BUILDDIR = build
TARGET   = BP_MQX_ETH

# ---------------------------------------------------------------------------
# Flags
# ---------------------------------------------------------------------------
CFLAGS   = -mcpu=52259 -msoft-float -ffreestanding -Os -g3 \
           -ffunction-sections -fdata-sections -fno-common -Wall -Wextra \
           -include compat/codewarrior.h

ASFLAGS  = -mcpu=52259 -msoft-float -ffreestanding -g3

LDFLAGS  = -mcpu=52259 -nostdlib -Wl,-T,intflash.ld -Wl,--gc-sections \
           -Wl,-Map,$(TARGET).map

# ---------------------------------------------------------------------------
# Include paths
# ---------------------------------------------------------------------------
INCLUDES = \
  -I $(MQX_SRC)/include \
  -I $(MQX_SRC)/psp/coldfire \
  -I $(BSP_DIR) \
  -I $(IO)/serial \
  -I $(IO)/i2c \
  -I $(IO)/spi_legacy \
  -I $(IO)/gpio \
  -I $(IO)/lwgpio \
  -I $(IO)/adc \
  -I $(IO)/flashx \
  -I $(IO)/enet \
  -I $(IO)/enet/phy \
  -I $(IO)/enet/mcf5xxx_fec \
  -I $(IO)/rtc \
  -I $(IO)/int_ctrl \
  -I $(IO)/timer \
  -I $(IO)/pcb \
  -I $(IO)/io_mem \
  -I $(IO)/io_null \
  -I $(RTCS_SRC)/include \
  -I compat

CFLAGS  += $(INCLUDES) -MMD -MP
ASFLAGS += $(INCLUDES) -MMD -MP

# ===========================================================================
#  Source file lists
# ===========================================================================

# --- PSP ColdFire (exclude chip variants not for MCF52259) -----------------
PSP_EXCLUDE = mcf51jf.c mcf5441.c
PSP_C_SRC   = $(filter-out $(addprefix $(MQX_SRC)/psp/coldfire/,$(PSP_EXCLUDE)), \
                $(wildcard $(MQX_SRC)/psp/coldfire/*.c))
PSP_S_SRC   = $(wildcard $(MQX_SRC)/psp/coldfire/*.S)

# --- MQX Kernel -----------------------------------------------------------
KERN_C_SRC  = $(wildcard $(MQX_SRC)/kernel/*.c)

# --- BSP m52259evb ---------------------------------------------------------
BSP_C_SRC   = $(wildcard $(BSP_DIR)/*.c)

# --- Compat layer ----------------------------------------------------------
COMPAT_SRC  = compat/newlib_stubs.c

# --- IO drivers (MCF5225x / MCF52xx variants only) -------------------------
IO_C_SRC = \
  $(wildcard $(IO)/io_*.c) \
  $(IO)/serial/io_sinit.c \
  $(IO)/serial/serl_mcf5225.c \
  $(IO)/serial/int/serl_int.c \
  $(IO)/serial/int/serl_int_mcf52xx.c \
  $(IO)/serial/polled/serl_pol.c \
  $(IO)/serial/polled/serl_pol_mcf52xx.c \
  $(IO)/i2c/i2c_mcf5225.c \
  $(IO)/i2c/polled/i2c_pol.c \
  $(IO)/i2c/polled/i2c_pol_mcf52xx.c \
  $(IO)/i2c/int/i2c_int.c \
  $(IO)/i2c/int/i2c_int_mcf52xx.c \
  $(IO)/spi_legacy/spi_mcf5225_qspi.c \
  $(IO)/spi_legacy/polled/spi_pol.c \
  $(IO)/spi_legacy/polled/spi_pol_mcf5xxx_qspi.c \
  $(IO)/spi_legacy/int/spi_int.c \
  $(IO)/spi_legacy/int/spi_int_mcf5xxx_qspi.c \
  $(IO)/gpio/io_gpio.c \
  $(IO)/gpio/mcf5225/io_gpio_mcf5225.c \
  $(IO)/lwgpio/lwgpio.c \
  $(IO)/lwgpio/lwgpio_mcf5225.c \
  $(IO)/adc/adc.c \
  $(IO)/adc/mcf522xx/adc_mcf522xx.c \
  $(IO)/adc/mcf522xx/adt_pit_mcf522xx.c \
  $(IO)/flashx/flashx.c \
  $(IO)/flashx/freescale/flash_mcf52xx.c \
  $(IO)/flashx/freescale/flash_mcf5225.c \
  $(IO)/enet/enaddr.c \
  $(IO)/enet/enclose.c \
  $(IO)/enet/enerr.c \
  $(IO)/enet/eninit.c \
  $(IO)/enet/enjoin.c \
  $(IO)/enet/enleave.c \
  $(IO)/enet/enmediactl.c \
  $(IO)/enet/enopen.c \
  $(IO)/enet/enrecv.c \
  $(IO)/enet/ensend.c \
  $(IO)/enet/enstat.c \
  $(IO)/enet/enstop.c \
  $(IO)/enet/enutil.c \
  $(IO)/enet/mcf5xxx_fec/mcf5225_fec.c \
  $(IO)/enet/mcf5xxx_fec/mcf5xxx_fec_init.c \
  $(IO)/enet/mcf5xxx_fec/mcf5xxx_fec_join.c \
  $(IO)/enet/mcf5xxx_fec/mcf5xxx_fec_mii.c \
  $(IO)/enet/mcf5xxx_fec/mcf5xxx_fec_receive.c \
  $(IO)/enet/mcf5xxx_fec/mcf5xxx_fec_rejoin.c \
  $(IO)/enet/mcf5xxx_fec/mcf5xxx_fec_send.c \
  $(IO)/enet/mcf5xxx_fec/mcf5xxx_fec_stop.c \
  $(IO)/enet/mcf5xxx_fec/mcf5xxx_fec_util.c \
  $(IO)/enet/phy/phy_dp83xxx.c \
  $(IO)/int_ctrl/int_ctrl_mcf5225.c \
  $(IO)/rtc/rtc_mcf5225.c \
  $(IO)/rtc/rtc_mcf52xx.c \
  $(IO)/timer/timer_mcf5225.c \
  $(IO)/pcb/io_pcb.c \
  $(IO)/pcb/io_pcb2.c \
  $(IO)/pcb/mqxa/pcb_mqxa.c \
  $(IO)/io_mem/io_mem.c \
  $(IO)/io_null/io_null.c

# --- RTCS TCP/IP stack -----------------------------------------------------
RTCS_C_SRC = \
  $(wildcard $(RTCS_SRC)/apps/*.c) \
  $(wildcard $(RTCS_SRC)/if/*.c) \
  $(wildcard $(RTCS_SRC)/media/*.c) \
  $(wildcard $(RTCS_SRC)/nat/*.c) \
  $(wildcard $(RTCS_SRC)/os/mqx/*.c) \
  $(wildcard $(RTCS_SRC)/ppp/*.c) \
  $(wildcard $(RTCS_SRC)/snmp/*.c) \
  $(wildcard $(RTCS_SRC)/tcpip/*.c)

# --- Application (empty — sources will be added later) ---------------------
APP_SRC  =
APP_OBJS = $(addprefix $(BUILDDIR)/, $(APP_SRC:.c=.o))

# ===========================================================================
#  Object file lists
# ===========================================================================

# MQX sources (../) → build/mqx/...; local sources → build/<path>
PSP_OBJS  = $(patsubst ../%, $(BUILDDIR)/%, $(PSP_C_SRC:.c=.o)) \
            $(patsubst ../%, $(BUILDDIR)/%, $(PSP_S_SRC:.S=.o)) \
            $(patsubst ../%, $(BUILDDIR)/%, $(KERN_C_SRC:.c=.o))

BSP_OBJS  = $(addprefix $(BUILDDIR)/, $(BSP_C_SRC:.c=.o)) \
            $(addprefix $(BUILDDIR)/, $(COMPAT_SRC:.c=.o)) \
            $(patsubst ../%, $(BUILDDIR)/%, $(IO_C_SRC:.c=.o))

RTCS_OBJS = $(patsubst ../%, $(BUILDDIR)/%, $(RTCS_C_SRC:.c=.o))

ALL_OBJS  = $(PSP_OBJS) $(BSP_OBJS) $(RTCS_OBJS) $(APP_OBJS)

# ===========================================================================
#  Build targets
# ===========================================================================

.PHONY: all size clean

all: $(TARGET).elf $(TARGET).s19 size

# --- Static libraries ------------------------------------------------------

psp.a: $(PSP_OBJS)
	$(AR) rcs $@ $^

bsp.a: $(BSP_OBJS)
	$(AR) rcs $@ $^

rtcs.a: $(RTCS_OBJS)
	$(AR) rcs $@ $^

# --- Final link -------------------------------------------------------------

$(TARGET).elf: psp.a bsp.a rtcs.a $(APP_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(APP_OBJS) \
	  -Wl,--start-group psp.a bsp.a rtcs.a -lgcc -Wl,--end-group

# --- S-record output --------------------------------------------------------

$(TARGET).s19: $(TARGET).elf
	$(OBJCOPY) -O srec --srec-forceS3 --srec-len=32 $< $@

# --- Size report ------------------------------------------------------------

size: $(TARGET).elf
	$(SIZE) $<

# --- Clean ------------------------------------------------------------------

clean:
	rm -rf $(BUILDDIR) *.a *.elf *.s19 *.map

# ===========================================================================
#  Compilation rules
# ===========================================================================

# MQX sources (paths starting with ../)
$(BUILDDIR)/%.o: ../%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/%.o: ../%.S
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) -c $< -o $@

# Local sources (BSP, compat, app)
$(BUILDDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# ===========================================================================
#  Auto-generated dependencies
# ===========================================================================
-include $(ALL_OBJS:.o=.d)
