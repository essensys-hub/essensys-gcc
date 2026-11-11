/*
 * pwm_core.c — logique du pilote PWM MCF5225x, sans MQX (testée sur hôte).
 * Offsets d'après MCF5225_PWM_STRUCT et le pilote 099-37 désassemblé.
 */
#include "pwm_mcf5225.h"

enum { PWME = 0x00, PWMPOL = 0x01, PWMCLK = 0x02, PWMPRCLK = 0x03,
       PWMCAE = 0x04, PWMCTL = 0x05, PWMSCLA = 0x08, PWMSCLB = 0x09,
       PWMCNT = 0x0C, PWMPER = 0x14, PWMDTY = 0x1C, PWMPCME = 0x26 };
/* GPIO : port TA (canaux impairs) et TC (canaux pairs). */
enum { GPIO_DDRTA = 0x26, GPIO_DDRTC = 0x27, GPIO_PTAPAR = 0x6E, GPIO_PTCPAR = 0x6F };

static volatile uint8_t *pwm_regs;
static volatile uint8_t *gpio_regs;

void pwm_core_bind(volatile uint8_t *pwm, volatile uint8_t *gpio)
{
    pwm_regs = pwm;
    gpio_regs = gpio;
}

static void set_bit(uint32_t off, uint32_t mask, int on)
{
    uint8_t v = pwm_regs[off];
    pwm_regs[off] = (uint8_t)(on ? (v | mask) : (v & ~mask));
}

static void set_duty(uint8_t pin, uint16_t period, uint16_t duty)
{
    if (pwm_regs[PWMCTL] & (0x10u << (pin >> 1))) {
        uint32_t ch = pin & ~1u;   /* paire concaténée : registres 16 bits */
        pwm_regs[PWMPER + ch] = (uint8_t)(period >> 8);
        pwm_regs[PWMPER + ch + 1] = (uint8_t)period;
        pwm_regs[PWMDTY + ch] = (uint8_t)(duty >> 8);
        pwm_regs[PWMDTY + ch + 1] = (uint8_t)duty;
    } else {
        pwm_regs[PWMPER + pin] = (uint8_t)period;
        pwm_regs[PWMDTY + pin] = (uint8_t)duty;
    }
}

static void mux_pin(uint8_t pin)
{
    uint32_t shift = (pin >> 1) * 2u;
    uint32_t ddr = (pin & 1u) ? GPIO_DDRTA : GPIO_DDRTC;
    uint32_t par = (pin & 1u) ? GPIO_PTAPAR : GPIO_PTCPAR;
    gpio_regs[ddr] = (uint8_t)(gpio_regs[ddr] & ~(1u << (pin >> 1)));
    gpio_regs[par] = (uint8_t)((gpio_regs[par] & ~(3u << shift)) | (3u << shift));
}

int32_t pwm_core_ioctl(uint32_t cmd, void *param)
{
    switch (cmd) {
    case PWM_IOCTL_CONFIG: {
        const PWM_DEV_CONFIG_DATA *c = param;
        pwm_regs[PWMPRCLK] = (uint8_t)(((c->clock_Sys_to_B_divisor & 7u) << 4) |
                                       (c->clock_Sys_to_A_divisor & 7u));
        pwm_regs[PWMSCLA] = c->clock_A_to_AS_divisor;
        pwm_regs[PWMSCLB] = c->clock_B_to_BS_divisor;
        return 0;
    }
    case PWM_IOCTL_ENABLE_PIN: {
        const PWM_DEV_ENABLE_PIN_DATA *p = param;
        uint8_t pin = p->pwm_pin;
        if (pin > 7) return PWM_ERROR_INVALID;
        uint8_t bit = (uint8_t)(1u << pin);
        set_bit(PWME, bit, 0);
        mux_pin(pin);
        set_bit(PWMCTL, 0x10u << (pin >> 1), p->concatenate);
        pwm_regs[PWMCNT + pin] = 0;
        set_duty(pin, p->period, p->duty);
        set_bit(PWMCAE, bit, p->center_aligned);
        set_bit(PWMCLK, bit, p->xS_clock_select);
        set_bit(PWMPOL, bit, p->polarity);
        set_bit(PWMPCME, bit, p->pcm_enable);
        set_bit(PWME, bit, 1);
        return 0;
    }
    case PWM_IOCTL_CHANGE_DUTY: {
        const PWM_DEV_CHANGE_DUTY_DATA *d = param;
        if (d->pwm_pin > 7) return PWM_ERROR_INVALID;
        set_duty(d->pwm_pin, d->period, d->duty);
        return 0;
    }
    default:
        return PWM_ERROR_INVALID;
    }
}
