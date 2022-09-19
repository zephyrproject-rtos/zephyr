/*
 * Copyright 2020 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @defgroup driver_sensor_subsys_tests sensor_subsys
 * @ingroup all_tests
 * @{
 * @}
 */

#include <zephyr/ztest.h>
#include <zephyr/drivers/sensor.h>

struct sensor_accel_fixture {
	const struct device *accel_spi;
	const struct device *accel_i2c;
};

static enum sensor_channel channel[] = {
	SENSOR_CHAN_ACCEL_X,
	SENSOR_CHAN_ACCEL_Y,
	SENSOR_CHAN_ACCEL_Z,
	SENSOR_CHAN_GYRO_X,
	SENSOR_CHAN_GYRO_Y,
	SENSOR_CHAN_GYRO_Z,
};

static void test_sensor_accel_basic(const struct device *dev)
{
	zassert_equal(sensor_sample_fetch(dev), 0, "fail to fetch sample");

	for (int i = 0; i < ARRAY_SIZE(channel); i++) {
		struct sensor_value val;

		zassert_ok(sensor_channel_get(dev, channel[i], &val),
			   "fail to get channel");
		zassert_equal(i, val.val1, "expected %d, got %d", i, val.val1);
		zassert_true(val.val2 < 1000, "error %d is too large",
			     val.val2);
	}
}

/* Run all of our tests on an accelerometer device with the given label */
static void run_tests_on_accel(const struct device *accel)
{
	zassert_true(device_is_ready(accel), "Accelerometer device is not ready");

	PRINT("Running tests on '%s'\n", accel->name);
	k_object_access_grant(accel, k_current_get());
}

ZTEST_USER_F(sensor_accel, test_sensor_accel_basic_spi)
{
	run_tests_on_accel(fixture->accel_spi);
	test_sensor_accel_basic(fixture->accel_spi);
}


ZTEST_USER_F(sensor_accel, test_sensor_accel_basic_i2c)
{
	if (fixture->accel_i2c == NULL) {
		ztest_test_skip();
	}

	run_tests_on_accel(fixture->accel_i2c);
	test_sensor_accel_basic(fixture->accel_i2c);
}

static void *sensor_accel_setup(void)
{
	static struct sensor_accel_fixture fixture = {
		.accel_spi = DEVICE_DT_GET(DT_ALIAS(accel_0)),
		.accel_i2c = DEVICE_DT_GET_OR_NULL(DT_ALIAS(accel_1)),
	};

	return &fixture;
}

ZTEST_SUITE(sensor_accel, NULL, sensor_accel_setup, NULL, NULL, NULL);
