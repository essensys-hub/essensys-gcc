/*
 * newlib_stubs.c — Minimal syscall stubs for newlib on bare-metal ColdFire
 *
 * Newlib's libc expects a handful of POSIX-like syscalls to be
 * provided by the runtime.  On a bare-metal target running MQX (or
 * standalone), there is no OS to delegate to, so we provide stubs.
 *
 * _sbrk() is the only non-trivial stub: it manages heap allocation
 * between the end of BSS and the top of the stack region.  The
 * symbols __end and __stack_top must be defined by the linker script.
 *
 * Compile with the rest of the BP firmware; this file is only
 * needed when linking against newlib (-lc) instead of -lnosys.
 */

#ifdef __GNUC__

#include <sys/stat.h>
#include <errno.h>
#include <stdint.h>

/* Symbols provided by the linker script (GCC .ld) */
extern char __end;        /* end of BSS — start of heap */
extern char __stack_top;  /* top of stack region — heap limit */

static char *heap_ptr;

void *_sbrk(int incr)
{
    char *prev;

    if (heap_ptr == 0)
        heap_ptr = &__end;

    prev = heap_ptr;

    if (heap_ptr + incr > &__stack_top) {
        errno = ENOMEM;
        return (void *)-1;
    }

    heap_ptr += incr;
    return prev;
}

int _close(int fd)
{
    (void)fd;
    return -1;
}

int _read(int fd, char *buf, int len)
{
    (void)fd;
    (void)buf;
    (void)len;
    return 0;
}

int _write(int fd, char *buf, int len)
{
    (void)fd;
    (void)buf;
    return len;
}

int _lseek(int fd, int ptr, int dir)
{
    (void)fd;
    (void)ptr;
    (void)dir;
    return 0;
}

int _fstat(int fd, struct stat *st)
{
    (void)fd;
    st->st_mode = S_IFCHR;
    return 0;
}

int _isatty(int fd)
{
    (void)fd;
    return 1;
}

void _exit(int status)
{
    (void)status;
    for (;;)
        ;
}

#endif /* __GNUC__ */
