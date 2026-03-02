#include <bsp.h>
#include <mqx.h>

#if (MQX_ASSERT == MQX_ASSERT_LOOP || MQX_ASSERT == MQX_ASSERT_BKPT)
void _mqx_assert(const char *string, const char *func, const char *file,
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
#endif

#if MQX_ASSERT == MQX_ASSERT_MSG
void _mqx_assert(const char *string, const char *func, const char *file,
                 int line) {
  printf("%s, %s, %s, %d \n\r", string, func, file, line);
#ifdef __GNUC__
  __asm__ volatile("move.w #0x2700,%sr\n\tstop #0x2700");
#else
  __asm(move.l #0x2700, d0);
  __asm(move.w d0, sr);
#endif
  while (1) {
  }
}
#endif
