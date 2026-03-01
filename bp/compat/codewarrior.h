/*
 * codewarrior.h — CodeWarrior → GCC compatibility layer
 *
 * Include automatically via: -include bp/compat/codewarrior.h
 *
 * Translates CodeWarrior-specific constructs (__declspec, compiler
 * detection macros) into GCC equivalents. Guarded by __GNUC__ so
 * the original CodeWarrior build is unaffected.
 *
 * MANUAL SOURCE CHANGES REQUIRED (cannot be handled by this header):
 *
 *   1. #pragma section _XXX begin / end
 *      CodeWarrior uses paired pragmas to place variables in named
 *      sections.  GCC has no pragma equivalent.  Each variable must
 *      be annotated individually:
 *
 *        // Before (CodeWarrior)
 *        #pragma section _APP_JUMP begin
 *        const unsigned long ul_AppJump = 0x12345678;
 *        #pragma section _APP_JUMP end
 *
 *        // After (GCC)
 *        const unsigned long ul_AppJump
 *            __attribute__((section(".APP_JUMP"), used)) = 0x12345678;
 *
 *      Use CW_SECTION() and CW_USED macros defined below.
 *
 *   2. #pragma options align=packed / align=reset
 *      Replace with __attribute__((packed)) on each struct, or wrap
 *      with:
 *        #ifdef __GNUC__
 *        #define PACKED __attribute__((packed))
 *        #else
 *        #pragma options align=packed
 *        #define PACKED
 *        #endif
 *
 *   3. Inline assembly (asm { ... } / asm volatile)
 *      CodeWarrior uses Motorola syntax; GCC uses MIT/gas syntax.
 *      These must be rewritten case-by-case.
 *
 *   4. __declspec(register_abi) / __declspec(standard_abi)
 *      GCC ColdFire does not have an ABI-switching declspec.
 *      Remove these and ensure the whole build uses a consistent ABI
 *      (-mregparm or default).
 */

#ifndef COMPAT_CODEWARRIOR_H
#define COMPAT_CODEWARRIOR_H

#ifdef __GNUC__

/* ------------------------------------------------------------------ */
/*  Section placement helpers                                          */
/* ------------------------------------------------------------------ */

#define CW_SECTION(name)  __attribute__((section(name)))
#define CW_USED           __attribute__((used))

/* ------------------------------------------------------------------ */
/*  __declspec(xxx) → __attribute__((...))                             */
/*                                                                     */
/*  The preprocessor expands __declspec(foo) to CW_DECLSPEC_foo        */
/*  via token-pasting.  Only the variants actually used in the         */
/*  firmware are defined; an unknown declspec will produce a clear     */
/*  compile error ("CW_DECLSPEC_xxx" undeclared).                      */
/* ------------------------------------------------------------------ */

#define __declspec(x)  CW_DECLSPEC_##x

/* ISR marker — ColdFire GCC uses interrupt_handler attribute */
#define CW_DECLSPEC_interrupt  __attribute__((interrupt_handler))

/*
 * Section-placement declspecs used by the BSP (vectors.c, etc.).
 * Each maps to a GCC section attribute matching the linker script.
 */
#define CW_DECLSPEC_far_absolute   /* no-op; use CW_SECTION() explicitly */
#define CW_DECLSPEC_cfmconfig      __attribute__((section(".cfmconfig"), used))
#define CW_DECLSPEC_vectors_ram    __attribute__((section(".vectors_ram"), used))
#define CW_DECLSPEC_vectors_rom    __attribute__((section(".vectors_rom"), used))
#define CW_DECLSPEC_usb_bdt        __attribute__((section(".usb_bdt"), used, aligned(512)))

/* ABI declspecs — no direct GCC equivalent; compile everything with
   the same ABI flag and ignore these. */
#define CW_DECLSPEC_register_abi   /* ignored */
#define CW_DECLSPEC_standard_abi   /* ignored */

/* Weak symbol support */
#define CW_DECLSPEC_weak           __attribute__((weak))

/* Init-code section (MCF54418 boot sequence) */
#define CW_DECLSPEC_init                        __attribute__((section(".init")))
#define CW_DECLSPEC_warm_start                  __attribute__((section(".warm_start")))
#define CW_DECLSPEC_bootstrap_nfcsram_code      __attribute__((section(".bootstrap_nfcsram_code")))
#define CW_DECLSPEC_bootstrap_sram_code         __attribute__((section(".bootstrap_sram_code")))
#define CW_DECLSPEC_bootstrap_nfcsram_vectors   __attribute__((section(".bootstrap_nfcsram_vectors")))
#define CW_DECLSPEC_bootstrap_sram_vectors      __attribute__((section(".bootstrap_sram_vectors")))
#define CW_DECLSPEC_hs_buffer                   __attribute__((section(".hs_buffer"), aligned(4)))

/* ------------------------------------------------------------------ */
/*  Compiler detection                                                 */
/* ------------------------------------------------------------------ */

/* Some MQX/firmware code tests __CODEWARRIOR__ to select code paths.
   Under GCC we leave it undefined and define __GNUC_COMPAT__ instead
   so #ifdef __CODEWARRIOR__ blocks are skipped. */
#undef __CODEWARRIOR__
#define __GNUC_COMPAT__  1

/* ------------------------------------------------------------------ */
/*  EWL → newlib notes                                                 */
/* ------------------------------------------------------------------ */
/*
 * Standard C library functions (sprintf, sscanf, strlen, strcat,
 * memcpy, etc.) are provided by newlib with an identical API.
 * No translation is needed.
 *
 * CW-specific EWL extensions (__ieee_status, etc.) must be replaced
 * or stubbed on a case-by-case basis.
 *
 * Newlib requires minimal syscall stubs for bare-metal targets.
 * See bp/compat/newlib_stubs.c.
 */

#endif /* __GNUC__ */
#endif /* COMPAT_CODEWARRIOR_H */
