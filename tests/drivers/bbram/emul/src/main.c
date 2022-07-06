/*
 * Copyright 2021 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/bbram.h>
#include <string.h>
#include <ztest.h>

#define BBRAM_NODELABEL DT_NODELABEL(bbram)
#define BBRAM_SIZE DT_PROP(BBRAM_NODELABEL, size)

static void test_get_size(void)
{
	const struct device *dev = DEVICE_DT_GET(BBRAM_NODELABEL);
	size_t size;

	zassert_true(device_is_ready(dev), "Device is not ready");

	zassert_ok(bbram_get_size(dev, &size), NULL);
	zassert_equal(size, BBRAM_SIZE, NULL);
}

static void test_bbram_out_of_bounds(void)
{
	const struct device *dev = DEVICE_DT_GET(BBRAM_NODELABEL);
	uint8_t buffer[BBRAM_SIZE];

	zassert_equal(bbram_read(dev, 0, 0, buffer), -EFAULT, NULL);
	zassert_equal(bbram_read(dev, 0, BBRAM_SIZE + 1, buffer), -EFAULT, NULL);
	zassert_equal(bbram_read(dev, BBRAM_SIZE - 1, 2, buffer), -EFAULT, NULL);
	zassert_equal(bbram_write(dev, 0, 0, buffer), -EFAULT, NULL);
	zassert_equal(bbram_write(dev, 0, BBRAM_SIZE + 1, buffer), -EFAULT, NULL);
	zassert_equal(bbram_write(dev, BBRAM_SIZE - 1, 2, buffer), -EFAULT, NULL);
}

static void test_read_write(void)
{
	const struct device *dev = DEVICE_DT_GET(BBRAM_NODELABEL);
	uint8_t buffer[BBRAM_SIZE];
	uint8_t expected[BBRAM_SIZE];

	for (int i = 0; i < BBRAM_SIZE; ++i) {
		expected[i] = i;
	}
	/* Set and verify content. */
	zassert_ok(bbram_write(dev, 0, BBRAM_SIZE, expected), NULL);
	zassert_ok(bbram_read(dev, 0, BBRAM_SIZE, buffer), NULL);
	zassert_mem_equal(buffer, expected, BBRAM_SIZE, NULL);
}

static void test_set_invalid(void)
{
	const struct device *dev = DEVICE_DT_GET(BBRAM_NODELABEL);

	zassert_equal(bbram_check_invalid(dev), 0, NULL);
	zassert_ok(bbram_emul_set_invalid(dev, true), NULL);
	zassert_equal(bbram_check_invalid(dev), 1, NULL);
	zassert_equal(bbram_check_invalid(dev), 0, NULL);
}

static void test_set_standby(void)
{
	const struct device *dev = DEVICE_DT_GET(BBRAM_NODELABEL);

	zassert_equal(bbram_check_standby_power(dev), 0, NULL);
	zassert_ok(bbram_emul_set_standby_power_state(dev, true), NULL);
	zassert_equal(bbram_check_standby_power(dev), 1, NULL);
	zassert_equal(bbram_check_standby_power(dev), 0, NULL);
}

static void test_set_power(void)
{
	const struct device *dev = DEVICE_DT_GET(BBRAM_NODELABEL);

	zassert_equal(bbram_check_power(dev), 0, NULL);
	zassert_ok(bbram_emul_set_power_state(dev, true), NULL);
	zassert_equal(bbram_check_power(dev), 1, NULL);
	zassert_equal(bbram_check_power(dev), 0, NULL);
}

static void test_reset_invalid_on_read(void)
{
	const struct device *dev = DEVICE_DT_GET(BBRAM_NODELABEL);
	uint8_t buffer[BBRAM_SIZE];

	zassert_ok(bbram_emul_set_invalid(dev, true), NULL);
	zassert_equal(bbram_read(dev, 0, BBRAM_SIZE, buffer), -EFAULT, NULL);
	zassert_equal(bbram_check_invalid(dev), 0, NULL);
}

static void test_reset_invalid_on_write(void)
{
	const struct device *dev = DEVICE_DT_GET(BBRAM_NODELABEL);
	uint8_t buffer[BBRAM_SIZE];

	zassert_ok(bbram_emul_set_invalid(dev, true), NULL);
	zassert_equal(bbram_write(dev, 0, BBRAM_SIZE, buffer), -EFAULT, NULL);
	zassert_equal(bbram_check_invalid(dev), 0, NULL);
}

static void setup(void)
{
	const struct device *dev = DEVICE_DT_GET(BBRAM_NODELABEL);

	bbram_emul_set_invalid(dev, false);
	bbram_emul_set_standby_power_state(dev, false);
	bbram_emul_set_power_state(dev, false);
}

void test_main(void)
{
	ztest_test_suite(
		bbram,
		ztest_unit_test_setup_teardown(test_get_size, setup, unit_test_noop),
		ztest_unit_test_setup_teardown(test_bbram_out_of_bounds, setup, unit_test_noop),
		ztest_unit_test_setup_teardown(test_read_write, setup, unit_test_noop),
		ztest_unit_test_setup_teardown(test_set_invalid, setup, unit_test_noop),
		ztest_unit_test_setup_teardown(test_set_standby, setup, unit_test_noop),
		ztest_unit_test_setup_teardown(test_set_power, setup, unit_test_noop),
		ztest_unit_test_setup_teardown(test_reset_invalid_on_read, setup, unit_test_noop),
		ztest_unit_test_setup_teardown(test_reset_invalid_on_write, setup, unit_test_noop));
	ztest_run_test_suite(bbram);
}
