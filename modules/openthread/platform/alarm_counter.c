/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_NAME net_openthread_alarm_counter
#define LOG_LEVEL       CONFIG_OPENTHREAD_PLATFORM_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <zephyr/kernel.h>
#include <string.h>
#include <inttypes.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>

#include <openthread/platform/alarm-milli.h>
#include <openthread/platform/alarm-micro.h>
#include <openthread-system.h>

#include "platform-zephyr.h"
#include "openthread-core-zephyr-config.h"

#include <zephyr/drivers/counter.h>

void alarm_milli_set_time_offset_ms(int32_t offset_ms);

static bool timer_us_fired;
static int32_t time_offset_us;

/* Wrap logic: split delays longer than one counter period into multiple alarms */
typedef struct wrap_timer_data wrap_timer_data_t;

struct wrap_timer_data {
	uint16_t remaining_iterations; /* Counts down from N to 0; 0 means alarm is done */
	uint32_t tick_value;	       /* Absolute tick used on every re-arm */
};

static wrap_timer_data_t wrap_data;

#define OT_COUNTER DT_CHOSEN(zephyr_openthread_counter)
const struct device *const alarm_counter = DEVICE_DT_GET(OT_COUNTER);

static void ot_timer_us_fired(struct k_timer *timer)
{
	ARG_UNUSED(timer);

	timer_us_fired = true;
	otSysEventSignalPending();
}

static inline bool is_alarm_finished(void)
{
	return wrap_data.remaining_iterations == 0U;
}

/* For delays > one period: set first alarm to (remaining % period), re-arm in handler until done */
static inline uint32_t set_alarm_wrapped_duration(uint64_t remaining_ticks)
{
	uint64_t period_ticks = (uint64_t)counter_get_top_value(alarm_counter) + 1U;
	bool wraps = (period_ticks > 0U && remaining_ticks > period_ticks);
	wrap_timer_data_t temp_wrap_data = {0};

	if (wraps) {
		/* Re-arm (N-1) times for N period chunks; 0 means no re-arms. */
		temp_wrap_data.remaining_iterations =
			(uint16_t)((remaining_ticks / period_ticks) - 1U);
	}
	wrap_data = temp_wrap_data;
	return (uint32_t)(wraps ? remaining_ticks % period_ticks : remaining_ticks);
}

static void alarm_handler(const struct device *dev, uint8_t chan_id, uint32_t ticks,
			  void *user_data)
{

	if (!is_alarm_finished()) {
		struct counter_alarm_cfg cntr_alarm_cfg;

		wrap_data.remaining_iterations--;
		counter_cancel_channel_alarm(dev, chan_id);

		/* Re-arm at same absolute tick (full period wraps to same value in 32-bit). */
		cntr_alarm_cfg.ticks = wrap_data.tick_value;
		cntr_alarm_cfg.flags = COUNTER_ALARM_CFG_ABSOLUTE;
		cntr_alarm_cfg.callback = alarm_handler;
		cntr_alarm_cfg.user_data = user_data;
		counter_set_channel_alarm(dev, chan_id, &cntr_alarm_cfg);
	} else {
		counter_cancel_channel_alarm(dev, chan_id);
		timer_us_fired = true;
		otSysEventSignalPending();
	}
}

void platformAlarmMicroInit(void)
{
	if (!device_is_ready(alarm_counter) || counter_get_num_of_channels(alarm_counter) < 1U) {
		return;
	}
	if (counter_start(alarm_counter) != 0) {
		LOG_ERR("Failed to start alarm counter");
	}

	memset(&wrap_data, 0, sizeof(wrap_timer_data_t));

#if defined(CONFIG_OPENTHREAD_PLATFORM_PKT_TXTIME)
	time_offset_us =
		(int32_t)((int64_t)otPlatAlarmMicroGetNow() - (uint32_t)otPlatRadioGetNow(NULL));
	alarm_milli_set_time_offset_ms(time_offset_us / 1000);
#endif
}

void platformAlarmMicroProcess(otInstance *aInstance)
{
#if OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE
	if (timer_us_fired) {
		timer_us_fired = false;
		otPlatAlarmMicroFired(aInstance);
	}
#endif
}

void otPlatAlarmMicroStartAt(otInstance *aInstance, uint32_t aT0, uint32_t aDt)
{
	ARG_UNUSED(aInstance);
	struct counter_alarm_cfg cntr_alarm_cfg;
	uint32_t current_ticks;
	uint64_t remaining_us = (uint64_t)aT0 + (uint64_t)aDt;

	(void)counter_get_value(alarm_counter, &current_ticks);
	remaining_us -= counter_ticks_to_us(alarm_counter, current_ticks);

	counter_cancel_channel_alarm(alarm_counter, 0);

	if ((int64_t)remaining_us <= 0) {
		ot_timer_us_fired(NULL);
	} else {
		uint64_t remaining_ticks =
			counter_us_to_ticks_64(alarm_counter, remaining_us);
		uint64_t period_ticks =
			(uint64_t)counter_get_top_value(alarm_counter) + 1U;
		bool wraps = (period_ticks > 0U && remaining_ticks > period_ticks);

		cntr_alarm_cfg.ticks = wraps ? set_alarm_wrapped_duration(remaining_ticks)
					     : (uint32_t)remaining_ticks;
		if (wraps) {
			/* Modulo period so tick_value is in counter range (e.g. 16-bit). */
			wrap_data.tick_value =
				(uint32_t)((current_ticks + cntr_alarm_cfg.ticks) % period_ticks);
		}
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
	uint32_t ticks;

	counter_get_value(alarm_counter, &ticks);
	return (uint32_t)counter_ticks_to_us(alarm_counter, ticks);
}

uint16_t otPlatTimeGetXtalAccuracy(void)
{
	return otPlatRadioGetCslAccuracy(NULL);
}

#ifdef CONFIG_HDLC_RCP_IF
uint64_t otPlatTimeGet(void)
{
	static uint32_t timerWraps;
	static uint32_t prev32TimeUs;
	uint32_t now32TimeUs;
	uint64_t now64TimeUs;
	uint32_t ticks;

	counter_get_value(alarm_counter, &ticks);
	now32TimeUs = (uint32_t)counter_ticks_to_us(alarm_counter, ticks);
	if (now32TimeUs < prev32TimeUs) {
		timerWraps += 1U;
	}
	prev32TimeUs = now32TimeUs;
	now64TimeUs = ((uint64_t)timerWraps << 32) + now32TimeUs;
	return now64TimeUs;
}
#endif
