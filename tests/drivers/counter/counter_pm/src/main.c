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

#define COUNTER_PM_ACTIVE_MS 20
#define COUNTER_PM_IDLE_MS 100
#define COUNTER_PM_ALARM_MARGIN_MS 200
#define COUNTER_PM_ALARM_SLACK_MS 500
#define COUNTER_PM_SLEEP_ALARM_MS 50

static atomic_t counter_pm_entries;
static struct k_sem counter_pm_alarm_sem;
static int64_t counter_pm_alarm_at_ms;

static void counter_pm_state_entry(enum pm_state state)
{
	ARG_UNUSED(state);
	atomic_inc(&counter_pm_entries);
}

static struct pm_notifier counter_pm_notifier = {
	.state_entry = counter_pm_state_entry,
};

static const struct device *counter_pm_dev(bool lptimer)
{
#if DT_HAS_ALIAS(counter_pm_gptimer)
	if (!lptimer) {
		return DEVICE_DT_GET(DT_ALIAS(counter_pm_gptimer));
	}
#endif
#if DT_HAS_ALIAS(counter_pm_lptimer)
	if (lptimer) {
		return DEVICE_DT_GET(DT_ALIAS(counter_pm_lptimer));
	}
#endif
	ARG_UNUSED(lptimer);

	return NULL;
}

static uint32_t counter_pm_delta(uint32_t before, uint32_t after)
{
	if (after >= before) {
		return after - before;
	}

	return (UINT32_MAX - before) + after + 1U;
}

static void counter_pm_alarm_cb(const struct device *dev, uint8_t chan_id, uint32_t ticks,
				void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(chan_id);
	ARG_UNUSED(ticks);
	ARG_UNUSED(user_data);

	counter_pm_alarm_at_ms = k_uptime_get();
	k_sem_give(&counter_pm_alarm_sem);
}

static void counter_pm_require_running(const struct device *dev)
{
	uint32_t before = 0U;
	uint32_t after = 0U;
	int ret;

	ret = counter_start(dev);
	zassert_ok(ret, "counter_start failed (%d)", ret);

	pm_policy_state_all_lock_get();
	ret = counter_get_value(dev, &before);
	if (ret == 0) {
		k_msleep(COUNTER_PM_ACTIVE_MS);
		ret = counter_get_value(dev, &after);
	}
	pm_policy_state_all_lock_put();
	zassert_ok(ret, "counter_get_value failed (%d)", ret);
	zassert_true(counter_pm_delta(before, after) > 0U, "counter did not advance while active");
}

/* The notifier records that a power state was entered. Callers decide from
 * the counter value whether the clock stopped.
 */
static int64_t counter_pm_idle(const struct device *dev, uint32_t *before, uint32_t *after)
{
	int64_t wall_ms;
	int ret;

	atomic_clear(&counter_pm_entries);
	ret = counter_get_value(dev, before);
	zassert_ok(ret, "counter_get_value failed (%d)", ret);

	wall_ms = k_uptime_get();
	k_msleep(COUNTER_PM_IDLE_MS);
	wall_ms = k_uptime_get() - wall_ms;

	ret = counter_get_value(dev, after);
	zassert_ok(ret, "counter_get_value failed (%d)", ret);
	zassert_true(atomic_get(&counter_pm_entries) > 0, "power state was not entered");

	return wall_ms;
}

static uint32_t counter_pm_window_ticks(const struct device *dev, int64_t wall_ms)
{
	uint32_t ticks;

	zassert_true(wall_ms > 0, "idle window did not advance the system clock");
	ticks = counter_us_to_ticks(dev, (uint64_t)wall_ms * 1000ULL);
	zassert_true(ticks > 4U, "idle window is shorter than a few counter ticks");

	return ticks;
}

static void counter_pm_expect_rolling(const struct device *dev, uint32_t before, uint32_t after,
				      int64_t wall_ms)
{
	uint32_t window = counter_pm_window_ticks(dev, wall_ms);
	uint32_t delta = counter_pm_delta(before, after);
	uint32_t tol = window / 4U;

	if (tol < 2U) {
		tol = 2U;
	}

	zassert_true((uint64_t)delta + tol >= window,
		     "count stalled: delta %u, window %u ticks", delta, window);
	zassert_true(delta <= window + tol, "count jumped: delta %u, window %u ticks", delta,
		     window);
}

