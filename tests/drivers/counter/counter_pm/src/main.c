/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/pm/pm.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/policy.h>

#if defined(CONFIG_SMP)
#include <zephyr/arch/cpu.h>
#endif

#define COUNTER_PM_MIN_IDLE_MS 100
#define COUNTER_PM_IDLE_MARGIN_MS 10
#define COUNTER_PM_ALARM_MARGIN_MS 200
#define COUNTER_PM_ALARM_SLACK_MS 500
#define COUNTER_PM_SLEEP_ALARM_MS 50

static atomic_t counter_pm_entries[PM_STATE_COUNT];
static struct k_sem counter_pm_alarm_sem;
static int64_t counter_pm_alarm_at_ms;

static void count_state_entry(enum pm_state state)
{
	if (state < PM_STATE_COUNT) {
		atomic_inc(&counter_pm_entries[state]);
	}
}

static void reset_state_counts(void)
{
	for (int state = 0; state < PM_STATE_COUNT; state++) {
		atomic_clear(&counter_pm_entries[state]);
	}
}

static const char *get_state_name(enum pm_state state)
{
	switch (state) {
	case PM_STATE_ACTIVE:
		return "PM_STATE_ACTIVE";
	case PM_STATE_RUNTIME_IDLE:
		return "PM_STATE_RUNTIME_IDLE";
	case PM_STATE_SUSPEND_TO_IDLE:
		return "PM_STATE_SUSPEND_TO_IDLE";
	case PM_STATE_STANDBY:
		return "PM_STATE_STANDBY";
	case PM_STATE_SUSPEND_TO_RAM:
		return "PM_STATE_SUSPEND_TO_RAM";
	case PM_STATE_SUSPEND_TO_DISK:
		return "PM_STATE_SUSPEND_TO_DISK";
	case PM_STATE_SOFT_OFF:
		return "PM_STATE_SOFT_OFF";
	default:
		return "unknown";
	}
}

static bool report_state_counts(void)
{
	bool entered = false;

	for (int state = 0; state < PM_STATE_COUNT; state++) {
		atomic_val_t count = atomic_get(&counter_pm_entries[state]);

		if (count > 0) {
			printk("entered %s, count %ld\n",
			       get_state_name((enum pm_state)state), (long)count);
			entered = true;
		}
	}

	return entered;
}

static struct pm_notifier counter_pm_notifier = {
	.state_entry = count_state_entry,
};

static const struct device *dev_get(bool lp_counter)
{
#if DT_HAS_ALIAS(counter_pm)
	if (!lp_counter) {
		return DEVICE_DT_GET(DT_ALIAS(counter_pm));
	}
#endif
#if DT_HAS_ALIAS(counter_pm_lp)
	if (lp_counter) {
		return DEVICE_DT_GET(DT_ALIAS(counter_pm_lp));
	}
#endif
	ARG_UNUSED(lp_counter);

	return NULL;
}

static uint32_t get_count_delta(const struct device *dev, uint32_t before, uint32_t after)
{
	uint64_t span = (uint64_t)counter_get_top_value(dev) + 1ULL;

	if (counter_is_counting_up(dev)) {
		if (after >= before) {
			return after - before;
		}

		return (uint32_t)((span - before) + after);
	}

	if (before >= after) {
		return before - after;
	}

	return (uint32_t)(before + (span - after));
}

static void alarm_cb(const struct device *dev, uint8_t chan_id, uint32_t ticks, void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(chan_id);
	ARG_UNUSED(ticks);
	ARG_UNUSED(user_data);

	counter_pm_alarm_at_ms = k_uptime_get();
	k_sem_give(&counter_pm_alarm_sem);
}

static void check_count_running(const struct device *dev)
{
	uint32_t before = 0U;
	uint32_t after = 0U;
	int ret;

	ret = counter_start(dev);
	zassert_ok(ret, "counter_start failed (%d)", ret);

	pm_policy_state_all_lock_get();
	ret = counter_get_value(dev, &before);
	if (ret == 0) {
		k_msleep(CONFIG_COUNTER_PM_ACTIVE_MS);
		ret = counter_get_value(dev, &after);
	}
	pm_policy_state_all_lock_put();
	zassert_ok(ret, "counter_get_value failed (%d)", ret);
	zassert_true(get_count_delta(dev, before, after) > 0U,
		     "counter did not advance while active");
}

static uint8_t get_cpu_id(void)
{
#if defined(CONFIG_SMP)
	return arch_curr_cpu()->id;
#else
	return 0U;
#endif
}

static uint32_t get_sleep_time_ms(const struct pm_state_info *state)
{
	uint64_t need_us = (uint64_t)state->min_residency_us + state->exit_latency_us;
	uint32_t need_ms = (uint32_t)DIV_ROUND_UP(need_us, 1000U);

	if (need_ms < COUNTER_PM_MIN_IDLE_MS) {
		return COUNTER_PM_MIN_IDLE_MS;
	}

	return need_ms + COUNTER_PM_IDLE_MARGIN_MS;
}

