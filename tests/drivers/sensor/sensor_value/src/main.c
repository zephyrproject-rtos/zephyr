/*
 * Copyright (c) 2026 Richard Gudino
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Unit tests for the unit-conversion helpers declared in
 * <zephyr/drivers/sensor.h> (sensor_ms2_to_g(), sensor_rad_to_degrees(),
 * sensor_value_to_double(), ...). These operate purely on struct sensor_value
 * and require no sensor device, so they run on any platform.
 *
 * Expected values are derived from the documented constants
 * SENSOR_G = 9806650 (micro-m/s^2 per g) and SENSOR_PI = 3141592
 * (micro-radians for pi), and were computed by hand.
 */

#include <zephyr/drivers/sensor.h>
#include <zephyr/ztest.h>

#define DOUBLE_EPSILON 1e-6
#define FLOAT_EPSILON  1e-6f

/* Acceleration: m/s^2 <-> g ------------------------------------------------ */

ZTEST(sensor_value, test_ms2_to_g)
{
	/* Exactly 1 g == 9.80665 m/s^2. */
	struct sensor_value one_g = { .val1 = 9, .val2 = 806650 };
	/* Exactly 2 g. */
	struct sensor_value two_g = { .val1 = 19, .val2 = 613300 };
	/* Exactly 0.5 g == 4.903325 m/s^2, exercises round-half-up. */
	struct sensor_value half_g = { .val1 = 4, .val2 = 903325 };
	struct sensor_value zero = { .val1 = 0, .val2 = 0 };
	struct sensor_value neg_one_g = { .val1 = -9, .val2 = -806650 };

	zassert_equal(sensor_ms2_to_g(&one_g), 1, "1 g");
	zassert_equal(sensor_ms2_to_g(&two_g), 2, "2 g");
	zassert_equal(sensor_ms2_to_g(&half_g), 1, "0.5 g rounds to 1");
	zassert_equal(sensor_ms2_to_g(&zero), 0, "0");
	zassert_equal(sensor_ms2_to_g(&neg_one_g), -1, "-1 g");
}

ZTEST(sensor_value, test_g_to_ms2)
{
	struct sensor_value out;

	sensor_g_to_ms2(1, &out);
	zassert_equal(out.val1, 9, "1 g val1");
	zassert_equal(out.val2, 806650, "1 g val2");

	sensor_g_to_ms2(2, &out);
	zassert_equal(out.val1, 19, "2 g val1");
	zassert_equal(out.val2, 613300, "2 g val2");

	sensor_g_to_ms2(-1, &out);
	zassert_equal(out.val1, -9, "-1 g val1");
	zassert_equal(out.val2, -806650, "-1 g val2");

	sensor_g_to_ms2(0, &out);
	zassert_equal(out.val1, 0, "0 g val1");
	zassert_equal(out.val2, 0, "0 g val2");
}

ZTEST(sensor_value, test_ms2_to_mg_and_ug)
{
	struct sensor_value one_g = { .val1 = 9, .val2 = 806650 };
	struct sensor_value two_g = { .val1 = 19, .val2 = 613300 };

	/* 1 g == 1000 mg == 1000000 ug. */
	zassert_equal(sensor_ms2_to_mg(&one_g), 1000, "1 g in mg");
	zassert_equal(sensor_ms2_to_ug(&one_g), 1000000, "1 g in ug");
	zassert_equal(sensor_ms2_to_ug(&two_g), 2000000, "2 g in ug");
}

ZTEST(sensor_value, test_ug_to_ms2)
{
	struct sensor_value out;

	/* 1000000 ug == 1 g == 9.80665 m/s^2. */
	sensor_ug_to_ms2(1000000, &out);
	zassert_equal(out.val1, 9, "1 g val1");
	zassert_equal(out.val2, 806650, "1 g val2");
}

/* Angle: radians <-> degrees ----------------------------------------------- */

