/*
 * Copyright 2021 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/bbram.h>
#include <string.h>
#include <zephyr/ztest.h>

#define BBRAM_NODELABEL DT_NODELABEL(bbram)
#define BBRAM_SIZE DT_PROP(BBRAM_NODELABEL, size)

ZTEST(bbram, test_get_size)
{
	const struct device *const dev = DEVICE_DT_GET(BBRAM_NODELABEL);
	size_t size;

	zassert_true(device_is_ready(dev), "Device is not ready");

	zassert_ok(bbram_get_size(dev, &size));
	zassert_equal(size, BBRAM_SIZE);
}

ZTEST(bbram, test_bbram_out_of_bounds)
{
	const struct device *const dev = DEVICE_DT_GET(BBRAM_NODELABEL);
	uint8_t buffer[BBRAM_SIZE];

	zassert_equal(bbram_read(dev, 0, 0, buffer), -EFAULT);
	zassert_equal(bbram_read(dev, 0, BBRAM_SIZE + 1, buffer), -EFAULT);
	zassert_equal(bbram_read(dev, BBRAM_SIZE - 1, 2, buffer), -EFAULT);
	zassert_equal(bbram_write(dev, 0, 0, buffer), -EFAULT);
	zassert_equal(bbram_write(dev, 0, BBRAM_SIZE + 1, buffer), -EFAULT);
	zassert_equal(bbram_write(dev, BBRAM_SIZE - 1, 2, buffer), -EFAULT);
}

ZTEST(bbram, test_read_write)
{
	const struct device *const dev = DEVICE_DT_GET(BBRAM_NODELABEL);
	uint8_t buffer[BBRAM_SIZE];
	uint8_t expected[BBRAM_SIZE];

	for (int i = 0; i < BBRAM_SIZE; ++i) {
		expected[i] = i;
	}
	/* Set and verify content. */
	zassert_ok(bbram_write(dev, 0, BBRAM_SIZE, expected));
	zassert_ok(bbram_read(dev, 0, BBRAM_SIZE, buffer));
	zassert_mem_equal(buffer, expected, BBRAM_SIZE, NULL);
}

ZTEST(bbram, test_set_invalid)
{
	const struct device *const dev = DEVICE_DT_GET(BBRAM_NODELABEL);

	zassert_equal(bbram_check_invalid(dev), 0);
	zassert_ok(bbram_emul_set_invalid(dev, true));
	zassert_equal(bbram_check_invalid(dev), 1);
	zassert_equal(bbram_check_invalid(dev), 0);
}

ZTEST(bbram, test_set_standby)
{
	const struct device *const dev = DEVICE_DT_GET(BBRAM_NODELABEL);

	zassert_equal(bbram_check_standby_power(dev), 0);
	zassert_ok(bbram_emul_set_standby_power_state(dev, true));
	zassert_equal(bbram_check_standby_power(dev), 1);
	zassert_equal(bbram_check_standby_power(dev), 0);
}

ZTEST(bbram, test_set_power)
{
	const struct device *const dev = DEVICE_DT_GET(BBRAM_NODELABEL);

	zassert_equal(bbram_check_power(dev), 0);
	zassert_ok(bbram_emul_set_power_state(dev, true));
	zassert_equal(bbram_check_power(dev), 1);
	zassert_equal(bbram_check_power(dev), 0);
}

ZTEST(bbram, test_reset_invalid_on_read)
{
	const struct device *const dev = DEVICE_DT_GET(BBRAM_NODELABEL);
	uint8_t buffer[BBRAM_SIZE];

	zassert_ok(bbram_emul_set_invalid(dev, true));
	zassert_equal(bbram_read(dev, 0, BBRAM_SIZE, buffer), -EFAULT);
	zassert_equal(bbram_check_invalid(dev), 0);
}

ZTEST(bbram, test_reset_invalid_on_write)
{
	const struct device *const dev = DEVICE_DT_GET(BBRAM_NODELABEL);
	uint8_t buffer[BBRAM_SIZE];

	zassert_ok(bbram_emul_set_invalid(dev, true));
	zassert_equal(bbram_write(dev, 0, BBRAM_SIZE, buffer), -EFAULT);
	zassert_equal(bbram_check_invalid(dev), 0);
}

static void before(void *data)
{
	ARG_UNUSED(data);
	const struct device *const dev = DEVICE_DT_GET(BBRAM_NODELABEL);

	bbram_emul_set_invalid(dev, false);
	bbram_emul_set_standby_power_state(dev, false);
	bbram_emul_set_power_state(dev, false);
}

ZTEST_SUITE(bbram, NULL, NULL, before, NULL, NULL);