static void lock_other_states(const struct pm_state_info *states, uint8_t count,
			      const struct pm_state_info *keep, bool lock)
{
	for (uint8_t i = 0; i < count; i++) {
		if (&states[i] == keep) {
			continue;
		}

		if (lock) {
			pm_policy_state_lock_get(states[i].state, states[i].substate_id);
		} else {
			pm_policy_state_lock_put(states[i].state, states[i].substate_id);
		}
	}
}

/* The notifier records which power state was entered. Callers decide from
 * the counter value whether the clock stopped.
 */
static int64_t read_count_across_sleep(const struct device *dev, uint32_t *before, uint32_t *after,
		    const struct pm_state_info *state, uint32_t idle_ms)
{
	int64_t wall_ms;
	int ret;

	reset_state_counts();
	ret = counter_get_value(dev, before);
	zassert_ok(ret, "counter_get_value failed (%d)", ret);

	wall_ms = k_uptime_get();
	k_msleep(idle_ms);
	wall_ms = k_uptime_get() - wall_ms;

	ret = counter_get_value(dev, after);
	zassert_ok(ret, "counter_get_value failed (%d)", ret);
	(void)report_state_counts();
	zassert_true(atomic_get(&counter_pm_entries[state->state]) > 0,
		     "power state %s was not entered", get_state_name(state->state));

	return wall_ms;
}

static void run_in_each_state(const struct device *dev,
			       void (*fn)(const struct device *dev,
					  const struct pm_state_info *state, uint32_t idle_ms))
{
	const struct pm_state_info *states;
	uint8_t count = pm_state_cpu_get_all(get_cpu_id(), &states);
	bool tested = false;

	for (uint8_t i = 0; i < count; i++) {
		const struct pm_state_info *state = &states[i];
		uint32_t sleep_ms;

		if (state->state == PM_STATE_SOFT_OFF) {
			continue;
		}

		sleep_ms = get_sleep_time_ms(state);
		tested = true;
		if (state->substate_id == 0U) {
			printk("testing %s for %u ms\n", get_state_name(state->state), sleep_ms);
		} else {
			printk("testing %s substate %u for %u ms\n", get_state_name(state->state),
			       state->substate_id, sleep_ms);
		}

		lock_other_states(states, count, state, true);
		fn(dev, state, sleep_ms);
		lock_other_states(states, count, state, false);
	}

	zassert_true(tested, "no idle power state is enabled");
}

static uint32_t sleep_ms_to_ticks(const struct device *dev, int64_t wall_ms)
{
	uint32_t ticks;

	zassert_true(wall_ms > 0, "idle window did not advance the system clock");
	ticks = counter_us_to_ticks(dev, (uint64_t)wall_ms * 1000ULL);
	zassert_true(ticks > 4U, "idle window is shorter than a few counter ticks");

	return ticks;
}

static void check_count_stopped(const struct device *dev, uint32_t before, uint32_t after,
				int64_t wall_ms)
{
	uint32_t window = sleep_ms_to_ticks(dev, wall_ms);
	uint32_t delta = get_count_delta(dev, before, after);
	uint32_t tol = window / 4U;

	if (tol < 2U) {
		tol = 2U;
	}

	zassert_true((uint64_t)delta + tol < window,
		     "count kept moving: delta %u, window %u ticks", delta, window);
}

static void arm_alarm(const struct device *dev, uint32_t delay_ms)
{
	struct counter_alarm_cfg alarm = {
		.callback = alarm_cb,
		.ticks = counter_us_to_ticks(dev, (uint64_t)delay_ms * 1000ULL),
	};
	int ret;

	if (alarm.ticks < 2U) {
		alarm.ticks = 2U;
	}

	(void)counter_cancel_channel_alarm(dev, 0U);
	k_sem_reset(&counter_pm_alarm_sem);
	ret = counter_set_channel_alarm(dev, 0U, &alarm);
	zassert_ok(ret, "counter_set_channel_alarm failed (%d)", ret);
}

static void wait_alarm(uint32_t timeout_ms)
{
	int ret;

	pm_policy_state_all_lock_get();
	ret = k_sem_take(&counter_pm_alarm_sem, K_MSEC(timeout_ms));
	pm_policy_state_all_lock_put();
	zassert_ok(ret, "alarm did not fire");
}

static void check_alarm_after_sleep(const struct device *dev,
					  const struct pm_state_info *state, uint32_t idle_ms)
{
	uint32_t before;
	uint32_t after;
	int64_t wall_ms;

	wall_ms = read_count_across_sleep(dev, &before, &after, state, idle_ms);
	check_count_stopped(dev, before, after, wall_ms);
	arm_alarm(dev, COUNTER_PM_ALARM_MARGIN_MS);
	wait_alarm(COUNTER_PM_ALARM_MARGIN_MS + COUNTER_PM_ALARM_SLACK_MS);
}

