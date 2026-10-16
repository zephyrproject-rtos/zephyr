/*
 * Copyright (c) 2026 Hula Earth
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/timer/system_timer_lpm.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/util.h>

#define LPM_COUNTER_NODE       DT_CHOSEN(zephyr_system_timer_companion)
#define LPM_ALARM_CHANNEL_ID   0U
#define LPM_MAX_WAKE_SECONDS   UINT16_MAX
#define LPM_MAX_WAKE_TIME_US   ((uint64_t)LPM_MAX_WAKE_SECONDS * USEC_PER_SEC)

BUILD_ASSERT(DT_SAME_NODE(LPM_COUNTER_NODE, DT_NODELABEL(rtc)),
	     "STM32N6 needs RTC as the system timer companion");
BUILD_ASSERT(DT_PROP(LPM_COUNTER_NODE, wakeup_source),
	     "STM32N6 system timer companion must be a wakeup source");

static const struct device *const lpm_counter = DEVICE_DT_GET(LPM_COUNTER_NODE);
static uint32_t counter_pre_lpm_ticks;
static uint32_t counter_scheduled_lpm_ticks;

static void lpm_alarm_callback(const struct device *dev, uint8_t chan_id,
			       uint32_t ticks, void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(chan_id);
	ARG_UNUSED(ticks);
	ARG_UNUSED(user_data);
}

static void lpm_timer_panic_on_error(int ret)
{
	if (ret != 0) {
		k_panic();
	}
}

void z_sys_clock_lpm_init(void)
{
	if (!device_is_ready(lpm_counter)) {
		k_panic();
	}

	if (IS_ENABLED(CONFIG_PM_DEVICE)) {
		if (!pm_device_wakeup_is_capable(lpm_counter) ||
		    !pm_device_wakeup_enable(lpm_counter, true)) {
			k_panic();
		}
	}
}

void z_sys_clock_lpm_enter(uint64_t max_lpm_time_us)
{
	struct counter_alarm_cfg alarm_cfg = {
		.callback = lpm_alarm_callback,
		.user_data = NULL,
		.flags = 0U,
	};
	int ret;

	if (IS_ENABLED(CONFIG_PM_DEVICE) && !pm_device_wakeup_is_enabled(lpm_counter)) {
		k_panic();
	}

	max_lpm_time_us = MIN(max_lpm_time_us, LPM_MAX_WAKE_TIME_US);
	counter_scheduled_lpm_ticks = counter_us_to_ticks(lpm_counter, max_lpm_time_us);
	alarm_cfg.ticks = counter_scheduled_lpm_ticks;

	ret = counter_cancel_channel_alarm(lpm_counter, LPM_ALARM_CHANNEL_ID);
	lpm_timer_panic_on_error(ret);

	ret = counter_set_channel_alarm(lpm_counter, LPM_ALARM_CHANNEL_ID, &alarm_cfg);
	lpm_timer_panic_on_error(ret);

	ret = counter_get_value(lpm_counter, &counter_pre_lpm_ticks);
	lpm_timer_panic_on_error(ret);
}

uint64_t z_sys_clock_lpm_exit(void)
{
	uint32_t counter_top = counter_get_top_value(lpm_counter);
	uint32_t post_lpm_ticks;
	uint32_t ticks_elapsed;
	bool counter_int_pending = counter_get_pending_int(lpm_counter) != 0;
	bool wraparound_occurred;
	int ret;

	ret = counter_get_value(lpm_counter, &post_lpm_ticks);
	lpm_timer_panic_on_error(ret);

	if (counter_pre_lpm_ticks > post_lpm_ticks) {
		wraparound_occurred = true;
	} else if (counter_pre_lpm_ticks == post_lpm_ticks) {
		wraparound_occurred = counter_int_pending;
	} else {
		wraparound_occurred = counter_int_pending &&
			((uint64_t)counter_pre_lpm_ticks + counter_scheduled_lpm_ticks >=
			 counter_top);
	}

	if (wraparound_occurred) {
		ticks_elapsed = counter_top - counter_pre_lpm_ticks + post_lpm_ticks + 1U;
	} else {
		ticks_elapsed = post_lpm_ticks - counter_pre_lpm_ticks;
	}

	return counter_ticks_to_us(lpm_counter, ticks_elapsed);
}
