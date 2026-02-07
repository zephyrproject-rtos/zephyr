/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

#include <zephyr/drivers/sensor.h>

#include "adxl345_features.h"

extern void adxl345_accel_convert(struct sensor_value *val, int16_t sample, uint8_t selected_range);

ZTEST(adxl345_accel_convert, test_convert_10bit_right_justified_2g_mode)
{
	struct sensor_value data[3] = {0};
	int16_t samples[3] = {0, 100, -100};

	/* Convert and check x-axis. */
	adxl345_accel_convert(&data[0], samples[0], ADXL345_RANGE_2G);
	zassert_equal(data[0].val1, 0, "expected 0, was %d", data[0].val1);
	zassert_equal(data[0].val2, 0, "expected 0, was %d", data[0].val2);

	/* Convert and check y-axis. */
	adxl345_accel_convert(&data[1], samples[1], ADXL345_RANGE_2G);
	zassert_equal(data[1].val1, 3, "expected 3, was %d", data[1].val1);
	zassert_equal(data[1].val2, 830700, "expected 830700, was %d", data[1].val2);

	/* Convert and check z-axis. */
	adxl345_accel_convert(&data[2], samples[2], ADXL345_RANGE_2G);
	zassert_equal(data[2].val1, -3, "expected -3, was %d", data[2].val1);
	zassert_equal(data[2].val2, -830700, "expected -830700, was %d", data[2].val2);
}

ZTEST(adxl345_accel_convert, test_convert_10bit_right_justified_4g_mode)
{
	struct sensor_value data[3] = {0};
	int16_t samples[3] = {0, 100, -100};

	/* Convert and check x-axis. */
	adxl345_accel_convert(&data[0], samples[0], ADXL345_RANGE_4G);
	zassert_equal(data[0].val1, 0, "expected 0, was %d", data[0].val1);
	zassert_equal(data[0].val2, 0, "expected 0, was %d", data[0].val2);

	/* Convert and check y-axis. */
	adxl345_accel_convert(&data[1], samples[1], ADXL345_RANGE_4G);
	zassert_equal(data[1].val1, 7, "expected 7, was %d", data[1].val1);
	zassert_equal(data[1].val2, 661400, "expected 661400, was %d", data[1].val2);

	/* Convert and check z-axis. */
	adxl345_accel_convert(&data[2], samples[2], ADXL345_RANGE_4G);
	zassert_equal(data[2].val1, -7, "expected -7, was %d", data[2].val1);
	zassert_equal(data[2].val2, -661400, "expected -661400, was %d", data[2].val2);
}

ZTEST_SUITE(adxl345_accel_convert, NULL, NULL, NULL, NULL, NULL);
