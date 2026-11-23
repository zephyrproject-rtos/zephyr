/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/pm.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/ztest.h>

#include "pwm_pm.h"

/* Long enough for the idle thread to pick a power state many times over */
#define IDLE_TIME K_SECONDS(1)

#if HAS_DISABLING_STATES(PWM_PM_OUT_CTLR)
static const struct pm_state_constraint out_states[] = DISABLING_STATES(PWM_PM_OUT_CTLR);

static atomic_t entries;
static atomic_t stopping_entries;
static atomic_t stopping_state;
static atomic_t stopping_substate;

static bool stops_output(enum pm_state state, uint8_t substate_id)
{
	for (size_t i = 0; i < ARRAY_SIZE(out_states); i++) {
		if (out_states[i].state == state && out_states[i].substate_id == substate_id) {
			return true;
		}
	}

	return false;
}

static void state_entry(enum pm_state state, uint8_t substate_id)
{
	atomic_inc(&entries);

	if (stops_output(state, substate_id)) {
		atomic_set(&stopping_state, state);
		atomic_set(&stopping_substate, substate_id);
		atomic_inc(&stopping_entries);
	}
}

static struct pm_notifier notifier = {
	.substate_entry = state_entry,
	.report_substate = true,
};
#endif /* HAS_DISABLING_STATES(PWM_PM_OUT_CTLR) */

/*
 * The system-managed sweep suspends every device that is not busy before a
 * state deeper than runtime-idle, and pays no attention to policy locks. A
 * driving output stays up only because its lock keeps the states that stop it
 * from being picked at all, so that is what this checks: with the output
 * running, the SoC idles, but never into a state the output node lists in
 * zephyr,disabling-power-states. States it does not list are allowed.
 *
 * A capture taken after waking cannot tell: the sweep resumes the device on
 * the way out, the driver replays the waveform, and the gap is gone.
 */
ZTEST(pwm_pm_sysmanaged, test_output_survives_idle)
{
#if HAS_DISABLING_STATES(PWM_PM_OUT_CTLR)
	struct pwm_pm_reading ref;
	struct pwm_pm_reading now;
	enum pm_device_state state;
	int err;

	pwm_pm_start_output();
	pwm_pm_capture(&ref);

	atomic_clear(&entries);
	atomic_clear(&stopping_entries);

	pm_notifier_register(&notifier);
	k_sleep(IDLE_TIME);
	err = pm_notifier_unregister(&notifier);
	zassert_ok(err, "failed to unregister the pm notifier (err %d)", err);

	zassert_equal(atomic_get(&stopping_entries), 0,
		      "SoC entered state %d/%d %d times with the output running",
		      (int)atomic_get(&stopping_state), (int)atomic_get(&stopping_substate),
		      (int)atomic_get(&stopping_entries));
	zassert_true(atomic_get(&entries) > 0, "the SoC never idled");

	err = pm_device_state_get(pwm_pm_out.dev, &state);
	zassert_ok(err, "%s: no pm state (err %d)", pwm_pm_out.dev->name, err);
	zassert_equal(state, PM_DEVICE_STATE_ACTIVE, "%s is %s after idling",
		      pwm_pm_out.dev->name, pm_device_state_str(state));

	pwm_pm_capture(&now);
	zassert_within(now.period, ref.period, MAX(ref.period / 100U, 1U),
		       "period %u, was %u before idling", now.period, ref.period);
	zassert_within(now.pulse, ref.pulse, MAX(ref.pulse / 100U, 1U),
		       "pulse %u, was %u before idling", now.pulse, ref.pulse);
#else
	TC_PRINT("%s declares no disabling power states\n", pwm_pm_out.dev->name);
	ztest_test_skip();
#endif
}

ZTEST_SUITE(pwm_pm_sysmanaged, pwm_pm_supported, NULL, NULL, pwm_pm_after, NULL);
