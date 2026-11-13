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
#include <zephyr/pm/device.h>

BUILD_ASSERT(DT_PROP(SYSTEM_TIMER_COMPANION_NODE, wakeup_source),
	"The Low-Power Companion counter (chosen `zephyr,system-timer-companion` at path: "
	DT_NODE_PATH(SYSTEM_TIMER_COMPANION_NODE) ") must be marked as wake-up capable! "
	"(hint: add the `wakeup-source` property to the node)");

static const struct device *lpm_counter = DEVICE_DT_GET(SYSTEM_TIMER_COMPANION_NODE);

/* Companion counter's value before entry in low-power state. */
static uint32_t counter_pre_lpm_ticks;

/*
 * How many ticks the system was expected to remain in low-power state.
 * Used to determine if the counter overflowed or not.
 */
static uint32_t counter_scheduled_lpm_ticks;

/* Channel ID of the alarm that should wake up the system */
#define LPM_ALARM_CHANNEL_ID 0U

/*
 * Set by the alarm callback when the wake-up alarm fires, cleared when a new sleep is started.
 * Unlike the counter's interrupt status, this is not lost when the counter's ISR runs before
 * z_sys_clock_lpm_exit() does.
 */
static volatile bool lpm_alarm_fired;

void z_sys_clock_lpm_init(void)
{
	__ASSERT(!IS_ENABLED(CONFIG_PM_DEVICE) || pm_device_wakeup_is_capable(lpm_counter),
		"Low-Power Companion counter not marked as wakeup-source!");

	/*
	 * Enable low-power companion counter as wake-up source.
	 *
	 * This cannot be done from z_sys_clock_lpm_enter() because this runs
	 * after all devices have been suspended, which is what enabling the
	 * counter as wake-up source is supposed to prevent.
	 *
	 * NOTE: it is assumed that no other code will change this configuration.
	 * Doing so is an application error and will result in undefined behavior.
	 */
	pm_device_wakeup_enable(lpm_counter, true);
}

/* Records that the wake-up alarm fired (`callback` cannot be NULL in the Counter API) */
static void stub_alarm_callback(const struct device *dev, uint8_t chan_id,
				      uint32_t ticks, void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(chan_id);
	ARG_UNUSED(ticks);
	ARG_UNUSED(user_data);

	lpm_alarm_fired = true;
}

void z_sys_clock_lpm_enter(uint64_t max_lpm_time_us)
{
	/* Catch misuse of the device by external code */
	__ASSERT(!IS_ENABLED(CONFIG_PM_DEVICE) || pm_device_wakeup_is_enabled(lpm_counter),
		"External code must not disable Low-Power Companion wake-up source!");

	counter_scheduled_lpm_ticks = counter_us_to_ticks(lpm_counter, max_lpm_time_us);

	struct counter_alarm_cfg cfg = {
		.callback = stub_alarm_callback,
		.ticks = counter_scheduled_lpm_ticks,
		.user_data = NULL,
		.flags = 0,
	};

	lpm_alarm_fired = false;

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
	uint64_t alarm_target_ticks = (uint64_t)counter_pre_lpm_ticks + counter_scheduled_lpm_ticks;
	bool alarm_expired, wraparound_occurred;

	/*
	 * The callback flag covers the case where the counter's ISR already ran and cleared the
	 * pending interrupt.
	 */
	alarm_expired = lpm_alarm_fired || !!counter_get_pending_int(lpm_counter);

	counter_top = counter_get_top_value(lpm_counter);
	counter_get_value(lpm_counter, &post_lpm_ticks);

	/*
	 * Check for counter overflow/wraparound.
	 *
	 * The counter value alone cannot always tell whether it wrapped: a sleep of exactly one
	 * counter period (or a few ticks more) ends with a value equal to, or slightly above, the
	 * starting one, which looks the same as a very short sleep. Whether the alarm expired is
	 * used to tell these cases apart, as an alarm scheduled to cross the top value can only
	 * expire after the counter has wrapped.
	 */
	/* TODO: this works only for upcounting timers! */
	if (counter_pre_lpm_ticks > post_lpm_ticks) {
		/* Pre > Post: the value went backwards, so the counter must have wrapped. */
		wraparound_occurred = true;
	} else if (counter_pre_lpm_ticks == post_lpm_ticks) {
		/*
		 * Pre == Post: either no time passed or exactly one full period did. The latter
		 * is only possible if the alarm expired.
		 */
		wraparound_occurred = alarm_expired;
	} else {
		/*
		 * Pre < Post: usually means no wraparound. However, if the alarm was scheduled
		 * far enough to cross the top value and it expired, then the counter wrapped and
		 * only landed above its starting value because of the latency of waking up.
		 */
		wraparound_occurred = alarm_expired && alarm_target_ticks >= counter_top;
	}

	if (wraparound_occurred) {
		ticks_elapsed = (counter_top - counter_pre_lpm_ticks) + post_lpm_ticks + 1;
	} else {
		ticks_elapsed = post_lpm_ticks - counter_pre_lpm_ticks;
	}

	return (uint64_t)counter_ticks_to_us(lpm_counter, ticks_elapsed);
}
