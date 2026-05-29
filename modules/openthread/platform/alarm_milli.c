/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2024 NXP.
 * Copyright (c) 2026 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_NAME net_openthread_alarm_milli
#define LOG_LEVEL       CONFIG_OPENTHREAD_PLATFORM_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <zephyr/kernel.h>

#include <openthread/platform/alarm-milli.h>
#include <openthread/platform/diag.h>
#include <openthread-system.h>

static bool timer_ms_fired;
static int32_t time_offset_ms;

static void ot_timer_ms_fired(struct k_timer *timer)
{
	ARG_UNUSED(timer);

	timer_ms_fired = true;
	otSysEventSignalPending();
}

K_TIMER_DEFINE(ot_ms_timer, ot_timer_ms_fired, NULL);

void platformAlarmMilliInit(void)
{
	/* Millisecond alarm uses K_TIMER_DEFINE(ot_ms_timer) and static state only;
	 * no device or runtime init required.
	 */
}

void alarm_milli_set_time_offset_ms(int32_t offset_ms)
{
	time_offset_ms = offset_ms;
}

void platformAlarmMilliProcess(otInstance *aInstance)
{
	if (timer_ms_fired) {
		timer_ms_fired = false;
#if defined(CONFIG_OPENTHREAD_DIAG)
		if (otPlatDiagModeGet()) {
			otPlatDiagAlarmFired(aInstance);
		} else {
#endif
			otPlatAlarmMilliFired(aInstance);
#if defined(CONFIG_OPENTHREAD_DIAG)
		}
#endif
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
