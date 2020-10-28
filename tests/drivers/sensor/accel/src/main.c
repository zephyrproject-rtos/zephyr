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

/* There is no obvious way to pass this to tests, so use a global */
ZTEST_BMEM static const char *accel_label;

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

	dev = device_get_binding(accel_label);
	zassert_not_null(dev, "failed: dev '%s' is null", accel_label);

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
static void run_tests_on_accel(const char *label)
{
	const struct device *accel = device_get_binding(label);

	PRINT("Running tests on '%s'\n", label);
	zassert_not_null(accel, "Unable to get Accelerometer device");
	k_object_access_grant(accel, k_current_get());
	accel_label = label;
	ztest_test_suite(test_sensor_accel,
			 ztest_user_unit_test(test_sensor_accel_basic));
	ztest_run_test_suite(test_sensor_accel);
}

/* test case main entry */
void test_main(void)
{
	run_tests_on_accel(DT_LABEL(DT_ALIAS(accel_0)));

#if DT_NODE_EXISTS(DT_ALIAS(accel_1))
	run_tests_on_accel(DT_LABEL(DT_ALIAS(accel_1)));
#endif
}
