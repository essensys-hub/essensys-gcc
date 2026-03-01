/*HEADER**********************************************************************
*
* Copyright 2008 Freescale Semiconductor, Inc.
*
* This software is owned or controlled by Freescale Semiconductor.
* Use of this software is governed by the Freescale MQX RTOS License
* distributed with this Material.
* See the MQX_RTOS_LICENSE file distributed for more details.
*
* Brief License Summary:
* This software is provided in source form for you to use free of charge,
* but it is not open source software. You are allowed to use this software
* but you cannot redistribute it or derivative works of it in source form.
* The software may be used only in connection with a product containing
* a Freescale microprocessor, microcontroller, or digital signal processor.
* See license agreement file for full license terms including other
* restrictions.
*****************************************************************************
*
* Comments:
*
*   GCC-compatible boot code for Essensys SC944D (MCF52259).
*
*   STARTUP FLOW
*   ============
*   Reset
*     │
*     ▼  (vector 0 → SP, vector 1 → PC)
*   _startup()          [boot.S — pure GAS assembly]
*     ├── disable interrupts (SR = 0x2700)
*     ├── set SP from __BOOT_STACK_ADDRESS
*     ├── set VBR → __VECTOR_TABLE_ROM_START
*     ├── init IPSBAR, FLASHBAR (RAMBAR0), RAMBAR1
*     ├── mask INTC0 / INTC1
*     ├── call mcf5225_init()  [bsp_init.c — PLL, watchdog, PHY]
*     ├── copy .data from Flash (LMA) to SRAM (VMA)
*     ├── zero .bss
*     └── call main()
*           └── _mqx()  →  MQX scheduler
*
*   The assembly entry point is in boot.S.
*   mcf5225_init() is in bsp_init.c.
*   main() / _mqx() is in mqx_main.c.
*
*   DIFFERENCES FROM CODEWARRIOR boot.c
*   ====================================
*   - CW "asm void __boot(void) { ... }" inline assembly is replaced by
*     boot.S in GAS syntax.
*   - CW "#pragma section" and "__declspec" are replaced by
*     __attribute__((section(...))) via gcc_comp.h / codewarrior.h.
*   - .data copy and .bss zero are done explicitly in boot.S;
*     CW relied on its own __startup runtime for this.
*   - ELF symbols do not have underscore prefix (ASM_PREFIX is identity).
*
*END************************************************************************/

#include "mqx.h"
#include "bsp.h"

/*
 * Linker-script symbols — declared as arrays so the compiler generates
 * address references rather than load instructions.
 */
extern unsigned long __BOOT_STACK_ADDRESS[];

/*
 * Assembly entry points defined in boot.S.
 * __boot / _startup : primary reset entry
 * __boot_exception  : catch-all halt for unexpected exceptions during boot
 */
extern void __boot(void)           __attribute__((noreturn));
extern void _startup(void)         __attribute__((noreturn));
extern void __boot_exception(void);

/* EOF */