ZTEST(counter_pm, test_counter)
{
	const struct device *dev = dev_get(false);

	zassume_not_null(dev, "counter alias is not configured");
	zassume_true(device_is_ready(dev), "counter device is not ready");

	check_count_running(dev);
	run_in_each_state(dev, check_alarm_after_sleep);
}

#if DT_HAS_ALIAS(counter_pm_lp)
static void check_count_across_sleep(const struct device *dev, uint32_t before, uint32_t after,
				     int64_t wall_ms)
{
	uint32_t window = sleep_ms_to_ticks(dev, wall_ms);
	uint32_t delta = get_count_delta(dev, before, after);
	uint32_t tol = window / 4U;

	if (tol < 2U) {
		tol = 2U;
	}

	zassert_true((uint64_t)delta + tol >= window,
		     "count stalled: delta %u, window %u ticks", delta, window);
	zassert_true(delta <= window + tol, "count jumped: delta %u, window %u ticks", delta,
		     window);
}

static void check_lp_count_and_alarm(const struct device *dev, const struct pm_state_info *state,
			     uint32_t idle_ms)
{
	uint32_t before;
	uint32_t after;
	int64_t wall_ms;

	wall_ms = read_count_across_sleep(dev, &before, &after, state, idle_ms);
	check_count_across_sleep(dev, before, after, wall_ms);
	arm_alarm(dev, COUNTER_PM_ALARM_MARGIN_MS);
	wait_alarm(COUNTER_PM_ALARM_MARGIN_MS + COUNTER_PM_ALARM_SLACK_MS);
}

ZTEST(counter_pm, test_lp_counter)
{
	const struct device *dev = dev_get(true);

	zassume_not_null(dev, "low-power counter alias is not configured");
	zassume_true(device_is_ready(dev), "low-power counter device is not ready");

	check_count_running(dev);
	run_in_each_state(dev, check_lp_count_and_alarm);
}

static void check_lp_alarm(const struct device *dev, uint32_t alarm_ms, const char *which,
			   const struct pm_state_info *state, uint32_t idle_ms)
{
	uint32_t before;
	uint32_t after;
	int64_t armed_at;
	int64_t woke_at;
	int ret;

	armed_at = k_uptime_get();
	arm_alarm(dev, alarm_ms);
	(void)read_count_across_sleep(dev, &before, &after, state, idle_ms);
	woke_at = k_uptime_get();

	ret = k_sem_take(&counter_pm_alarm_sem, K_NO_WAIT);
	if (ret != 0) {
		pm_policy_state_all_lock_get();
		ret = k_sem_take(&counter_pm_alarm_sem,
				 K_MSEC(alarm_ms + COUNTER_PM_ALARM_SLACK_MS));
		pm_policy_state_all_lock_put();
	}

	(void)counter_cancel_channel_alarm(dev, 0U);

	if (ret == 0) {
		printk("low-power counter alarm %s: requested %u ms, callback ran at %d ms, "
		       "wake at %d ms\n",
		       which, alarm_ms, (int)(counter_pm_alarm_at_ms - armed_at),
		       (int)(woke_at - armed_at));
	} else {
		printk("low-power counter alarm %s: requested %u ms, callback dropped, "
		       "wake at %d ms\n", which,
		       alarm_ms, (int)(woke_at - armed_at));
	}

	zassert_ok(ret, "low-power counter alarm %s did not fire", which);
}

static void check_lp_alarms(const struct device *dev, const struct pm_state_info *state,
			    uint32_t idle_ms)
{
	check_lp_alarm(dev, COUNTER_PM_SLEEP_ALARM_MS, "before kernel wake", state, idle_ms);
	check_lp_alarm(dev, idle_ms + COUNTER_PM_ALARM_MARGIN_MS, "after kernel wake", state,
		       idle_ms);
}

ZTEST(counter_pm, test_lp_counter_alarm_across_sleep)
{
	const struct device *dev = dev_get(true);

	zassume_not_null(dev, "low-power counter alias is not configured");
	zassume_true(device_is_ready(dev), "low-power counter device is not ready");

	check_count_running(dev);
	run_in_each_state(dev, check_lp_alarms);
}
#endif

static void *counter_pm_setup(void)
{
	const struct device *lp_counter = dev_get(true);

	k_sem_init(&counter_pm_alarm_sem, 0, 1);

	if ((lp_counter != NULL) && device_is_ready(lp_counter) &&
	    pm_device_wakeup_is_capable(lp_counter)) {
		zassert_true(pm_device_wakeup_enable(lp_counter, true),
			     "could not enable the low-power counter as a wakeup source");
	}

	pm_notifier_register(&counter_pm_notifier);

	return NULL;
}

static void counter_pm_teardown(void *fixture)
{
	ARG_UNUSED(fixture);
	(void)pm_notifier_unregister(&counter_pm_notifier);
}

ZTEST_SUITE(counter_pm, NULL, counter_pm_setup, NULL, NULL, counter_pm_teardown);
