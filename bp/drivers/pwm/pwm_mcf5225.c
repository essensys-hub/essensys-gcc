/*
 * pwm_mcf5225.c — périphérique MQX « pwm: » au-dessus de pwm_core.c.
 * Les registres sont à IPSBAR + 0x1B0000 (PWM) et IPSBAR + 0x100000 (GPIO).
 */
#include <mqx.h>
#include <bsp.h>
#include <fio.h>
#include <io.h>
#include "pwm_mcf5225.h"

static _mqx_int pwm_open(MQX_FILE_PTR f, char *name, char *flags)
{
    (void)f; (void)name; (void)flags;
    pwm_core_bind((volatile uint8_t *)(BSP_IPSBAR + 0x1B0000),
                  (volatile uint8_t *)(BSP_IPSBAR + 0x100000));
    return MQX_OK;
}

static _mqx_int pwm_close(MQX_FILE_PTR f) { (void)f; return MQX_OK; }
static _mqx_int pwm_rw(MQX_FILE_PTR f, char *b, _mqx_int n) { (void)f; (void)b; (void)n; return 0; }

static _mqx_int pwm_ioctl(MQX_FILE_PTR f, _mqx_uint cmd, void *param)
{
    (void)f;
    return (_mqx_int)pwm_core_ioctl(cmd, param);
}

int32_t _io_pwm_install(char *identifier)
{
    return _io_dev_install(identifier, pwm_open, pwm_close, pwm_rw, pwm_rw, pwm_ioctl, NULL);
}
