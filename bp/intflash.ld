/*
 * intflash.ld — GNU ld linker script for MCF52259 (Essensys BP firmware)
 *
 * Converted from CodeWarrior intflash.lcf for the m68k-elf-gcc toolchain.
 * Compatible with the existing Essensys bootloader residing at 0x0420–0x2FFF.
 *
 * Flash map:
 *   0x00000000  Interrupt vector table          (1 KB)
 *   0x00000400  CFM protection registers         (32 B)
 *   0x00000420  Bootloader (preserved, not built here)
 *   0x00003000  APP_CRC | APP_VERSION | APP_JUMP (12 B — bootloader header)
 *   0x0000300C  Application code + constants     (.text, .rodata)
 *   …
 *   0x0007E000  Tb_Echange[] persistence          (4 KB, NOLOAD)
 *   0x0007F000  FlashX spare sector               (4 KB)
 *
 *   0x20000000  SRAM 64 KB  (.data, .bss, stacks, MQX kernel)
 *
 * Bootloader header layout (from Download.h):
 *   Offset 0x00  APP_CRC      unsigned short  CRC-16 of application zone
 *   Offset 0x02  APP_VERSION  unsigned short  firmware version number
 *   Offset 0x04  APP_JUMP     asm jmp __boot  entry point (6–8 bytes)
 *
 * The bootloader calculates CRC-16 over 0x3002–0x7DFFF and compares it to
 * the value stored at 0x3000.  It then jumps to APP_JUMP at 0x3004.
 */

ENTRY(__boot)
EXTERN(_cfm __init_hardware)

MEMORY
{
    vectors     (rx)  : ORIGIN = 0x00000000, LENGTH = 0x00000400
    cfmprotrom  (r)   : ORIGIN = 0x00000400, LENGTH = 0x00000020
    /* 0x00000420 – 0x00002FFF : existing bootloader — intentionally unmapped */
    rom         (rx)  : ORIGIN = 0x00003000, LENGTH = 0x0004B000  /* 0x3000–0x7DFFF */
    persist     (rw)  : ORIGIN = 0x0007E000, LENGTH = 0x00001000  /* 4 KB NOLOAD    */
    sram        (rwx) : ORIGIN = 0x20000000, LENGTH = 0x00010000  /* 64 KB          */
    end_of_kd   (rw)  : ORIGIN = 0x2000FFF0, LENGTH = 0x00000010
    bstack      (rw)  : ORIGIN = 0x2000FA00, LENGTH = 0x00000200  /* 512 B boot stack */
    end_bstack  (rw)  : ORIGIN = 0x2000FC00, LENGTH = 0x00000000
}

