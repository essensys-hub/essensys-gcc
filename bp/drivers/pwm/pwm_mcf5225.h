/*
 * pwm_mcf5225.h — pilote PWM « pwm: » du MCF5225x, interface du firmware 099-37.
 *
 * Réécrit d'après le pilote d'origine (bsp.a, io_pwm_mcf5225) désassemblé
 * depuis la sauvegarde 099-37 : mêmes codes ioctl, mêmes structures, mêmes
 * écritures de registres. Feature bp-gcc-firmware-2026-11-006.
 */
#ifndef PWM_MCF5225_H
#define PWM_MCF5225_H

#include <stdint.h>

#define PWM_IOCTL_CONFIG        0x1801
#define PWM_IOCTL_ENABLE_PIN    0x1802
#define PWM_IOCTL_CHANGE_DUTY   0x1803
#define PWM_ERROR_INVALID       0x0A09

typedef struct {
    uint8_t clock_Sys_to_A_divisor;   /* PWMPRCLK[2:0] */
    uint8_t clock_Sys_to_B_divisor;   /* PWMPRCLK[6:4] */
    uint8_t clock_A_to_AS_divisor;    /* PWMSCLA */
    uint8_t clock_B_to_BS_divisor;    /* PWMSCLB */
} PWM_DEV_CONFIG_DATA;

typedef struct {
    uint8_t  pwm_pin;          /* 0 à 7 */
    uint8_t  polarity;         /* PWMPOL */
    uint8_t  xS_clock_select;  /* PWMCLK */
    uint8_t  center_aligned;   /* PWMCAE */
    uint8_t  concatenate;      /* PWMCTL */
    uint8_t  stop_doze;        /* non utilisé par le pilote d'origine */
    uint8_t  stop_debug;       /* non utilisé par le pilote d'origine */
    uint8_t  pcm_enable;       /* registre PCM, offset 0x26 */
    uint16_t period;
    uint16_t duty;
} PWM_DEV_ENABLE_PIN_DATA;

typedef struct {
    uint8_t  pwm_pin;
    uint16_t period;
    uint16_t duty;
} PWM_DEV_CHANGE_DUTY_DATA;

/* Cœur testable sur hôte : agit sur deux blocs de registres fournis. */
void    pwm_core_bind(volatile uint8_t *pwm, volatile uint8_t *gpio);
int32_t pwm_core_ioctl(uint32_t cmd, void *param);

/* Installation du périphérique MQX « pwm: » (cible seulement). */
int32_t _io_pwm_install(char *identifier);

#endif
