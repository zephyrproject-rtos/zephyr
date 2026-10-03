/*
 * Copyright (c) 2026 Brandon Edmonds
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdio.h>

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_dummy.h>
#include <zephyr/ztest.h>

static struct sensor_value captured_value;
static unsigned int attr_set_calls;

static int test_sensor_attr_set(const struct device *dev, enum sensor_channel chan,
				enum sensor_attribute attr, const struct sensor_value *value)
{
	ARG_UNUSED(dev);
	zassert_equal(chan, SENSOR_CHAN_AMBIENT_TEMP);
	zassert_equal(attr, SENSOR_ATTR_SAMPLING_FREQUENCY);
	captured_value = *value;
	attr_set_calls++;
	return 0;
}

static DEVICE_API(sensor, test_sensor_api) = {
	.attr_set = test_sensor_attr_set,
};

DEVICE_DEFINE(test_sensor, "test_sensor", NULL, NULL, NULL, NULL, POST_KERNEL,
	      CONFIG_SENSOR_INIT_PRIORITY, &test_sensor_api);

static int set_value(const char *input)
{
	char command[128];

	attr_set_calls = 0;
	snprintf(command, sizeof(command),
		 "sensor attr_set test_sensor ambient_temp sampling_frequency %s", input);
	return shell_execute_cmd(shell_backend_dummy_get_ptr(), command);
}

static void assert_value(const char *input, int32_t val1, int32_t val2)
{
	zassert_ok(set_value(input), "Rejected %s", input);
	zassert_equal(attr_set_calls, 1, "Driver not called for %s", input);
	zassert_equal(captured_value.val1, val1, "Wrong integer part for %s", input);
	zassert_equal(captured_value.val2, val2, "Wrong fraction for %s", input);
}

ZTEST(sensor_shell, test_zero_fraction)
{
	/* The old parser never returned for an all-zero fraction. */
	assert_value("50.0", 50, 0);
	assert_value("50.000000", 50, 0);
}

ZTEST(sensor_shell, test_decimal_fraction)
{
	assert_value("50.005", 50, 5000);
	assert_value("50.000001", 50, 1);
	assert_value("50.1", 50, 100000);
	assert_value("50.08", 50, 80000);
	assert_value("50.123456", 50, 123456);
}

ZTEST(sensor_shell, test_decimal_integer)
{
	assert_value("050.5", 50, 500000);
	assert_value("010.2", 10, 200000);
	assert_value("08.5", 8, 500000);
	assert_value("-050.5", -50, -500000);
	assert_value("050", 50, 0);
}

ZTEST(sensor_shell, test_negative_value)
{
	assert_value("-50.005", -50, -5000);
	assert_value("-0.005", 0, -5000);
}

ZTEST(sensor_shell, test_trailing_zeros)
{
	assert_value("50.1234560", 50, 123456);
	assert_value("50.0000000", 50, 0);
}

ZTEST(sensor_shell, test_invalid_value)
{
	static const char *const inputs[] = {
		"50.1234567", "50.0000001", "50.0x10", "50.-5", "50.+5", "50.", "50.1.2",
		"0x32", "0x32.5",
	};

	for (size_t i = 0; i < ARRAY_SIZE(inputs); i++) {
		zassert_equal(set_value(inputs[i]), -EINVAL, "Accepted %s", inputs[i]);
		zassert_equal(attr_set_calls, 0, "Driver called for %s", inputs[i]);
	}
}

static void *sensor_shell_setup(void)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();

	/* Wait for the initialization of the shell dummy backend. */
	WAIT_FOR(shell_ready(sh), 20000, k_msleep(1));
	zassert_true(shell_ready(sh), "Timed out waiting for dummy shell backend");
	return NULL;
}

ZTEST_SUITE(sensor_shell, NULL, sensor_shell_setup, NULL, NULL, NULL);
