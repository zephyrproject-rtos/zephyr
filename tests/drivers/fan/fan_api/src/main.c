/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/fan.h>
#include <zephyr/drivers/pwm/pwm_fake.h>
#include <zephyr/fff.h>
#include <zephyr/ztest.h>

DEFINE_FFF_GLOBALS;

#define TEST_FAN_NODE DT_NODELABEL(test_fan)

/*
 * The fake PWM runs at 10 MHz (100 ns per cycle) and the fan's PWM period
 * cell is 40000 ns, so a full period is 400 cycles.
 */
#define TEST_PERIOD_CYCLES 400U

static const struct device *fan_dev = DEVICE_DT_GET(TEST_FAN_NODE);

static void *fan_setup(void)
{
	zassert_true(device_is_ready(fan_dev), "Fan device is not ready");
	return NULL;
}

ZTEST(fan_api, test_set_speed_drives_pwm_duty)
{
	uint32_t expected_pulse;
	int ret;

	ret = fan_set_speed(fan_dev, 50U);
	zassert_ok(ret, "fan_set_speed(50) failed (%d)", ret);

	zassert_equal(fake_pwm_set_cycles_fake.call_count, 1,
		      "Expected exactly one PWM update");
	zassert_equal(fake_pwm_set_cycles_fake.arg2_val, TEST_PERIOD_CYCLES,
		      "Unexpected PWM period");

	expected_pulse = TEST_PERIOD_CYCLES / 2U;
	zassert_equal(fake_pwm_set_cycles_fake.arg3_val, expected_pulse,
		      "50%% speed should map to half-period pulse");
}

ZTEST(fan_api, test_set_speed_full)
{
	int ret;

	ret = fan_set_speed(fan_dev, FAN_SPEED_MAX);
	zassert_ok(ret, "fan_set_speed(max) failed (%d)", ret);

	zassert_equal(fake_pwm_set_cycles_fake.arg3_val, TEST_PERIOD_CYCLES,
		      "Full speed should map to full-period pulse");
}

ZTEST(fan_api, test_set_speed_zero)
{
	int ret;

	ret = fan_set_speed(fan_dev, 0U);
	zassert_ok(ret, "fan_set_speed(0) failed (%d)", ret);

	zassert_equal(fake_pwm_set_cycles_fake.arg3_val, 0U,
		      "Zero speed should map to zero pulse");
}

ZTEST(fan_api, test_set_speed_out_of_range)
{
	int ret;

	ret = fan_set_speed(fan_dev, FAN_SPEED_MAX + 1U);
	zassert_equal(ret, -EINVAL, "Out-of-range speed should return -EINVAL");

	zassert_equal(fake_pwm_set_cycles_fake.call_count, 0,
		      "Rejected speed must not touch the PWM");
}

ZTEST(fan_api, test_get_speed_reflects_setting)
{
	uint8_t percent;
	int ret;

	ret = fan_set_speed(fan_dev, 75U);
	zassert_ok(ret, "fan_set_speed(75) failed (%d)", ret);

	ret = fan_get_speed(fan_dev, &percent);
	zassert_ok(ret, "fan_get_speed() failed (%d)", ret);
	zassert_equal(percent, 75U, "fan_get_speed() returned %u, expected 75", percent);
}

ZTEST(fan_api, test_get_rpm_unsupported)
{
	uint32_t rpm;
	int ret;

	/* This fan has no tachometer, so RPM readback is unsupported. */
	ret = fan_get_rpm(fan_dev, &rpm);
	zassert_equal(ret, -ENOSYS, "fan_get_rpm() should report -ENOSYS without a tachometer");
}

ZTEST_SUITE(fan_api, NULL, fan_setup, NULL, NULL, NULL);
