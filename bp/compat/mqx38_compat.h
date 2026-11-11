/*
 * mqx38_compat.h — API MQX 3.8 utilisées par le firmware 099-37 et absentes
 * de MQX 4.0. Inclus de force pour les sources applicatives seulement.
 * Feature bp-gcc-firmware-2026-11-006.
 */
#ifndef MQX38_COMPAT_H
#define MQX38_COMPAT_H

#include <mqx.h>
#include <bsp.h>
#include <psptypes_legacy.h>

/* Ancienne API RTC (MQX 3.8) réimplémentée dans compat/mqx38_rtc.c. */
typedef struct {
    uint_32 seconds;
    uint_32 minutes;
    uint_32 hours;
    uint_32 days;
    uint_32 alarm;
} RTC_TIME_STRUCT;

/* MQX 4.0 garde le nom _rtc_get_time avec une autre signature. */
void    essensys_rtc_get_time(RTC_TIME_STRUCT *time);
#define _rtc_get_time(t) essensys_rtc_get_time(t)
uint_32 _rtc_sync_with_mqx(boolean update_mqx);


#include "pwm_mcf5225.h"

#endif
