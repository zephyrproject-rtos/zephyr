/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/ztest.h>

#include "pwm_pm.h"

/* Ten periods of the 10 ms test waveform */
#define CAPTURE_TIMEOUT K_MSEC(100)
/* Two periods, for a new or replayed setup to reach the pin */
#define SETTLE_TIME K_MSEC(20)

#define REPEAT_COUNT 20

const struct pwm_dt_spec pwm_pm_out = PWM_DT_SPEC_GET_BY_IDX(PWM_PM_NODE, 0);
const struct pwm_dt_spec pwm_pm_in = PWM_DT_SPEC_GET_BY_IDX(PWM_PM_NODE, 1);

static atomic_t capture_count;

static int capture_one(pwm_flags_t type, uint32_t *period, uint32_t *pulse)
{
	return pwm_capture_cycles(pwm_pm_in.dev, pwm_pm_in.channel, pwm_pm_in.flags | type,
				  period, pulse, CAPTURE_TIMEOUT);
}

void pwm_pm_start_output(void)
{
	uint64_t cycles_per_sec;
	uint32_t period;
	int err;

	err = pwm_get_cycles_per_sec(pwm_pm_out.dev, pwm_pm_out.channel, &cycles_per_sec);
	zassert_ok(err, "failed to get the output clock rate (err %d)", err);

	period = cycles_per_sec / 100U;
	err = pwm_set_cycles(pwm_pm_out.dev, pwm_pm_out.channel, period, period / 4U,
			     pwm_pm_out.flags);
	zassert_ok(err, "failed to set pwm output (err %d)", err);

	k_sleep(SETTLE_TIME);
}

void pwm_pm_capture(struct pwm_pm_reading *reading)
{
	uint32_t period;
	uint32_t pulse;
	int err;

	*reading = (struct pwm_pm_reading){0};

	err = capture_one(PWM_CAPTURE_TYPE_PERIOD, &period, &pulse);
	if (err == 0) {
		reading->period = period;
	} else {
		zassert_equal(err, -ENOTSUP, "no period on the line (err %d)", err);
	}

	err = capture_one(PWM_CAPTURE_TYPE_PULSE, &period, &pulse);
	if (err == 0) {
		reading->pulse = pulse;
	} else {
		zassert_equal(err, -ENOTSUP, "no pulse on the line (err %d)", err);
	}

	if (reading->period == 0U && reading->pulse == 0U) {
		TC_PRINT("capture not supported\n");
		ztest_test_skip();
	}
}

void pwm_pm_action(const struct device *dev, enum pm_device_action action)
{
	static const enum pm_device_state target[] = {
		[PM_DEVICE_ACTION_SUSPEND] = PM_DEVICE_STATE_SUSPENDED,
		[PM_DEVICE_ACTION_RESUME] = PM_DEVICE_STATE_ACTIVE,
		[PM_DEVICE_ACTION_TURN_OFF] = PM_DEVICE_STATE_OFF,
		[PM_DEVICE_ACTION_TURN_ON] = PM_DEVICE_STATE_SUSPENDED,
	};
	enum pm_device_state state;
	int err;

	err = pm_device_action_run(dev, action);
	if (action == PM_DEVICE_ACTION_TURN_OFF || action == PM_DEVICE_ACTION_TURN_ON) {
		/* The PM core moves the state on even when the driver does not
		 * handle these, since they report a power change that happened
		 * anyway.
		 */
		zassert_true(err == 0 || err == -ENOTSUP, "%s: action %d failed (err %d)",
			     dev->name, action, err);
	} else {
		zassert_ok(err, "%s: action %d failed (err %d)", dev->name, action, err);
	}

	err = pm_device_state_get(dev, &state);
	zassert_ok(err, "%s: no pm state (err %d)", dev->name, err);
	zassert_equal(state, target[action], "%s is %s after action %d", dev->name,
		      pm_device_state_str(state), action);
}

static void expect_no_output(void)
{
	uint32_t period;
	uint32_t pulse;
	int err;

	err = capture_one(PWM_CAPTURE_TYPE_PERIOD, &period, &pulse);
	if (err == -ENOTSUP) {
		err = capture_one(PWM_CAPTURE_TYPE_PULSE, &period, &pulse);
	}

	zassert_equal(err, -EAGAIN, "output still running while suspended (err %d)", err);
}

static void expect_same(const struct pwm_pm_reading *ref, const struct pwm_pm_reading *now)
{
	zassert_within(now->period, ref->period, MAX(ref->period / 100U, 1U),
		       "period %u, was %u before the transition", now->period, ref->period);
	zassert_within(now->pulse, ref->pulse, MAX(ref->pulse / 100U, 1U),
		       "pulse %u, was %u before the transition", now->pulse, ref->pulse);
}

/*
 * None of the tests below calls pwm_set again after the transition: the
 * driver has to bring back the waveform it was asked for.
 */
ZTEST(pwm_pm, test_suspend_stops_and_resume_restores)
{
	struct pwm_pm_reading ref;
	struct pwm_pm_reading now;

	pwm_pm_start_output();
	pwm_pm_capture(&ref);

	pwm_pm_action(pwm_pm_out.dev, PM_DEVICE_ACTION_SUSPEND);
	expect_no_output();

	pwm_pm_action(pwm_pm_out.dev, PM_DEVICE_ACTION_RESUME);
	k_sleep(SETTLE_TIME);

	pwm_pm_capture(&now);
	expect_same(&ref, &now);
}

