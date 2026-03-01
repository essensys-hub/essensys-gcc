/*HEADER**********************************************************************
*
* Copyright 2008 Freescale Semiconductor, Inc.
* Copyright 1989-2008 ARC International
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
*   GCC compiler-specific macros for MQX on ColdFire.
*   Equivalent of cw_comp.h for m68k-elf-gcc.
*
*   Include path must place bp/compat/ before mqx/mqx/source/psp/coldfire/
*   so that psp_comp.h (patched) picks up this header when __GNUC__ is set.
*
*END************************************************************************/
#ifndef __gcc_comp_h__
#define __gcc_comp_h__ 1

#ifdef __cplusplus
extern "C" {
#endif

#define _No_Console     1

/*---------------------------------------------------------------------------
 * Compiler attribute macros
 *
 * These abstract ColdFire-specific GCC attributes so driver and BSP code
 * can remain compiler-neutral.
 *---------------------------------------------------------------------------*/

#define PSP_INTERRUPT       __attribute__((interrupt_handler))
#define PSP_NORETURN        __attribute__((noreturn))
#define PSP_PACKED          __attribute__((packed))
#define PSP_SECTION(s)      __attribute__((section(s)))
#define PSP_USED            __attribute__((used))
#define PSP_ALIGNED(n)      __attribute__((aligned(n)))
#define PSP_WEAK            __attribute__((weak))

/*---------------------------------------------------------------------------
 * Assembly symbol prefix
 *
 * ELF targets (m68k-elf-gcc) do not prepend underscore to C symbols,
 * unlike a.out / CodeWarrior which use _symbol.
 *---------------------------------------------------------------------------*/

#define ASM_PREFIX(x)       x
#define ASM_COMP(x)         x

/*---------------------------------------------------------------------------
 * Inline assembly helpers
 *---------------------------------------------------------------------------*/

#define _ASM_NOP()          __asm__ __volatile__("nop")
#define _ASM_STOP(x)        __asm__ __volatile__("stop %0" : : "n"(x))
#define _ASM_SLEEP(x)       _ASM_STOP(0x2000)

/*---------------------------------------------------------------------------
 * PSP register access — set/get SR
 *---------------------------------------------------------------------------*/

static inline void __psp_set_sr(uint16_t _val)
{
    __asm__ __volatile__("move.w %0,%%sr" : : "d"(_val) : "cc", "memory");
}

static inline uint16_t __psp_get_sr(void)
{
    uint16_t _sr;
    __asm__ __volatile__("move.w %%sr,%0" : "=d"(_sr));
    return _sr;
}

#define _PSP_SET_A6_SP_AND_GOTO(x,y,z) \
    _psp_set_a6_sp_and_goto((uint32_t)(x), (uint32_t)(y), (uint32_t)(z))

#define _PSP_SET_SR(x)              __psp_set_sr((uint16_t)(x))
#define _PSP_SET_DISABLE_SR         _PSP_SET_SR
#define _PSP_SET_ENABLE_SR          _PSP_SET_SR
#define _PSP_GET_SR(x)              (x) = __psp_get_sr()
#define _PSP_SET_CPUCR              _psp_set_cpucr()

/*---------------------------------------------------------------------------
 * User mode access macros
 *---------------------------------------------------------------------------*/

#if MQX_ENABLE_USER_MODE
#error "MQX_ENABLE_USER_MODE not implemented for GCC"
#else

#define KERNEL_ACCESS
#define USER_RW_ACCESS
#define USER_RO_ACCESS
#define USER_NO_ACCESS

#endif /* MQX_ENABLE_USER_MODE */

/*---------------------------------------------------------------------------
 * Weak symbol / function support
 *---------------------------------------------------------------------------*/

#define _WEAK_SYMBOL(x)     __attribute__((weak)) x
#define _WEAK_FUNCTION(x)   __attribute__((weak)) x

/*---------------------------------------------------------------------------
 * Synchronization / pipeline flush
 *---------------------------------------------------------------------------*/

#define _PSP_SYNC()         _ASM_NOP()

/*---------------------------------------------------------------------------
 * Kernel data access
 *---------------------------------------------------------------------------*/

#define _GET_KERNEL_DATA(x) (x) = _mqx_kernel_data
#define _SET_KERNEL_DATA(x) \
   _mqx_kernel_data = (struct kernel_data_struct *)(x)

/*---------------------------------------------------------------------------
 * Read A5 / A6 registers via inline asm
 *---------------------------------------------------------------------------*/

static inline uint32_t __PSP_GET_A5(void)
{
    uint32_t _val;
    __asm__ __volatile__("move.l %%a5,%0" : "=r"(_val));
    return _val;
}
#define _PSP_GET_A5(x)      (x) = __PSP_GET_A5()

static inline void *__PSP_GET_A6(void)
{
    void *_val;
    __asm__ __volatile__("move.l %%a6,%0" : "=r"(_val));
    return _val;
}
#define _PSP_GET_A6(x)      (x) = __PSP_GET_A6()

/*---------------------------------------------------------------------------
 * VBR, CACR — delegate to assembly routines in dispatch.S
 *---------------------------------------------------------------------------*/

#define _PSP_GET_VBR(x)     (x) = (_mqx_max_type)_psp_saved_vbr;
#define _PSP_SET_VBR(x)     _psp_saved_vbr = _psp_set_vbr((uint32_t)(x));

#define _PSP_GET_CACR()     _psp_saved_cacr
#define _PSP_SET_CACR(x)    \
   { \
      uint32_t _xx = (uint32_t)(x); \
      _psp_saved_cacr = _xx & ~(CACR_AUTO_CLEAR_BITS); \
      _psp_set_cacr(_xx); \
   }

/*---------------------------------------------------------------------------
 * ACR registers
 *---------------------------------------------------------------------------*/

#define _PSP_SET_ACR(n, x) \
   { \
      uint32_t _xx = (uint32_t)(x); \
      if ((n) == 0) { \
         _psp_saved_acr0 = _xx; \
      } else if ((n) == 1) { \
         _psp_saved_acr1 = _xx; \
      } else if ((n) == 2) { \
         _psp_saved_acr2 = _xx; \
      } else { \
         _psp_saved_acr3 = _xx; \
      } \
      _psp_set_acr((n), _xx); \
   }

#define _PSP_GET_ACR(n) \
    (((n) == 0) ? _psp_saved_acr0 : \
    (((n) == 1) ? _psp_saved_acr1 : \
    (((n) == 2) ? _psp_saved_acr2 : _psp_saved_acr3)))

/*---------------------------------------------------------------------------
 * IPSBAR
 *---------------------------------------------------------------------------*/

#if (PSP_MQX_CPU_IS_MCF5222X || PSP_MQX_CPU_IS_MCF5223X || PSP_MQX_CPU_IS_MCF5225X || PSP_MQX_CPU_IS_MCF5282)
#define _PSP_SET_IPSBAR(x)  _PSP_SET_MBAR(x)
#define _PSP_GET_IPSBAR()   _PSP_GET_MBAR()
#endif

#if (MQX_CPU == PSP_CPU_MCF5282)
#define _PSP_SET_IPSBAR2(x) _PSP_SET_MBAR2(x)
#define _PSP_GET_IPSBAR2()  _PSP_GET_MBAR2()
#endif

#if (PSP_MQX_CPU_IS_MCF5227X || PSP_MQX_CPU_IS_MCF5301X || PSP_MQX_CPU_IS_MCF5445X || PSP_MQX_CPU_IS_MCF5208 || PSP_MQX_CPU_IS_MCF520X)
#define _PSP_SET_IPSBAR(x) ;
#define _PSP_GET_IPSBAR()  (void *)0xFC000000
#endif

#if (PSP_MQX_CPU_IS_MCF5441X || PSP_MQX_CPU_IS_MCF532X)
#define _PSP_SET_IPSBAR(x) ;
#define _PSP_GET_IPSBAR()  (void *)0xEC000000
#endif

/*---------------------------------------------------------------------------
 * MBAR
 *---------------------------------------------------------------------------*/

#if (PSP_MQX_CPU_IS_MCF5222X || PSP_MQX_CPU_IS_MCF5223X || PSP_MQX_CPU_IS_MCF5225X || MQX_CPU == PSP_CPU_MCF5282)
#define _PSP_SET_MBAR(x)     \
   {                          \
      uint32_t y;              \
      y = (x) & 0xFFFFF000;   \
      *(uint32_t *)_psp_saved_mbar = y | 1; \
      _psp_saved_mbar = y;    \
   }
#else
#define _PSP_SET_MBAR(x)     \
   {                          \
      uint32_t y;              \
      y = (x) & 0xFFFFF000;   \
      _psp_saved_mbar = y;    \
      y |= 0x1;               \
      _psp_set_mbar(y);       \
   }
#endif

#define _PSP_GET_MBAR()   (void *)_psp_saved_mbar

#define _PSP_SET_MBAR2(x)      \
   {                          \
      uint32_t y;              \
      y = (x) & 0xFFFFF000;    \
      _psp_saved_mbar2 = y;    \
      y |= 0x1;               \
      _psp_set_mbar2(y);       \
   }

#define _PSP_GET_MBAR2()  (void *)_psp_saved_mbar2

/*---------------------------------------------------------------------------
 * External assembly routine declarations (implemented in dispatch.S)
 *---------------------------------------------------------------------------*/

extern void     _psp_set_a6_sp_and_goto(uint32_t, uint32_t, uint32_t);
extern uint32_t _psp_set_vbr(uint32_t);
extern void     _psp_set_cacr(uint32_t);
extern void     _psp_set_cpucr(uint32_t);

#ifdef __cplusplus
}
#endif

#endif /* __gcc_comp_h__ */
/* EOF */
