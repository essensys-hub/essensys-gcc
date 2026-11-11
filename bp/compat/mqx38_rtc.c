/*
 * mqx38_rtc.c — ancienne API RTC de MQX 3.8 au-dessus de la RTC de MQX 4.0.
 * Feature bp-gcc-firmware-2026-11-006.
 *
 * MQX 3.8 : RTC_TIME_STRUCT découpe le temps en jours, heures, minutes,
 * secondes ; _rtc_sync_with_mqx(TRUE) recopie la RTC dans l'heure MQX,
 * FALSE recopie l'heure MQX dans la RTC. MQX 4.0 manipule des secondes.
 */
#include "mqx38_compat.h"
#include <rtc.h>

#undef _rtc_get_time

void essensys_rtc_get_time(RTC_TIME_STRUCT *t)
{
    uint32_t s = 0;
    _rtc_get_time(&s);
    t->days = s / 86400u;
    s %= 86400u;
    t->hours = s / 3600u;
    s %= 3600u;
    t->minutes = s / 60u;
    t->seconds = s % 60u;
    t->alarm = 0;
}

uint_32 _rtc_sync_with_mqx(boolean update_mqx)
{
    TIME_STRUCT mqx_time;
    uint32_t s = 0;
    if (update_mqx) {
        if (_rtc_get_time(&s) != MQX_OK) return MQX_INVALID_PARAMETER;
        mqx_time.SECONDS = s;
        mqx_time.MILLISECONDS = 0;
        _time_set(&mqx_time);
        return MQX_OK;
    }
    _time_get(&mqx_time);
    return (uint_32)_rtc_set_time(mqx_time.SECONDS);
}
