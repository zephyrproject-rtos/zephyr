/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * Implementation of the system timer low-power companion interface
 * using the standard Counter API.
 */

#include <zephyr/device.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/timer/system_timer_lpm.h>

#if defined(CONFIG_CORTEX_M_SYSTICK) && !DT_HAS_CHOSEN(zephyr_system_timer_companion)
/*
 * Compatibility fallback inherited from the Cortex-M SysTick driver:
 * the deprecated /chosen/zephyr,cortex-m-idle-timer may be used to select
 * the companion counter instead of /chosen/zephyr,system-timer-companion.
 *
 * This is scheduled for removal in Zephyr 4.6.0.
 */
#define COMPANION_COUNTER_NODE DT_CHOSEN(zephyr_cortex_m_idle_timer)
#else /* CONFIG_CORTEX_M_SYSTICK && !DT_HAS_CHOSEN(zephyr_system_timer_companion) */
#define COMPANION_COUNTER_NODE DT_CHOSEN(zephyr_system_timer_companion)
#endif /* CONFIG_CORTEX_M_SYSTICK && !DT_HAS_CHOSEN(zephyr_system_timer_companion) */

static const struct device *lpm_counter = DEVICE_DT_GET(COMPANION_COUNTER_NODE);

/* Companion counter's value before entry in low-power state. */
static uint32_t counter_pre_lpm_ticks;

/*
 * How many ticks the system was expected to remain in low-power state.
 * Used to determine if the counter overflowed or not.
 */
static uint32_t counter_scheduled_lpm_ticks;

/* Channel ID of the alarm that should wake up the system */
#define LPM_ALARM_CHANNEL_ID 0U

/* Stub to satisfy the Counter API (`callback` cannot be NULL) */
static void stub_alarm_callback(const struct device *dev, uint8_t chan_id,
				      uint32_t ticks, void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(chan_id);
	ARG_UNUSED(ticks);
	ARG_UNUSED(user_data);
}

void z_sys_clock_lpm_enter(uint64_t max_lpm_time_us)
{
	counter_scheduled_lpm_ticks = counter_us_to_ticks(lpm_counter, max_lpm_time_us);

	struct counter_alarm_cfg cfg = {
		.callback = stub_alarm_callback,
		.ticks = counter_scheduled_lpm_ticks,
		.user_data = NULL,
		.flags = 0,
	};

	/* Disable the counter alarm in case it was already running */
	counter_cancel_channel_alarm(lpm_counter, LPM_ALARM_CHANNEL_ID);

	/* Configure the alarm to wake up the system as requested */
	counter_set_channel_alarm(lpm_counter, LPM_ALARM_CHANNEL_ID, &cfg);

	/*
	 * Store the current value of the counter to allow computing
	 * how much time was really spent in low-power state, as the
	 * system may be awoken earlier than scheduled.
	 */
	counter_get_value(lpm_counter, &counter_pre_lpm_ticks);
}

uint64_t z_sys_clock_lpm_exit(void)
{
	/* Compute how much time elapsed according to the counter */
	uint32_t counter_top, post_lpm_ticks, ticks_elapsed;
	bool counter_int_pending, wraparound_occurred;

	counter_int_pending = !!counter_get_pending_int(lpm_counter);
	counter_top = counter_get_top_value(lpm_counter);
	counter_get_value(lpm_counter, &post_lpm_ticks);

	/* Check for counter overflow/wraparound */
	/* TODO: this works only for upcounting timers! */
	if (counter_pre_lpm_ticks > post_lpm_ticks) {
		/* Pre > Post: counter wrapped around (overflow occurred) */
		wraparound_occurred = true;
	} else if (counter_pre_lpm_ticks == post_lpm_ticks) {
		/* Pre == Post: counter wrapped around if interrupt is pending */
		wraparound_occurred = counter_int_pending;
	} else {
		/* Pre < Post: usually means no wraparound; however, if interrupt
		 * is pending and an overflow was expected to occur based on how
		 * many ticks we scheduled, then treat this as wraparound.
		 */
		wraparound_occurred = counter_int_pending &&
			((uint64_t)counter_pre_lpm_ticks + counter_scheduled_lpm_ticks
				>= counter_top);
	}

	if (wraparound_occurred) {
		ticks_elapsed = (counter_top - counter_pre_lpm_ticks) + post_lpm_ticks + 1;
	} else {
		ticks_elapsed = post_lpm_ticks - counter_pre_lpm_ticks;
	}

	return (uint64_t)counter_ticks_to_us(lpm_counter, ticks_elapsed);
}
