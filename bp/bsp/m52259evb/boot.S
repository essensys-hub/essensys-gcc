/*
 * boot.S — MCF52259 GCC/GAS startup for Essensys SC944D
 *
 * Startup flow:
 *   Reset vector → __boot → init MCU regs → mcf5225_init()
 *         → __copy_data → __zero_bss → main() → _mqx()
 *
 * Assembled with:  m68k-elf-gcc -mcpu=52259 -Wa,--register-prefix-optional
 *
 * Linker symbols required (from linker script, see bp/intflash.ld):
 *   __BOOT_STACK_ADDRESS   Initial stack pointer (top of SRAM)
 *   __VECTOR_TABLE_ROM_START  Base of vector table in Flash
 *   __IPSBAR               Peripheral base address
 *   __INTERNAL_FLASH_BASE  Internal Flash start
 *   __INTERNAL_SRAM_BASE   Internal SRAM start
 *   _sidata                .data section LMA (load address in Flash)
 *   _sdata / _edata        .data section VMA start/end (destination in RAM)
 *   _sbss / _ebss          .bss start/end (zero-initialized in RAM)
 * (intflash.ld's own comments document these as the intended startup
 *  contract: "Startup code copies LMA -> VMA using _sidata / _sdata /
 *  _edata" and "Startup code zeroes _sbss -> _ebss". Note _sdata/_edata
 *  span the whole .data output section (vectors_ram, usb_bdt, .data*,
 *  .sdata*) — not just the __START_DATA/__END_DATA sub-range, which
 *  would leave part of initialized data uncopied.)
 */

    .section .boot, "ax"
    .global __boot
    .global __boot_exception
    .global _startup

    .extern mcf5225_init
    .extern main
    .extern __BOOT_STACK_ADDRESS
    .extern __VECTOR_TABLE_ROM_START
    .extern __IPSBAR
    .extern __INTERNAL_FLASH_BASE
    .extern __INTERNAL_SRAM_BASE
    .extern _sidata
    .extern _sdata
    .extern _edata
    .extern _sbss
    .extern _ebss

    /* ColdFire MOVEC control-register selectors.
     * GAS's m68k register-name table only recognizes RAMBAR0/RAMBAR1
     * mnemonics for classic 68040/68060 CPUs, not for ColdFire targets;
     * on ColdFire these must be written as movec Rn,#<selector>. Values
     * match the Freescale numeric constants (see
     * mqx/source/psp/coldfire/types.inc, __ACF__ block: RAMBAR0=0xC04,
     * RAMBAR1=0xC05). */
    .equ    CF_RAMBAR0, 0xC04
    .equ    CF_RAMBAR1, 0xC05

/*
 * _startup / __boot — primary entry point after reset.
 *
 * At this point the CPU is in supervisor mode, interrupts are
 * disabled by the reset exception, and only the default VBR (0x0)
 * and stack pointer (from vector 0) are valid.
 */
    .balign 4
_startup:
__boot:
    move.w  #0x2700, sr                 /* disable all interrupts            */
    move.l  #__BOOT_STACK_ADDRESS, sp   /* establish initial stack            */

    /* VBR — point to our vector table in Flash */
    move.l  #__VECTOR_TABLE_ROM_START, d0
    movec   d0, vbr

    /* IPSBAR — peripheral base at 0x40000000 (MCF52259 default) */
    move.l  #__IPSBAR, d0
    andi.l  #0xC0000000, d0
    addi.l  #0x1, d0
    move.l  d0, 0x40000000

    /* FLASHBAR (RAMBAR0) — validate internal Flash for code+data access */
    move.l  #__INTERNAL_FLASH_BASE, d0
    andi.l  #0xFFF80000, d0
    addi.l  #0x61, d0
    movec   d0, #CF_RAMBAR0

    /* RAMBAR1 — validate internal SRAM for code+data access */
    move.l  #__INTERNAL_SRAM_BASE, d0
    andi.l  #0xFFF80000, d0
    addi.l  #0x21, d0
    movec   d0, #CF_RAMBAR1

    /* Mask all interrupts in INTC0 and INTC1 */
    movea.l #__IPSBAR, a0
    moveq   #1, d0
    move.l  d0, 0x0C0C(a0)             /* INTC0_IMRL = 1 (mask all)         */
    movea.l #__IPSBAR, a0
    moveq   #1, d0
    move.l  d0, 0x0D0C(a0)             /* INTC1_IMRL = 1 (mask all)         */

    /* Board-level init (PLL, watchdog, PHY) — called before C runtime */
    jsr     mcf5225_init

    /* Copy .data section from Flash (LMA) to SRAM (VMA) */
    lea     _sidata, a0                 /* source (Flash)                    */
    lea     _sdata, a1                  /* destination (SRAM)                */
    lea     _edata, a2                  /* end marker                        */
    bra.s   .Lcopy_check
.Lcopy_loop:
    move.l  (a0)+, (a1)+
.Lcopy_check:
    cmpa.l  a1, a2
    bhi.s   .Lcopy_loop

    /* Zero .bss section */
    lea     _sbss, a0
    lea     _ebss, a1
    moveq   #0, d0
    bra.s   .Lzero_check
.Lzero_loop:
    move.l  d0, (a0)+
.Lzero_check:
    cmpa.l  a0, a1
    bhi.s   .Lzero_loop

    /* Enter C — main() calls _mqx() which starts the MQX scheduler */
    jsr     main

    /* Should never return; halt if it does */
    halt

/*
 * __boot_exception — catch-all for unexpected exceptions during boot.
 */
    .balign 4
__boot_exception:
    halt
    rte
