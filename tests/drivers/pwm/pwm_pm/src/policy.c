/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/policy.h>
#include <zephyr/pm/state.h>
#include <zephyr/ztest.h>

#include "pwm_pm.h"

#if HAS_DISABLING_STATES(PWM_PM_OUT_CTLR)
static const struct pm_state_constraint out_states[] = DISABLING_STATES(PWM_PM_OUT_CTLR);
#endif

#if HAS_DISABLING_STATES(PWM_PM_IN_CTLR)
static const struct pm_state_constraint in_states[] = DISABLING_STATES(PWM_PM_IN_CTLR);
#endif

static __maybe_unused void expect_locked(const struct pm_state_constraint *states,
					 size_t count, bool locked, const char *when)
{
	for (size_t i = 0; i < count; i++) {
		zassert_equal(pm_policy_state_lock_is_active(states[i].state,
							     states[i].substate_id),
			      locked, "state %d/%u %s %s", states[i].state,
			      states[i].substate_id, locked ? "not locked" : "still locked", when);
	}
}

static __maybe_unused void dummy_capture(const struct device *dev, uint32_t channel,
					 uint32_t period_cycles, uint32_t pulse_cycles,
					 int status, void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(channel);
	ARG_UNUSED(period_cycles);
	ARG_UNUSED(pulse_cycles);
	ARG_UNUSED(status);
	ARG_UNUSED(user_data);
}

ZTEST(pwm_pm_policy, test_output_holds_power_lock)
{
#if HAS_DISABLING_STATES(PWM_PM_OUT_CTLR)
	pwm_pm_start_output();
	expect_locked(out_states, ARRAY_SIZE(out_states), true, "while driving");

	pwm_pm_action(pwm_pm_out.dev, PM_DEVICE_ACTION_SUSPEND);
	expect_locked(out_states, ARRAY_SIZE(out_states), false, "after suspend");

	/* The replay on resume drives the output again, so the lock comes back. */
	pwm_pm_action(pwm_pm_out.dev, PM_DEVICE_ACTION_RESUME);
	expect_locked(out_states, ARRAY_SIZE(out_states), true, "after resume");
#else
	TC_PRINT("%s declares no disabling power states\n", pwm_pm_out.dev->name);
	ztest_test_skip();
#endif
}

ZTEST(pwm_pm_policy, test_capture_holds_power_lock)
{
#if HAS_DISABLING_STATES(PWM_PM_IN_CTLR)
	pwm_flags_t flags = pwm_pm_in.flags | PWM_CAPTURE_MODE_CONTINUOUS;
	int err;

	/*
	 * Both ends may lock the same states. Suspend the output so that what
	 * is left to hold them is the capture alone.
	 */
	pwm_pm_start_output();
	pwm_pm_action(pwm_pm_out.dev, PM_DEVICE_ACTION_SUSPEND);
	expect_locked(in_states, ARRAY_SIZE(in_states), false, "with nothing running");

	err = pwm_configure_capture(pwm_pm_in.dev, pwm_pm_in.channel,
				    flags | PWM_CAPTURE_TYPE_PERIOD, dummy_capture, NULL);
	if (err == -ENOTSUP) {
		err = pwm_configure_capture(pwm_pm_in.dev, pwm_pm_in.channel,
					    flags | PWM_CAPTURE_TYPE_PULSE, dummy_capture, NULL);
	}
	zassert_ok(err, "failed to configure pwm capture (err %d)", err);

	err = pwm_enable_capture(pwm_pm_in.dev, pwm_pm_in.channel);
	zassert_ok(err, "failed to enable pwm capture (err %d)", err);
	expect_locked(in_states, ARRAY_SIZE(in_states), true, "while capturing");

	err = pwm_disable_capture(pwm_pm_in.dev, pwm_pm_in.channel);
	zassert_ok(err, "failed to disable pwm capture (err %d)", err);
	expect_locked(in_states, ARRAY_SIZE(in_states), false, "after the capture ended");
#else
	TC_PRINT("%s declares no disabling power states\n", pwm_pm_in.dev->name);
	ztest_test_skip();
#endif
}

ZTEST_SUITE(pwm_pm_policy, pwm_pm_supported, NULL, NULL, pwm_pm_after, NULL);