ZTEST(pwm_pm, test_power_cycle_restores)
{
	struct pwm_pm_reading ref;
	struct pwm_pm_reading now;

	pwm_pm_start_output();
	pwm_pm_capture(&ref);

	pwm_pm_action(pwm_pm_out.dev, PM_DEVICE_ACTION_SUSPEND);
	pwm_pm_action(pwm_pm_out.dev, PM_DEVICE_ACTION_TURN_OFF);
	pwm_pm_action(pwm_pm_out.dev, PM_DEVICE_ACTION_TURN_ON);
	pwm_pm_action(pwm_pm_out.dev, PM_DEVICE_ACTION_RESUME);
	k_sleep(SETTLE_TIME);

	pwm_pm_capture(&now);
	expect_same(&ref, &now);
}

ZTEST(pwm_pm, test_repeated_suspend_resume)
{
	struct pwm_pm_reading ref;
	struct pwm_pm_reading now;

	pwm_pm_start_output();
	pwm_pm_capture(&ref);

	for (int i = 0; i < REPEAT_COUNT; i++) {
		pwm_pm_action(pwm_pm_out.dev, PM_DEVICE_ACTION_SUSPEND);
		pwm_pm_action(pwm_pm_out.dev, PM_DEVICE_ACTION_RESUME);
	}
	k_sleep(SETTLE_TIME);

	pwm_pm_capture(&now);
	expect_same(&ref, &now);
}

static void count_capture(const struct device *dev, uint32_t channel, uint32_t period_cycles,
			  uint32_t pulse_cycles, int status, void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(channel);
	ARG_UNUSED(period_cycles);
	ARG_UNUSED(pulse_cycles);
	ARG_UNUSED(status);
	ARG_UNUSED(user_data);

	atomic_inc(&capture_count);
}

/*
 * A capture does not survive a suspend: the consumer arms it again. What this
 * checks is that the old one is really gone, rather than left armed in
 * hardware to fire into a callback nobody expects, and that a new one works.
 */
ZTEST(pwm_pm, test_capture_device_suspend)
{
	struct pwm_pm_reading ref;
	struct pwm_pm_reading now;
	enum pm_device_state state;
	pwm_flags_t type;
	atomic_val_t seen;
	int err;

	if (pm_device_state_get(pwm_pm_in.dev, &state) == -ENOSYS) {
		TC_PRINT("%s has no device PM\n", pwm_pm_in.dev->name);
		ztest_test_skip();
	}

	if (pwm_pm_in.dev == pwm_pm_out.dev) {
		TC_PRINT("suspending the capture device would stop the output too\n");
		ztest_test_skip();
	}

	pwm_pm_start_output();
	pwm_pm_capture(&ref);

	type = ref.period != 0U ? PWM_CAPTURE_TYPE_PERIOD : PWM_CAPTURE_TYPE_PULSE;
	atomic_clear(&capture_count);

	err = pwm_configure_capture(pwm_pm_in.dev, pwm_pm_in.channel,
				    pwm_pm_in.flags | PWM_CAPTURE_MODE_CONTINUOUS | type,
				    count_capture, NULL);
	zassert_ok(err, "failed to configure pwm capture (err %d)", err);

	err = pwm_enable_capture(pwm_pm_in.dev, pwm_pm_in.channel);
	zassert_ok(err, "failed to enable pwm capture (err %d)", err);

	k_sleep(K_MSEC(50));
	zassert_true(atomic_get(&capture_count) > 0, "continuous capture saw nothing");

	pwm_pm_action(pwm_pm_in.dev, PM_DEVICE_ACTION_SUSPEND);
	pwm_pm_action(pwm_pm_in.dev, PM_DEVICE_ACTION_RESUME);

	seen = atomic_get(&capture_count);
	k_sleep(K_MSEC(50));
	zassert_equal(atomic_get(&capture_count), seen, "capture still running after a suspend");

	pwm_pm_capture(&now);
	expect_same(&ref, &now);
}

bool pwm_pm_supported(const void *global_state)
{
	enum pm_device_state state;

	ARG_UNUSED(global_state);

	if (!pwm_is_ready_dt(&pwm_pm_out) || !pwm_is_ready_dt(&pwm_pm_in)) {
		TC_PRINT("pwm loopback devices are not ready\n");
		return false;
	}

	if (pm_device_state_get(pwm_pm_out.dev, &state) == -ENOSYS) {
		TC_PRINT("%s has no device PM\n", pwm_pm_out.dev->name);
		return false;
	}

	return true;
}

static void resume_if_needed(const struct device *dev)
{
	enum pm_device_state state;

	if (pm_device_state_get(dev, &state) != 0) {
		return;
	}

	if (state == PM_DEVICE_STATE_OFF) {
		(void)pm_device_action_run(dev, PM_DEVICE_ACTION_TURN_ON);
		state = PM_DEVICE_STATE_SUSPENDED;
	}

	if (state == PM_DEVICE_STATE_SUSPENDED) {
		(void)pm_device_action_run(dev, PM_DEVICE_ACTION_RESUME);
	}
}

void pwm_pm_after(void *fixture)
{
	ARG_UNUSED(fixture);

	/* Resume first: a suspended device may not take register access. */
	resume_if_needed(pwm_pm_out.dev);
	resume_if_needed(pwm_pm_in.dev);

	(void)pwm_disable_capture(pwm_pm_in.dev, pwm_pm_in.channel);
}

ZTEST_SUITE(pwm_pm, pwm_pm_supported, NULL, NULL, pwm_pm_after, NULL);
