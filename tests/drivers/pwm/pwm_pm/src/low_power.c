/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/pm/pm.h>
#include <zephyr/pm/policy.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/ztest.h>

#if defined(CONFIG_SMP)
#include <zephyr/arch/cpu.h>
#endif

#include "pwm_pm.h"

/* Long enough for idle to enter the state under test. */
#define LOW_POWER_MIN_MS 100U
#define LOW_POWER_MARGIN_MS 10U

static atomic_t entries;
static atomic_t entered_state;
static atomic_t entered_substate;
static bool notifier_on;

static void count_state_entry(enum pm_state state, uint8_t substate_id)
{
	atomic_set(&entered_state, state);
	atomic_set(&entered_substate, substate_id);
	atomic_inc(&entries);
}

static struct pm_notifier notifier = {
	.substate_entry = count_state_entry,
	.report_substate = true,
};

static uint8_t get_cpu_id(void)
{
#if defined(CONFIG_SMP)
	return arch_curr_cpu()->id;
#else
	return 0U;
#endif
}

static uint32_t get_low_power_time_ms(const struct pm_state_info *state)
{
	uint64_t need_us = (uint64_t)state->min_residency_us + state->exit_latency_us;
	uint32_t need_ms = (uint32_t)DIV_ROUND_UP(need_us, 1000U);

	if (need_ms < LOW_POWER_MIN_MS) {
		return LOW_POWER_MIN_MS;
	}

	return need_ms + LOW_POWER_MARGIN_MS;
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

static bool within(uint32_t value, uint32_t expected, uint32_t tolerance)
{
	uint32_t delta = (value > expected) ? (value - expected) : (expected - value);

	return delta <= tolerance;
}

static int read_waveform(struct pwm_pm_reading *reading)
{
	uint32_t period;
	uint32_t pulse;
	int err;

	*reading = (struct pwm_pm_reading){0};

	err = pwm_capture_cycles(pwm_pm_in.dev, pwm_pm_in.channel,
				 pwm_pm_in.flags | PWM_CAPTURE_TYPE_PERIOD, &period, &pulse,
				 K_MSEC(100));
	if (err == 0) {
		reading->period = period;
	} else if (err != -ENOTSUP) {
		TC_PRINT("no period on the line (err %d)\n", err);
		return -TC_FAIL;
	}

	err = pwm_capture_cycles(pwm_pm_in.dev, pwm_pm_in.channel,
				 pwm_pm_in.flags | PWM_CAPTURE_TYPE_PULSE, &period, &pulse,
				 K_MSEC(100));
	if (err == 0) {
		reading->pulse = pulse;
	} else if (err != -ENOTSUP) {
		TC_PRINT("no pulse on the line (err %d)\n", err);
		return -TC_FAIL;
	}

	if (reading->period == 0U && reading->pulse == 0U) {
		TC_PRINT("capture not supported\n");
		return -ENOTSUP;
	}

	return TC_PASS;
}

static int start_output(void)
{
	uint64_t cycles_per_sec;
	uint32_t period;
	int err;

	err = pwm_get_cycles_per_sec(pwm_pm_out.dev, pwm_pm_out.channel, &cycles_per_sec);
	if (err != 0) {
		TC_PRINT("failed to get the output clock rate (err %d)\n", err);
		return -TC_FAIL;
	}

	period = cycles_per_sec / 100U;
	err = pwm_set_cycles(pwm_pm_out.dev, pwm_pm_out.channel, period, period / 4U,
			     pwm_pm_out.flags);
	if (err != 0) {
		TC_PRINT("failed to set pwm output (err %d)\n", err);
		return -TC_FAIL;
	}

	k_sleep(K_MSEC(20));
	return TC_PASS;
}

/* Entering a low power state while sampling stops the output, so hold every
 * state locked for the capture.
 */
static int capture_settled(struct pwm_pm_reading *reading)
{
	int ret;

	pm_policy_state_all_lock_get();
	k_sleep(K_MSEC(20));
	ret = read_waveform(reading);
	pm_policy_state_all_lock_put();

	return ret;
}

static int expect_same(const struct pwm_pm_reading *ref, const struct pwm_pm_reading *now)
{
	if (!within(now->period, ref->period, MAX(ref->period / 100U, 1U))) {
		TC_PRINT("period %u, was %u before the low power state\n", now->period,
			 ref->period);
		return -TC_FAIL;
	}

	if (!within(now->pulse, ref->pulse, MAX(ref->pulse / 100U, 1U))) {
		TC_PRINT("pulse %u, was %u before the low power state\n", now->pulse, ref->pulse);
		return -TC_FAIL;
	}

	return TC_PASS;
}

static int check_capture(void)
{
	const uint32_t period_usec = 10000U;
	const uint32_t pulse_usec = period_usec / 2U;
	uint64_t period_capture = 0U;
	uint64_t pulse_capture = 0U;
	int err;

	err = pwm_set(pwm_pm_out.dev, pwm_pm_out.channel, PWM_USEC(period_usec),
		      PWM_USEC(pulse_usec), pwm_pm_out.flags);
	if (err != 0) {
		TC_PRINT("failed to set pwm output (err %d)\n", err);
		return -TC_FAIL;
	}

	/* Wait for the next period to ensure the new period has taken effect before capturing it.
	 * Some PWM IPs only apply the new period after the current one completes.
	 */
	k_sleep(K_USEC(period_usec));

	err = pwm_capture_usec(pwm_pm_in.dev, pwm_pm_in.channel,
			       pwm_pm_in.flags | PWM_CAPTURE_TYPE_PERIOD, &period_capture,
			       &pulse_capture, K_USEC(period_usec * 10U));
	pwm_disable_capture(pwm_pm_in.dev, pwm_pm_in.channel);
	if (err == -ENOTSUP) {
		TC_PRINT("period capture not supported\n");
		return -ENOTSUP;
	}
	if (err != 0) {
		TC_PRINT("failed to capture pwm (err %d)\n", err);
		return -TC_FAIL;
	}

	if (period_capture + (period_usec / 100U) < period_usec ||
	    period_capture > (uint64_t)period_usec + (period_usec / 100U)) {
		TC_PRINT("period capture off by more than 1%%\n");
		return -TC_FAIL;
	}

	err = pwm_capture_usec(pwm_pm_in.dev, pwm_pm_in.channel,
			       pwm_pm_in.flags | PWM_CAPTURE_TYPE_PULSE, &period_capture,
			       &pulse_capture, K_USEC(period_usec * 10U));
	pwm_disable_capture(pwm_pm_in.dev, pwm_pm_in.channel);
	if (err == -ENOTSUP) {
		TC_PRINT("pulse capture not supported\n");
		return -ENOTSUP;
	}
	if (err != 0) {
		TC_PRINT("failed to capture pwm (err %d)\n", err);
		return -TC_FAIL;
	}

	if (pulse_capture + (pulse_usec / 100U) < pulse_usec ||
	    pulse_capture > (uint64_t)pulse_usec + (pulse_usec / 100U)) {
		TC_PRINT("pulse capture off by more than 1%%\n");
		return -TC_FAIL;
	}

	return TC_PASS;
}

static int check_output_across_low_power(const struct pm_state_info *state, uint32_t idle_ms,
					 const struct pwm_pm_reading *ref)
{
	struct pwm_pm_reading now;
	int ret;

	atomic_clear(&entries);
	k_msleep(idle_ms);
	if (atomic_get(&entries) <= 0) {
		TC_PRINT("power state %s was not entered\n", pm_state_to_str(state->state));
		return -TC_FAIL;
	}

	if (atomic_get(&entered_state) != state->state) {
		TC_PRINT("entered state %d, expected %s\n", (int)atomic_get(&entered_state),
			 pm_state_to_str(state->state));
		return -TC_FAIL;
	}

	if (atomic_get(&entered_substate) != state->substate_id) {
		TC_PRINT("entered substate %d, expected %u\n", (int)atomic_get(&entered_substate),
			 state->substate_id);
		return -TC_FAIL;
	}

	ret = capture_settled(&now);
	if (ret != TC_PASS) {
		return ret;
	}

	ret = expect_same(ref, &now);
	if (ret != TC_PASS) {
		return ret;
	}

	pm_policy_state_all_lock_get();
	ret = check_capture();
	if (ret == TC_PASS) {
		/* The next state is entered with the original waveform running. */
		ret = start_output();
	}
	pm_policy_state_all_lock_put();

	return ret;
}

/*
 * Program a waveform and capture it. Then, for each low power state that
 * returns, enter it with that waveform running and capture it again. The
 * driver has to bring the waveform back, and it has to accept a new pulse
 * afterwards.
 */
ZTEST(pwm_pm_low_power, test_output_across_low_power)
{
	const struct pm_state_info *states;
	struct pwm_pm_reading ref;
	uint8_t count = pm_state_cpu_get_all(get_cpu_id(), &states);
	bool tested = false;
	int ret;

	pm_policy_state_all_lock_get();
	ret = check_capture();
	if (ret == -ENOTSUP) {
		pm_policy_state_all_lock_put();
		ztest_test_skip();
	}
	if (ret == TC_PASS) {
		ret = start_output();
	}
	if (ret == TC_PASS) {
		ret = read_waveform(&ref);
	}
	pm_policy_state_all_lock_put();
	zassert_equal(ret, TC_PASS);

	pm_notifier_register(&notifier);
	notifier_on = true;

	for (uint8_t i = 0; i < count; i++) {
		const struct pm_state_info *state = &states[i];
		uint32_t idle_ms;

		if (state->state == PM_STATE_SOFT_OFF) {
			continue;
		}

		idle_ms = get_low_power_time_ms(state);
		tested = true;
		if (state->substate_id == 0U) {
			printk("testing %s for %u ms\n", pm_state_to_str(state->state), idle_ms);
		} else {
			printk("testing %s substate %u for %u ms\n", pm_state_to_str(state->state),
			       state->substate_id, idle_ms);
		}

		lock_other_states(states, count, state, true);
		ret = check_output_across_low_power(state, idle_ms, &ref);
		lock_other_states(states, count, state, false);
		zassert_equal(ret, TC_PASS);
	}

	ret = pm_notifier_unregister(&notifier);
	if (ret != 0) {
		TC_PRINT("failed to unregister the pm notifier (err %d)\n", ret);
		zassert_equal(ret, 0);
	}
	notifier_on = false;
	zassert_true(tested, "no low power state is enabled");
}

static void low_power_after(void *fixture)
{
	if (notifier_on) {
		(void)pm_notifier_unregister(&notifier);
		notifier_on = false;
	}

	pwm_pm_after(fixture);
}

ZTEST_SUITE(pwm_pm_low_power, pwm_pm_supported, NULL, NULL, low_power_after, NULL);