ZTEST(sensor_value, test_rad_to_degrees)
{
	/* pi rad == 180 deg. */
	struct sensor_value pi = { .val1 = 3, .val2 = 141592 };
	/* pi/2 rad == 90 deg. */
	struct sensor_value half_pi = { .val1 = 1, .val2 = 570796 };
	struct sensor_value zero = { .val1 = 0, .val2 = 0 };
	struct sensor_value neg_pi = { .val1 = -3, .val2 = -141592 };

	zassert_equal(sensor_rad_to_degrees(&pi), 180, "pi -> 180");
	zassert_equal(sensor_rad_to_degrees(&half_pi), 90, "pi/2 -> 90");
	zassert_equal(sensor_rad_to_degrees(&zero), 0, "0 -> 0");
	zassert_equal(sensor_rad_to_degrees(&neg_pi), -180, "-pi -> -180");
}

ZTEST(sensor_value, test_degrees_to_rad)
{
	struct sensor_value out;

	sensor_degrees_to_rad(180, &out);
	zassert_equal(out.val1, 3, "180 deg val1");
	zassert_equal(out.val2, 141592, "180 deg val2");

	sensor_degrees_to_rad(90, &out);
	zassert_equal(out.val1, 1, "90 deg val1");
	zassert_equal(out.val2, 570796, "90 deg val2");

	sensor_degrees_to_rad(0, &out);
	zassert_equal(out.val1, 0, "0 deg val1");
	zassert_equal(out.val2, 0, "0 deg val2");
}

ZTEST(sensor_value, test_rad_10udegrees_roundtrip)
{
	struct sensor_value pi = { .val1 = 3, .val2 = 141592 };
	struct sensor_value out;

	/* 180 deg in units of 10 micro-degrees == 180 * 100000. */
	zassert_equal(sensor_rad_to_10udegrees(&pi), 18000000, "pi -> 180 deg");

	sensor_10udegrees_to_rad(18000000, &out);
	zassert_equal(out.val1, 3, "180 deg back to rad val1");
	zassert_equal(out.val2, 141592, "180 deg back to rad val2");
}

/* struct sensor_value -> floating point ------------------------------------ */

ZTEST(sensor_value, test_value_to_double)
{
	struct sensor_value a = { .val1 = 1, .val2 = 500000 };
	struct sensor_value b = { .val1 = -1, .val2 = -500000 };
	struct sensor_value zero = { .val1 = 0, .val2 = 0 };

	zassert_within(sensor_value_to_double(&a), 1.5, DOUBLE_EPSILON, "1.5");
	zassert_within(sensor_value_to_double(&b), -1.5, DOUBLE_EPSILON, "-1.5");
	zassert_within(sensor_value_to_double(&zero), 0.0, DOUBLE_EPSILON, "0.0");
}

ZTEST(sensor_value, test_value_to_float)
{
	struct sensor_value a = { .val1 = 2, .val2 = 250000 };

	zassert_within(sensor_value_to_float(&a), 2.25f, FLOAT_EPSILON, "2.25");
}

/* struct sensor_value -> scaled integers ----------------------------------- */

ZTEST(sensor_value, test_value_to_scaled_integers)
{
	struct sensor_value a = { .val1 = 1, .val2 = 500000 };
	struct sensor_value b = { .val1 = -2, .val2 = -250000 };

	zassert_equal(sensor_value_to_deci(&a), 15, "1.5 deci");
	zassert_equal(sensor_value_to_centi(&a), 150, "1.5 centi");
	zassert_equal(sensor_value_to_milli(&a), 1500, "1.5 milli");
	zassert_equal(sensor_value_to_micro(&a), 1500000, "1.5 micro");

	zassert_equal(sensor_value_to_milli(&b), -2250, "-2.25 milli");
	zassert_equal(sensor_value_to_micro(&b), -2250000, "-2.25 micro");
}

ZTEST_SUITE(sensor_value, NULL, NULL, NULL, NULL, NULL);
