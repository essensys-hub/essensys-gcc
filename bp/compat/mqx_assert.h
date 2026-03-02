#ifndef __mqx_assert_compat_h__
#define __mqx_assert_compat_h__

#ifdef __cplusplus
extern "C" {
#endif

#define MQX_ASSERT_NONE (0)
#define MQX_ASSERT_TOOLCHAIN (1)
#define MQX_ASSERT_BKPT (2)
#define MQX_ASSERT_BKPT_INL (3)
#define MQX_ASSERT_MSG (4)
#define MQX_ASSERT_LOOP (5)
#define MQX_ASSERT_LOOP_INL (6)

#ifndef MQX_ASSERT
#define MQX_ASSERT MQX_ASSERT_NONE
#endif

#if MQX_ASSERT == MQX_ASSERT_TOOLCHAIN
#ifndef assert
#include <assert.h>
#endif
#else
#if defined(NDEBUG) || (MQX_ASSERT == MQX_ASSERT_NONE)
#undef assert
#define assert(exp)
#else
#undef assert
#define assert(exp)                                                            \
  ((exp) || (_mqx_assert(#exp, __func__, __FILE__, __LINE__), 0))
#endif

#if (MQX_ASSERT == MQX_ASSERT_LOOP_INL || MQX_ASSERT == MQX_ASSERT_BKPT_INL)
inline void _mqx_assert(const char *string, const char *func, const char *file,
                        int line) {
  (void)string;
  (void)func;
  (void)file;
  (void)line;
#ifdef __GNUC__
  __asm__ volatile("move.w #0x2700,%sr\n\tstop #0x2700");
#else
  __asm(move.l #0x2700, d0);
  __asm(move.w d0, sr);
#endif
  while (1) {
  }
}
#else
extern void _mqx_assert(const char *string, const char *func, const char *file,
                        int line);
#endif

#endif
#ifdef __cplusplus
}
#endif

#endif /* __mqx_assert_compat_h__ */
