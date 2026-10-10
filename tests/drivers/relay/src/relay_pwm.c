/*
 * Copyright (c) 2026 Siemens AG
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * PWM relay backend test.
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/pwm/pwm_fake.h>
#include <zephyr/drivers/relay/relay.h>
#include <zephyr/fff.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#if DT_HAS_COMPAT_STATUS_OKAY(zephyr_pwm_relay)

DEFINE_FFF_GLOBALS;

#define RELAY_SIMPLE  DT_NODELABEL(relay_pwm_simple)
#define RELAY_PROFILE DT_NODELABEL(relay_pwm_profile)
#define RELAY_REFRESH DT_NODELABEL(relay_pwm_refresh)

BUILD_ASSERT(DT_NODE_EXISTS(RELAY_SIMPLE) && DT_NODE_EXISTS(RELAY_PROFILE) &&
		     DT_NODE_EXISTS(RELAY_REFRESH),
	     "board overlay must define relay_pwm_simple, relay_pwm_profile and "
	     "relay_pwm_refresh");

/* Time to let the reconcile work item run after a state change. */
#define SETTLE_MS           20
/* Pull-in window of the profile relay, taken from the overlay. */
#define PROFILE_PULL_IN_MS  DT_PROP(RELAY_PROFILE, pull_in_time_ms)
/* Timing and hold duty of the refresh relay, taken from the overlay. */
#define REFRESH_PULL_IN_MS  DT_PROP(RELAY_REFRESH, pull_in_time_ms)
#define REFRESH_INTERVAL_MS DT_PROP(RELAY_REFRESH, refresh_interval_ms)
#define REFRESH_HOLD_DUTY   DT_PROP(RELAY_REFRESH, hold_duty_percent)

static const struct device *const relay_simple = DEVICE_DT_GET(RELAY_SIMPLE);
static const struct device *const relay_profile = DEVICE_DT_GET(RELAY_PROFILE);
static const struct device *const relay_refresh = DEVICE_DT_GET(RELAY_REFRESH);

/* Duty last programmed on the coil, as a percentage of the PWM period. */
static uint32_t last_duty_percent(void)
{
	uint32_t period = fake_pwm_set_cycles_fake.arg2_val;
	uint32_t pulse = fake_pwm_set_cycles_fake.arg3_val;

	zassert_true(period > 0, "no PWM period programmed");
	return (uint32_t)(((uint64_t)pulse * 100) / period);
}

ZTEST(relay_pwm, test_devices_ready)
{
	zassert_true(device_is_ready(relay_simple), "simple relay not ready");
	zassert_true(device_is_ready(relay_profile), "profile relay not ready");
	zassert_true(device_is_ready(relay_refresh), "refresh relay not ready");
}

ZTEST(relay_pwm, test_simple_on_off)
{
	enum relay_state state;

	zassert_ok(relay_set_state(relay_simple, RELAY_STATE_ON), "set on failed");
	k_msleep(SETTLE_MS);
	zassert_true(fake_pwm_set_cycles_fake.call_count > 0, "coil not driven");
	zassert_equal(last_duty_percent(), 100, "on should hold the coil at full duty");
	zassert_ok(relay_get_state(relay_simple, &state), "get_state failed");
	zassert_equal(state, RELAY_STATE_ON, "state should read back on");

	zassert_ok(relay_set_state(relay_simple, RELAY_STATE_OFF), "set off failed");
	k_msleep(SETTLE_MS);
	zassert_equal(last_duty_percent(), 0, "off should release the coil");
	zassert_ok(relay_get_state(relay_simple, &state), "get_state failed");
	zassert_equal(state, RELAY_STATE_OFF, "state should read back off");
}

ZTEST(relay_pwm, test_profile_pull_in_then_hold)
{
	enum relay_state state;

	zassert_ok(relay_set_state(relay_profile, RELAY_STATE_ON), "set on failed");

	/* Still inside the pull-in window: the coil is driven at pull-in duty. */
	k_msleep(SETTLE_MS);
	zassert_equal(last_duty_percent(), 100, "pull-in should drive full duty");

	/* After the pull-in window it drops to the lower hold duty. */
	k_msleep(PROFILE_PULL_IN_MS);
	zassert_equal(last_duty_percent(), 50, "hold should drive the hold duty");
	zassert_ok(relay_get_state(relay_profile, &state), "get_state failed");
	zassert_equal(state, RELAY_STATE_ON, "state should stay on across phases");

	zassert_ok(relay_set_state(relay_profile, RELAY_STATE_OFF), "set off failed");
	k_msleep(SETTLE_MS);
	zassert_equal(last_duty_percent(), 0, "off should release the coil");
	zassert_ok(relay_get_state(relay_profile, &state), "get_state failed");
	zassert_equal(state, RELAY_STATE_OFF, "state should read back off");
}

ZTEST(relay_pwm, test_refresh_repulses_coil)
{
	zassert_ok(relay_set_state(relay_refresh, RELAY_STATE_ON), "set on failed");

	/* Initial pull-in pulse. */
	k_msleep(SETTLE_MS);
	zassert_equal(last_duty_percent(), 100, "pull-in should drive full duty");

	/* Settles to the hold duty after the pull-in window. */
	k_msleep(REFRESH_PULL_IN_MS);
	zassert_equal(last_duty_percent(), REFRESH_HOLD_DUTY, "should settle to hold duty");

	/* A refresh re-applies the pull-in pulse. */
	k_msleep(REFRESH_INTERVAL_MS);
	zassert_equal(last_duty_percent(), 100, "refresh should re-pulse at pull-in duty");

	/* And drops back to the hold duty after the pull-in window. */
	k_msleep(REFRESH_PULL_IN_MS);
	zassert_equal(last_duty_percent(), REFRESH_HOLD_DUTY, "should settle back to hold duty");

	zassert_ok(relay_set_state(relay_refresh, RELAY_STATE_OFF), "set off failed");
	k_msleep(SETTLE_MS);
	zassert_equal(last_duty_percent(), 0, "off should release the coil");
}

ZTEST(relay_pwm, test_drive_error_reaches_caller)
{
	enum relay_state state;

	/* A coil-drive failure in the commanded transition reaches the caller
	 * synchronously instead of being reported as success.
	 */
	fake_pwm_set_cycles_fake.return_val = -EIO;
	zassert_equal(relay_set_state(relay_simple, RELAY_STATE_ON), -EIO,
		      "set_state should return the coil-drive error");

	/* The latched failure is also surfaced on read-back. */
	zassert_equal(relay_get_state(relay_simple, &state), -EIO,
		      "get_state should surface the latched drive error");

	/* A subsequent successful drive clears the latch. */
	fake_pwm_set_cycles_fake.return_val = 0;
	zassert_ok(relay_set_state(relay_simple, RELAY_STATE_OFF), "recovery set off failed");
	zassert_ok(relay_get_state(relay_simple, &state), "latched error should clear on success");
	zassert_equal(state, RELAY_STATE_OFF, "state should read back off");
}

ZTEST_SUITE(relay_pwm, NULL, NULL, NULL, NULL, NULL);

#endif /* DT_HAS_COMPAT_STATUS_OKAY(zephyr_pwm_relay) */