static void counter_pm_arm(const struct device *dev, uint32_t delay_ms)
{
	struct counter_alarm_cfg alarm = {
		.callback = counter_pm_alarm_cb,
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

static void counter_pm_wait_alarm(uint32_t timeout_ms)
{
	int ret;

	pm_policy_state_all_lock_get();
	ret = k_sem_take(&counter_pm_alarm_sem, K_MSEC(timeout_ms));
	pm_policy_state_all_lock_put();
	zassert_ok(ret, "alarm did not fire");
}

ZTEST(counter_pm, test_gptimer_counter)
{
	const struct device *dev = counter_pm_dev(false);
	uint32_t before;
	uint32_t after;

	zassume_not_null(dev, "general-purpose timer counter alias is not configured");
	zassume_true(device_is_ready(dev), "general-purpose timer counter device is not ready");

	counter_pm_require_running(dev);
	(void)counter_pm_idle(dev, &before, &after);

	counter_pm_arm(dev, COUNTER_PM_ALARM_MARGIN_MS);
	counter_pm_wait_alarm(COUNTER_PM_ALARM_MARGIN_MS + COUNTER_PM_ALARM_SLACK_MS);
}

ZTEST(counter_pm, test_lptimer_counter)
{
	const struct device *dev = counter_pm_dev(true);
	uint32_t before;
	uint32_t after;
	int64_t wall_ms;

	zassume_not_null(dev, "low-power timer counter alias is not configured");
	zassume_true(device_is_ready(dev), "low-power timer counter device is not ready");

	counter_pm_require_running(dev);
	wall_ms = counter_pm_idle(dev, &before, &after);
	counter_pm_expect_rolling(dev, before, after, wall_ms);

	counter_pm_arm(dev, COUNTER_PM_ALARM_MARGIN_MS);
	counter_pm_wait_alarm(COUNTER_PM_ALARM_MARGIN_MS + COUNTER_PM_ALARM_SLACK_MS);
}

/* Arm before the idle window and report when the callback runs. The result is
 * the log line: an early wake, a callback after the kernel wake, or a drop.
 */
static void counter_pm_probe_lptimer_alarm(const struct device *dev, uint32_t alarm_ms,
					    const char *which)
{
	uint32_t before;
	uint32_t after;
	int64_t armed_at;
	int64_t woke_at;
	int ret;

	armed_at = k_uptime_get();
	counter_pm_arm(dev, alarm_ms);
	(void)counter_pm_idle(dev, &before, &after);
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
		printk("low-power timer alarm %s: requested %u ms, callback ran at %d ms, "
		       "wake at %d ms\n",
		       which, alarm_ms, (int)(counter_pm_alarm_at_ms - armed_at),
		       (int)(woke_at - armed_at));
	} else {
		printk("low-power timer alarm %s: requested %u ms, callback dropped, "
		       "wake at %d ms\n", which,
		       alarm_ms, (int)(woke_at - armed_at));
	}
}

ZTEST(counter_pm, test_lptimer_alarm_across_sleep)
{
	const struct device *dev = counter_pm_dev(true);

	zassume_not_null(dev, "low-power timer counter alias is not configured");
	zassume_true(device_is_ready(dev), "low-power timer counter device is not ready");

	counter_pm_require_running(dev);
	counter_pm_probe_lptimer_alarm(dev, COUNTER_PM_SLEEP_ALARM_MS, "before kernel wake");
	counter_pm_probe_lptimer_alarm(dev, COUNTER_PM_IDLE_MS + COUNTER_PM_ALARM_MARGIN_MS,
				       "after kernel wake");
}

static void *counter_pm_setup(void)
{
	const struct device *lptimer = counter_pm_dev(true);

	k_sem_init(&counter_pm_alarm_sem, 0, 1);

	if ((lptimer != NULL) && device_is_ready(lptimer) &&
	    pm_device_wakeup_is_capable(lptimer)) {
		zassert_true(pm_device_wakeup_enable(lptimer, true),
			     "could not enable the low-power timer counter as a wakeup source");
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