SECTIONS
{
    /* ================================================================
     *  MCF52259 platform constants (consumed by MQX BSP)
     * ================================================================ */
    __INTERNAL_SRAM_BASE   = 0x20000000;
    __INTERNAL_SRAM_SIZE   = 0x00010000;
    __INTERNAL_FLASH_BASE  = 0x00000000;
    __INTERNAL_FLASH_SIZE  = 0x00080000;

    __EXTERNAL_MRAM_BASE      = 0x10000000;
    __EXTERNAL_MRAM_SIZE      = 0x00080000;
    __EXTERNAL_MRAM_ROM_BASE  = 0x10000000;
    __EXTERNAL_MRAM_ROM_SIZE  = 0x00000000;
    __EXTERNAL_MRAM_RAM_BASE  = 0x10000000;
    __EXTERNAL_MRAM_RAM_SIZE  = 0x00080000;

    __EXTERNAL_LCD_BASE       = 0x00400000;
    __EXTERNAL_LCD_SIZE       = 0x1FFFF;
    __EXTERNAL_LCD_DC_BASE    = 0x00410000;

    __IPSBAR                  = 0x40000000;
    __VECTOR_TABLE_START      = 0x20000000;

    /* MQX link-time configuration */
    __DEFAULT_PROCESSOR_NUMBER     = 1;
    __DEFAULT_INTERRUPT_STACK_SIZE = 1500;
    __KERNEL_DATA_VERIFY_ENABLE    = 0;
    __FLASHX_SECT_SIZE             = 0x1000;

    /* ================================================================
     *  Interrupt vector table (Flash, 0x00000000)
     * ================================================================ */
    .vectors :
    {
        __vector_table = .;
        __VECTOR_TABLE_ROM_START = .;
        KEEP(*(.vectors_rom))
        . = ALIGN(0x4);
    } > vectors

    /* ================================================================
     *  ColdFire Flash Module protection (0x00000400)
     * ================================================================ */
    .cfmprotect :
    {
        KEEP(*(.cfmconfig))
        . = ALIGN(0x4);
    } > cfmprotrom

    /* ================================================================
     *  Bootloader header — CRITICAL fixed-address sections
     *
     *  The existing bootloader reads these at absolute addresses:
     *    0x3000  CRC-16       (2 B)
     *    0x3002  Version      (2 B)
     *    0x3004  jmp __boot   (≤ 8 B)
     * ================================================================ */
    .app_header :
    {
        KEEP(*(.APP_CRC))
        KEEP(*(.APP_VERSION))
        KEEP(*(.APP_JUMP))
        . = ALIGN(0x10);
    } > rom

    /* ================================================================
     *  Application code (.text + read-only data)
     * ================================================================ */
    .text :
    {
        *(KERNEL)
        *(S_BOOT)
        *(IPSUM)

        *(.text*)
        *(.eh_frame)

        KEEP(*(.init))
        KEEP(*(.fini))

        . = ALIGN(0x4);
        *(.rodata*)
        . = ALIGN(0x4);
        *(.rdata*)
        . = ALIGN(0x4);
        *(.exception)
        . = ALIGN(0x4);
        __exception_table_start__ = .;
        __exception_table_end__   = .;
        __sinit__ = .;
    } > rom

    /* ================================================================
     *  C++ constructor / destructor tables
     * ================================================================ */
    .ctors :
    {
        __CTOR_LIST__ = .;
        KEEP(*crtbegin.o(.ctors))
        KEEP(*(EXCLUDE_FILE(*crtend.o) .ctors))
        KEEP(*(SORT(.ctors.*)))
        KEEP(*(.ctors))
        __CTOR_END__ = .;
    } > rom

    .dtors :
    {
        __DTOR_LIST__ = .;
        KEEP(*crtbegin.o(.dtors))
        KEEP(*(EXCLUDE_FILE(*crtend.o) .dtors))
        KEEP(*(SORT(.dtors.*)))
        KEEP(*(.dtors))
        __DTOR_END__ = .;
    } > rom

    /* ================================================================
     *  Initialized data  (VMA → SRAM,  LMA → Flash)
     *
     *  Startup code copies LMA → VMA using _sidata / _sdata / _edata.
     * ================================================================ */
    _sidata = LOADADDR(.data);

    .data :
    {
        . = ALIGN(128);
        _sdata = .;

        __VECTOR_TABLE_RAM_START = .;
        KEEP(*(.vectors_ram))

        . = ALIGN(512);
        __BDT_BASE = .;
        *(.usb_bdt)
        __BDT_END = .;

        . = ALIGN(0x4);
        __START_DATA = .;
        *(.data*)
        __END_DATA = .;

        . = ALIGN(0x4);
        __START_SDATA = .;
        *(.sdata*)
        __END_SDATA = .;

        . = ALIGN(0x4);
        __SDA_BASE  = .;
        __SDA_BASE_ = __SDA_BASE;

        . = ALIGN(0x10);
        _edata = .;
    } > sram AT> rom

    /* ================================================================
     *  ROM copy table (consumed by MQX __init_data / boot_init_data)
     * ================================================================ */
    .romp :
    {
        __S_romp = .;
        LONG(LOADADDR(.data));
        LONG(_sdata);
        LONG(_edata - _sdata);
        LONG(0);
        LONG(0);
        LONG(0);
    } > rom

    .rom_end_data :
    {
        _rom_data_end = .;
    } > rom

    /* ================================================================
     *  BSS — zero-initialized data in SRAM
     *
     *  Startup code zeroes _sbss → _ebss.
     * ================================================================ */
    .bss (NOLOAD) :
    {
        . = ALIGN(0x10);
        _sbss = .;

        __START_SBSS = .;
        *(.sbss*)
        *(SCOMMON)
        __END_SBSS = .;

        __START_BSS  = .;
        __bss_start__ = __START_BSS;
        *(.bss*)
        *(COMMON)
        __END_BSS   = .;
        __bss_end__ = __END_BSS;

        . = ALIGN(0x10);
        _ebss = .;
    } > sram

    . = ALIGN(0x10);

    /* ================================================================
     *  MQX kernel data (fills remaining SRAM up to 0x2000FFF0)
     * ================================================================ */
    .kernel_data :
    {
        __KERNEL_DATA_START = ALIGN(0x10);
    }

    .end_of_kernel_data :
    {
        __KERNEL_DATA_END = .;
    } > end_of_kd

    /* ================================================================
     *  Boot stack (reused by MQX kernel data after init)
     * ================================================================ */
    .boot_stack :
    {
        _stack_end = .;
    } > bstack

    .end_of_boot_stack :
    {
        _stack_addr        = .;
        _estack            = _stack_addr;
        __SP_INIT          = _stack_addr;
        __BOOT_STACK_ADDRESS = .;
    } > end_bstack

    /* ================================================================
     *  Persistence zone — Tb_Echange[] backup (953 B + CRC-16)
     *  This 4 KB sector is written at runtime; must never contain code.
     * ================================================================ */
    .persist (NOLOAD) :
    {
        KEEP(*(.persist))
    } > persist

    /* ================================================================
     *  FlashX working area (remaining flash after code + data image)
     * ================================================================ */
    _flashx_start       = _rom_data_end;
    __FLASHX_START_ADDR = ((_flashx_start + __FLASHX_SECT_SIZE - 1)
                           / __FLASHX_SECT_SIZE) * __FLASHX_SECT_SIZE;
    __FLASHX_END_ADDR   = __INTERNAL_FLASH_BASE + __INTERNAL_FLASH_SIZE;

    _end = .;

    /* ================================================================
     *  Compile-time sanity checks
     * ================================================================ */
    ASSERT(ADDR(.app_header) == 0x3000,
           "Bootloader header must start at 0x3000 (FLASH_APP_SOFT_START)")

    ASSERT(_rom_data_end <= 0x7E000,
           "Code/data image overflows into persistence zone at 0x7E000")
}
