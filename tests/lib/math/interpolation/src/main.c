/*
 * Copyright (c) 2024 Embeint Inc
 * SPDX-License-Identifier: Apache-2.0
 */

#include <math.h>

#include <zephyr/ztest.h>
#include <zephyr/math/interpolation.h>

ZTEST(interpolation, test_interpolation_oob)
{
	int32_t x_axis[] = {10, 20, 30, 40, 50};
	int32_t y_axis[] = {20, 22, 24, 28, 36};
	uint8_t len = ARRAY_SIZE(x_axis);

	zassert_equal(ARRAY_SIZE(x_axis), ARRAY_SIZE(y_axis));

	zassert_equal(y_axis[0], linear_interpolate(x_axis, y_axis, len, INT32_MIN));
	zassert_equal(y_axis[0], linear_interpolate(x_axis, y_axis, len, -1));
	zassert_equal(y_axis[0], linear_interpolate(x_axis, y_axis, len, 0));
	zassert_equal(y_axis[0], linear_interpolate(x_axis, y_axis, len, x_axis[0] - 1));

	zassert_equal(y_axis[4], linear_interpolate(x_axis, y_axis, len, x_axis[4] + 1));
	zassert_equal(y_axis[4], linear_interpolate(x_axis, y_axis, len, 100));
	zassert_equal(y_axis[4], linear_interpolate(x_axis, y_axis, len, INT32_MAX));
}

ZTEST(interpolation, test_interpolation_on_boundary)
{
	int32_t x_axis[] = {10, 20, 30, 40, 50};
	int32_t y_axis[] = {20, 22, 24, 28, 36};
	uint8_t len = ARRAY_SIZE(x_axis);

	zassert_equal(ARRAY_SIZE(x_axis), ARRAY_SIZE(y_axis));

	/* Looking up x_axis values should return corresponding y_axis */
	for (int i = 0; i < ARRAY_SIZE(x_axis); i++) {
		zassert_equal(y_axis[i], linear_interpolate(x_axis, y_axis, len, x_axis[i]));
	}
}

ZTEST(interpolation, test_interpolation_rounding)
{
	int32_t y_axis[] = {0, 1, 2};
	int32_t x_axis[] = {0, 10, 20};
	uint8_t len = ARRAY_SIZE(x_axis);

	zassert_equal(ARRAY_SIZE(x_axis), ARRAY_SIZE(y_axis));

	/* 0 to 4 -> 0 */
	for (int i = 0; i < 5; i++) {
		zassert_equal(0, linear_interpolate(x_axis, y_axis, len, i));
	}
	/* 5 to 14 -> 1 */
	for (int i = 5; i < 15; i++) {
		zassert_equal(1, linear_interpolate(x_axis, y_axis, len, i));
	}
	/* 15 to N -> 2 */
	for (int i = 15; i <= 20; i++) {
		zassert_equal(2, linear_interpolate(x_axis, y_axis, len, i));
	}
}

ZTEST(interpolation, test_interpolation_simple)
{
	int32_t y_axis[] = {0, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100};
	int32_t x_axis[] = {2000, 2100, 2200, 2300, 2400, 2500, 2600, 2700, 2800, 2900, 3000};
	uint8_t len = ARRAY_SIZE(x_axis);
	int32_t expected;

	zassert_equal(ARRAY_SIZE(x_axis), ARRAY_SIZE(y_axis));

	/* y = (x - 2000) / 10 */
	for (int i = x_axis[0]; i <= x_axis[1]; i++) {
		expected = round((i - 2000.0) / 10.0);
		zassert_equal(expected, linear_interpolate(x_axis, y_axis, len, i));
	}
}

ZTEST(interpolation, test_interpolation_negative_y)
{
	int32_t y_axis[] = {-100, -90, -80, -70, -60, -50, -40, -30, -20, -10, 0};
	int32_t x_axis[] = {2000, 2100, 2200, 2300, 2400, 2500, 2600, 2700, 2800, 2900, 3000};
	uint8_t len = ARRAY_SIZE(x_axis);
	int32_t expected;

	zassert_equal(ARRAY_SIZE(x_axis), ARRAY_SIZE(y_axis));

	/* y = ((x - 2000) / 10) - 100 */
	for (int i = x_axis[0]; i <= x_axis[len - 1]; i++) {
		expected = round((i - 2000.0) / 10.0 - 100.0);
		zassert_equal(expected, linear_interpolate(x_axis, y_axis, len, i));
	}
}

ZTEST(interpolation, test_interpolation_negative_x)
{
	int32_t y_axis[] = {0, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100};
	int32_t x_axis[] = {-3000, -2900, -2800, -2700, -2600, -2500,
			    -2400, -2300, -2200, -2100, -2000};
	uint8_t len = ARRAY_SIZE(x_axis);
	int32_t expected;

	zassert_equal(ARRAY_SIZE(x_axis), ARRAY_SIZE(y_axis));

	/* y = ((x + 3000) / 10) */
	for (int i = x_axis[0]; i <= x_axis[len - 1]; i++) {
		expected = round((i + 3000.0) / 10.0);
		zassert_equal(expected, linear_interpolate(x_axis, y_axis, len, i));
	}
}

ZTEST(interpolation, test_interpolation_negative_xy)
{
	int32_t y_axis[] = {-100, -90, -80, -70, -60, -50, -40, -30, -20, -10, 0};
	int32_t x_axis[] = {-3000, -2900, -2800, -2700, -2600, -2500,
			    -2400, -2300, -2200, -2100, -2000};
	uint8_t len = ARRAY_SIZE(x_axis);
	int32_t expected;

	zassert_equal(ARRAY_SIZE(x_axis), ARRAY_SIZE(y_axis));

	/* y = ((x + 3000) / 10) - 100 */
	for (int i = x_axis[0]; i <= x_axis[len - 1]; i++) {
		expected = round((i + 3000.0) / 10.0 - 100.0);
		zassert_equal(expected, linear_interpolate(x_axis, y_axis, len, i));
	}
}

ZTEST(interpolation, test_interpolation_piecewise)
{
	int32_t y_axis[] = {10, 30, 110, 40, 0};
	int32_t x_axis[] = {100, 150, 200, 250, 300};
	uint8_t len = ARRAY_SIZE(x_axis);
	int32_t expected;

	zassert_equal(ARRAY_SIZE(x_axis), ARRAY_SIZE(y_axis));

	/* First line segment, y = 0.4x - 30 */
	for (int i = x_axis[0]; i <= x_axis[1]; i++) {
		expected = round(0.4 * i - 30.0);
		zassert_equal(expected, linear_interpolate(x_axis, y_axis, len, i));
	}

	/* Second line segment, y = 1.6x - 210 */
	for (int i = x_axis[1]; i <= x_axis[2]; i++) {
		expected = round(1.6 * i - 210.0);
		zassert_equal(expected, linear_interpolate(x_axis, y_axis, len, i));
	}

	/* Third line segment, y = 390 - 1.4x */
	for (int i = x_axis[2]; i <= x_axis[3]; i++) {
		expected = round(390.0 - 1.4 * i);
		zassert_equal(expected, linear_interpolate(x_axis, y_axis, len, i));
	}

	/* Fourth line segment, y = 240 - 0.8x */
	for (int i = x_axis[3]; i <= x_axis[4]; i++) {
		expected = round(240.0 - 0.8 * i);
		zassert_equal(expected, linear_interpolate(x_axis, y_axis, len, i));
	}
}

ZTEST_SUITE(interpolation, NULL, NULL, NULL, NULL, NULL);
