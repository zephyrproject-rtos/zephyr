/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/pm/pm.h>
#include <zephyr/pm/policy.h>

#if defined(CONFIG_SMP)
#include <zephyr/arch/cpu.h>
#endif

#define MCPWM_PM_MIN_IDLE_MS 100
#define MCPWM_PM_IDLE_MARGIN_MS 10
#define MCPWM_PM_OUT_CHANNEL 0U
#define MCPWM_PM_CAPTURE_CHANNEL 6U
#define MCPWM_PM_PERIOD_USEC 10000U
#define MCPWM_PM_PULSE_USEC 2500U
#define MCPWM_PM_PULSE_AFTER_USEC 5000U

static atomic_t mcpwm_pm_entries[PM_STATE_COUNT];

static void count_state_entry(enum pm_state state)
{
	if (state < PM_STATE_COUNT) {
		atomic_inc(&mcpwm_pm_entries[state]);
	}
}

static void reset_state_counts(void)
{
	for (int state = 0; state < PM_STATE_COUNT; state++) {
		atomic_clear(&mcpwm_pm_entries[state]);
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
		atomic_val_t count = atomic_get(&mcpwm_pm_entries[state]);

		if (count > 0) {
			printk("entered %s, count %ld\n",
			       get_state_name((enum pm_state)state), (long)count);
			entered = true;
		}
	}

	return entered;
}

static struct pm_notifier mcpwm_pm_notifier = {
	.state_entry = count_state_entry,
};

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

	if (need_ms < MCPWM_PM_MIN_IDLE_MS) {
		return MCPWM_PM_MIN_IDLE_MS;
	}

	return need_ms + MCPWM_PM_IDLE_MARGIN_MS;
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

static void program_period_and_pulse(const struct device *dev, uint32_t period_usec,
				     uint32_t pulse_usec)
{
	int ret;

	ret = pwm_set(dev, MCPWM_PM_OUT_CHANNEL, PWM_USEC(period_usec), PWM_USEC(pulse_usec),
		      PWM_POLARITY_NORMAL);
	zassert_ok(ret, "pwm_set failed (%d)", ret);
}

static void check_capture(const struct device *dev, uint32_t period_usec, uint32_t pulse_usec)
{
	uint64_t period = 0;
	uint64_t pulse = 0;
	uint32_t period_tol = MAX(period_usec / 100U, 1U);
	uint32_t pulse_tol = MAX(pulse_usec / 100U, 1U);
	int ret;

	/* Stay awake while sampling. Sleep forces the output off. */
	pm_policy_state_all_lock_get();
	k_sleep(K_USEC(period_usec));
	ret = pwm_capture_usec(dev, MCPWM_PM_CAPTURE_CHANNEL,
			       PWM_CAPTURE_TYPE_BOTH | PWM_POLARITY_NORMAL, &period, &pulse,
			       K_USEC(period_usec * 10U));
	(void)pwm_disable_capture(dev, MCPWM_PM_CAPTURE_CHANNEL);
	pm_policy_state_all_lock_put();

	zassert_ok(ret, "capture failed (%d)", ret);
	zassert_within(period, period_usec, period_tol, "period %u usec, captured %" PRIu64,
		       period_usec, period);
	zassert_within(pulse, pulse_usec, pulse_tol, "pulse %u usec, captured %" PRIu64, pulse_usec,
		       pulse);
}

static void check_output_across_sleep(const struct device *dev, const struct pm_state_info *state,
				      uint32_t idle_ms)
{
	program_period_and_pulse(dev, MCPWM_PM_PERIOD_USEC, MCPWM_PM_PULSE_USEC);
	reset_state_counts();
	k_msleep(idle_ms);
	(void)report_state_counts();
	zassert_true(atomic_get(&mcpwm_pm_entries[state->state]) > 0,
		     "power state %s was not entered", get_state_name(state->state));
	check_capture(dev, MCPWM_PM_PERIOD_USEC, MCPWM_PM_PULSE_USEC);
	program_period_and_pulse(dev, MCPWM_PM_PERIOD_USEC, MCPWM_PM_PULSE_AFTER_USEC);
	check_capture(dev, MCPWM_PM_PERIOD_USEC, MCPWM_PM_PULSE_AFTER_USEC);
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

ZTEST(mcpwm_pm, test_capture_across_sleep)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(mcpwm0));

	zassert_true(device_is_ready(dev), "MCPWM device is not ready");

	program_period_and_pulse(dev, MCPWM_PM_PERIOD_USEC, MCPWM_PM_PULSE_USEC);
	check_capture(dev, MCPWM_PM_PERIOD_USEC, MCPWM_PM_PULSE_USEC);
	run_in_each_state(dev, check_output_across_sleep);
}

static void *mcpwm_pm_setup(void)
{
	pm_notifier_register(&mcpwm_pm_notifier);

	return NULL;
}

static void mcpwm_pm_teardown(void *fixture)
{
	ARG_UNUSED(fixture);
	(void)pm_notifier_unregister(&mcpwm_pm_notifier);
}

ZTEST_SUITE(mcpwm_pm, NULL, mcpwm_pm_setup, NULL, NULL, mcpwm_pm_teardown);
