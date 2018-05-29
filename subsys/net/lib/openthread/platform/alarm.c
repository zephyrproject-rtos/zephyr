/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <string.h>
#include <inttypes.h>

#include <openthread/platform/alarm-milli.h>
#include <platform.h>

#define SYS_LOG_DOMAIN "openthread-plat"
#define SYS_LOG_LEVEL SYS_LOG_LEVEL_DEBUG
#include <logging/sys_log.h>
#include <stdio.h>

#include "platform-zephyr.h"

static bool timer_fired;

static void ot_timer_fired(struct k_timer *timer)
{
	ARG_UNUSED(timer);

	timer_fired = true;
	PlatformEventSignalPending();
}

K_TIMER_DEFINE(ot_timer, ot_timer_fired, NULL);

void platformAlarmInit(void)
{
	/* Intentionally empty */
}

uint32_t otPlatAlarmMilliGetNow(void)
{
	return k_uptime_get_32();
}

void otPlatAlarmMilliStartAt(otInstance *aInstance, uint32_t t0, uint32_t dt)
{
	ARG_UNUSED(aInstance);

	s64_t reftime = (s64_t)t0 + (s64_t)dt;
	s64_t delta = -k_uptime_delta(&reftime);

	if (delta > 0) {
		k_timer_start(&ot_timer, K_MSEC(delta), 0);
	} else {
		ot_timer_fired(NULL);
	}
}

void otPlatAlarmMilliStop(otInstance *aInstance)
{
	ARG_UNUSED(aInstance);

	k_timer_stop(&ot_timer);
}


void platformAlarmProcess(otInstance *aInstance)
{
	if (timer_fired) {
		timer_fired = false;
		otPlatAlarmMilliFired(aInstance);
	}
}
