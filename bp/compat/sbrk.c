/*
 * sbrk.c — malloc et free du firmware 099-37 sur l'allocateur de MQX.
 * newlib appellerait sbrk, absent sans système ; CodeWarrior (EWL) passait
 * déjà par MQX. Feature bp-gcc-firmware-2026-11-006.
 */
#include <mqx.h>
#include <string.h>
#include <errno.h>
#include <reent.h>

void *malloc(size_t n) { return _mem_alloc_system(n); }
void free(void *p) { if (p) _mem_free(p); }
void *calloc(size_t n, size_t m) { return _mem_alloc_system_zero(n * m); }

void *realloc(void *p, size_t n)
{
    void *q;
    if (!p) return malloc(n);
    if (!n) { free(p); return NULL; }
    q = malloc(n);
    if (q) {
        _mem_size old = _mem_get_size(p);
        memcpy(q, p, old < n ? old : n);
        free(p);
    }
    return q;
}

/* Filet : toute autre allocation de newlib échoue proprement. */
void *_sbrk_r(struct _reent *r, ptrdiff_t incr)
{
    (void)incr;
    r->_errno = ENOMEM;
    return (void *)-1;
}
void *sbrk(ptrdiff_t incr) { (void)incr; errno = ENOMEM; return (void *)-1; }
