#ifndef __psp_comp_compat_h__
#define __psp_comp_compat_h__

#ifndef __ASM__
#include "psp_cpudef.h"

#if defined(__GNUC__)
#include "gcc_comp.h"
#else
#error "Unexpected compiler in GCC compat layer"
#endif

#define PSP_TASK_PARAM(stack_start_ptr) (stack_start_ptr->PARAMETER)
#endif /* __ASM__ */

#endif
