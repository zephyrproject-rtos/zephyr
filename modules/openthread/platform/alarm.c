/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2024 NXP.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_NAME net_openthread_alarm
#define LOG_LEVEL CONFIG_OPENTHREAD_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <zephyr/kernel.h>
#include <string.h>
#include <inttypes.h>

#include <openthread/platform/alarm-milli.h>
#include <openthread/platform/alarm-micro.h>
#include <openthread/platform/diag.h>
#include <openthread-system.h>

#include <stdio.h>

#include "platform-zephyr.h"
#include "openthread-core-zephyr-config.h"

static bool timer_ms_fired, timer_us_fired;
static int32_t time_offset_us;
static int32_t time_offset_ms;

static void ot_timer_ms_fired(struct k_timer *timer)
{
	ARG_UNUSED(timer);

	timer_ms_fired = true;
	otSysEventSignalPending();
}

static void ot_timer_us_fired(struct k_timer *timer)
{
	ARG_UNUSED(timer);

	timer_us_fired = true;
	otSysEventSignalPending();
}

K_TIMER_DEFINE(ot_ms_timer, ot_timer_ms_fired, NULL);
K_TIMER_DEFINE(ot_us_timer, ot_timer_us_fired, NULL);

void platformAlarmInit(void)
{
#if defined(CONFIG_NET_PKT_TXTIME)
	time_offset_us =
		(int32_t)((int64_t)otPlatAlarmMicroGetNow() - (uint32_t)otPlatRadioGetNow(NULL));
	time_offset_ms = time_offset_us / 1000;
#endif
}

void platformAlarmProcess(otInstance *aInstance)
{
#if OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE
	if (timer_us_fired) {
		timer_us_fired = false;
		otPlatAlarmMicroFired(aInstance);
	}
#endif
	if (timer_ms_fired) {
		timer_ms_fired = false;
#if defined(CONFIG_OPENTHREAD_DIAG)
		if (otPlatDiagModeGet()) {
			otPlatDiagAlarmFired(aInstance);
		} else
#endif
		{
			otPlatAlarmMilliFired(aInstance);
		}
	}
}

uint32_t otPlatAlarmMilliGetNow(void)
{
	return k_uptime_get_32() - time_offset_ms;
}

void otPlatAlarmMilliStartAt(otInstance *aInstance, uint32_t aT0, uint32_t aDt)
{
	ARG_UNUSED(aInstance);

	int32_t delta = (int32_t)(aT0 + aDt - otPlatAlarmMilliGetNow());

	if (delta > 0) {
		k_timer_start(&ot_ms_timer, K_MSEC(delta), K_NO_WAIT);
	} else {
		ot_timer_ms_fired(NULL);
	}
}

void otPlatAlarmMilliStop(otInstance *aInstance)
{
	ARG_UNUSED(aInstance);

	k_timer_stop(&ot_ms_timer);
}

void otPlatAlarmMicroStartAt(otInstance *aInstance, uint32_t aT0, uint32_t aDt)
{
	ARG_UNUSED(aInstance);

	int32_t delta = (int32_t)(aT0 + aDt - otPlatAlarmMicroGetNow());

	if (delta > 0) {
		k_timer_start(&ot_us_timer, K_USEC(delta), K_NO_WAIT);
	} else {
		ot_timer_us_fired(NULL);
	}
}

void otPlatAlarmMicroStop(otInstance *aInstance)
{
	ARG_UNUSED(aInstance);

	k_timer_stop(&ot_us_timer);
}

uint32_t otPlatAlarmMicroGetNow(void)
{
	return (uint32_t)(k_ticks_to_us_floor64(k_uptime_ticks()) - time_offset_us);
}

uint16_t otPlatTimeGetXtalAccuracy(void)
{
	return otPlatRadioGetCslAccuracy(NULL);
}

uint64_t otPlatTimeGet(void)
{
	return  (uint64_t)(k_uptime_get() * 1000);
}
