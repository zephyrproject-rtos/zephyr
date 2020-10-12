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

#include <ztest.h>
#include <drivers/sensor.h>

#define ACCEL_LABEL	DT_LABEL(DT_ALIAS(accel_0))

static enum sensor_channel channel[] = {
	SENSOR_CHAN_ACCEL_X,
	SENSOR_CHAN_ACCEL_Y,
	SENSOR_CHAN_ACCEL_Z,
	SENSOR_CHAN_GYRO_X,
	SENSOR_CHAN_GYRO_Y,
	SENSOR_CHAN_GYRO_Z,
};

void test_sensor_accel_basic(void)
{
	const struct device *dev;

	dev = device_get_binding(ACCEL_LABEL);
	zassert_not_null(dev, "failed: dev '%s' is null", ACCEL_LABEL);

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

/* test case main entry */
void test_main(void)
{
	ztest_test_suite(test_sensor_accel,
			 ztest_user_unit_test(test_sensor_accel_basic));

	ztest_run_test_suite(test_sensor_accel);
}
