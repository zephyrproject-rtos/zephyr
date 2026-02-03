/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_NAME openthread_alarm_counter
#define LOG_LEVEL CONFIG_OPENTHREAD_PLATFORM_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <zephyr/kernel.h>
#include <string.h>
#include <inttypes.h>
#include <zephyr/devicetree.h>

#include <openthread/platform/alarm-milli.h>
#include <openthread/platform/alarm-micro.h>
#include <openthread/platform/diag.h>
#include <openthread-system.h>

#include <stdio.h>

#include "platform-zephyr.h"
#include "openthread-core-zephyr-config.h"

#include <zephyr/drivers/counter.h>

static bool timer_ms_fired, timer_us_fired;
static int32_t time_offset_us;
static int32_t time_offset_ms;

typedef struct wrap_timer_data wrap_timer_data_t;

struct wrap_timer_data
{
    uint16_t overflow_counter;
    uint16_t overflow_max;
};

static wrap_timer_data_t wrap_data;

#define COUNTER DT_CHOSEN(zephyr_ot_alarm)
const struct device *const alarm_counter = DEVICE_DT_GET(DT_NODELABEL(protimer));

struct counter_alarm_cfg alarm_cfg;

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

static inline bool is_alarm_overflow_in_progress()
{
    return wrap_data.overflow_counter < wrap_data.overflow_max;
}

static inline uint32_t set_alarm_wrapped_duration(uint64_t aRemainingTime)
{
    uint64_t initial_wrap_time = aRemainingTime;
    wrap_timer_data_t wrapData = {0};

    if (initial_wrap_time > counter_get_top_value(alarm_counter))
    {
        initial_wrap_time %= counter_get_top_value(alarm_counter);
        wrapData.overflow_max     = (uint16_t)(aRemainingTime / counter_get_top_value(alarm_counter));
        wrapData.overflow_counter = 0;
    }
    wrap_data = wrapData;
    return (uint32_t)initial_wrap_time;
}

K_TIMER_DEFINE(ot_ms_timer, ot_timer_ms_fired, NULL);
// K_TIMER_DEFINE(ot_us_timer, ot_timer_us_fired, NULL);

static void alarm_handler(const struct device *dev, uint8_t chan_id, uint32_t ticks, void *user_data) {
	ARG_UNUSED(dev);

	if (is_alarm_overflow_in_progress())
    {
		struct counter_alarm_cfg cntr_alarm_cfg;
		wrap_data.overflow_counter++;
		counter_cancel_channel_alarm(alarm_counter, chan_id);

		cntr_alarm_cfg.ticks = ticks; //counter_get_top_value(alarm_counter);
		cntr_alarm_cfg.flags = 0;
		cntr_alarm_cfg.callback = alarm_handler;
		cntr_alarm_cfg.user_data = user_data;
		counter_set_channel_alarm(alarm_counter, chan_id, &cntr_alarm_cfg);
    }
    else
    {
		counter_cancel_channel_alarm(alarm_counter, chan_id);
		ot_timer_us_fired(NULL);
    }
}

static void top_handler(const struct device *dev, void *user_data)
{
	ARG_UNUSED(user_data);
	counter_stop(dev);
	return;
}

void platformAlarmInit(void)
{
	struct counter_top_cfg top_cfg = {
		.callback = top_handler,
		.user_data = NULL,
		.flags = 0,
		.ticks = UINT32_MAX
	};
	if (counter_get_num_of_channels(alarm_counter) < 1U) {
		/* Counter does not support any alarm */
		return;
	}
	int status = counter_start(alarm_counter);
	if (status != 0)
	{
		LOG_ERR("Failed to start alarm counter: %d\n", status);
	}
	status = counter_set_top_value(alarm_counter, &top_cfg);
	if (status != 0)
	{
		LOG_ERR("Failed to set top value for alarm counter: %d\n", status);
	}
	
	memset(&wrap_data, 0, sizeof(wrap_timer_data_t));
	
#if defined(CONFIG_OPENTHREAD_PLATFORM_PKT_TXTIME)
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
	struct counter_alarm_cfg cntr_alarm_cfg;
	counter_cancel_channel_alarm(alarm_counter, 0);

    uint64_t requested_time = (uint64_t)aT0 + (uint64_t)aDt;
	uint32_t *ticks;
	int status = counter_get_value(alarm_counter, ticks);
    int64_t  remaining      = (int64_t)requested_time - (int64_t)counter_ticks_to_us(alarm_counter, ticks);

    if (remaining <= 0)
    {
		counter_cancel_channel_alarm(alarm_counter, 0);
		ot_timer_us_fired(NULL);
    }
    else
    {
		cntr_alarm_cfg.ticks = counter_us_to_ticks(alarm_counter, remaining);
		cntr_alarm_cfg.flags = 0;
		cntr_alarm_cfg.callback = alarm_handler;
		cntr_alarm_cfg.user_data = &cntr_alarm_cfg;
		counter_set_channel_alarm(alarm_counter, 0, &cntr_alarm_cfg);
    }
}

void otPlatAlarmMicroStop(otInstance *aInstance)
{
	ARG_UNUSED(aInstance);
	counter_cancel_channel_alarm(alarm_counter, 0);
}

uint32_t otPlatAlarmMicroGetNow(void)
{
	uint32_t *ticks;
	int status = counter_get_value(alarm_counter, ticks);
	return (uint32_t)counter_ticks_to_us(alarm_counter, ticks);
}

uint16_t otPlatTimeGetXtalAccuracy(void)
{
	return otPlatRadioGetCslAccuracy(NULL);
}

#ifdef CONFIG_HDLC_RCP_IF
uint64_t otPlatTimeGet(void)
{
	static uint32_t timerWraps   = 0U;
    static uint32_t prev32TimeUs = 0U;
    uint32_t        now32TimeUs;
    uint64_t        now64TimeUs;
	uint32_t *ticks;
	int status = counter_get_value(alarm_counter, ticks);
    now32TimeUs = (uint32_t)counter_ticks_to_us(alarm_counter, ticks);
    if (now32TimeUs < prev32TimeUs)
    {
        timerWraps += 1U;
    }
    prev32TimeUs = now32TimeUs;
    now64TimeUs  = ((uint64_t)timerWraps << 32) + now32TimeUs;
    return now64TimeUs;
}
#endif