/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/misc/infineon_mxcordic/cordic_infineon_mxcordic.h>
#include <zephyr/ztest.h>

/* Test tolerance: 0.01 absolute error, or 1% when scaled by |expected|. */
#define CORDIC_TOL (1.0e-2f)

static const struct device *const cordic_dev = DEVICE_DT_GET(DT_NODELABEL(cordic0));

static void *cordic_setup(void)
{
	zassert_true(device_is_ready(cordic_dev), "CORDIC device not ready");
	return NULL;
}

ZTEST(cordic_mxcordic, test_sin)
{
	float result;

	zassert_ok(cordic_ifx_mxcordic_sin(cordic_dev, 0.0f, &result));
	zassert_within(result, 0.0f, CORDIC_TOL, "sin(0) wrong: %f", (double)result);

	zassert_ok(cordic_ifx_mxcordic_sin(cordic_dev, 0.5235988f /* pi/6 */, &result));
	zassert_within(result, 0.5f, CORDIC_TOL, "sin(pi/6) wrong: %f", (double)result);

	zassert_ok(cordic_ifx_mxcordic_sin(cordic_dev, -0.5235988f, &result));
	zassert_within(result, -0.5f, CORDIC_TOL, "sin(-pi/6) wrong: %f", (double)result);

	zassert_ok(cordic_ifx_mxcordic_sin(cordic_dev, 1.74f, &result));
	zassert_within(result, 0.9857192f, CORDIC_TOL, "sin(1.74) wrong: %f", (double)result);

	zassert_equal(cordic_ifx_mxcordic_sin(cordic_dev, 1.75f, &result), -EINVAL,
		      "sin(1.75) must fail");
}

ZTEST(cordic_mxcordic, test_cos)
{
	float result;

	zassert_ok(cordic_ifx_mxcordic_cos(cordic_dev, 0.0f, &result));
	zassert_within(result, 1.0f, CORDIC_TOL, "cos(0) wrong: %f", (double)result);

	zassert_ok(cordic_ifx_mxcordic_cos(cordic_dev, 1.0471976f /* pi/3 */, &result));
	zassert_within(result, 0.5f, CORDIC_TOL, "cos(pi/3) wrong: %f", (double)result);
}

ZTEST(cordic_mxcordic, test_tan)
{
	float result;

	zassert_ok(cordic_ifx_mxcordic_tan(cordic_dev, 0.0f, &result));
	zassert_within(result, 0.0f, CORDIC_TOL, "tan(0) wrong: %f", (double)result);

	zassert_ok(cordic_ifx_mxcordic_tan(cordic_dev, 0.7853982f /* pi/4 */, &result));
	zassert_within(result, 1.0f, CORDIC_TOL, "tan(pi/4) wrong: %f", (double)result);
}

ZTEST(cordic_mxcordic, test_atan2)
{
	float result;

	zassert_ok(cordic_ifx_mxcordic_atan2(cordic_dev, 1.0f, 1.0f, &result));
	zassert_within(result, 0.7853982f /* pi/4 */, CORDIC_TOL, "atan2(1,1) wrong: %f",
		       (double)result);

	zassert_ok(cordic_ifx_mxcordic_atan2(cordic_dev, 1.0f, -1.0f, &result));
	zassert_within(result, 2.3561945f /* 3pi/4 */, CORDIC_TOL, "atan2(1,-1) wrong: %f",
		       (double)result);

	zassert_ok(cordic_ifx_mxcordic_atan2(cordic_dev, -1.0f, -1.0f, &result));
	zassert_within(result, -2.3561945f, CORDIC_TOL, "atan2(-1,-1) wrong: %f", (double)result);

	zassert_ok(cordic_ifx_mxcordic_atan2(cordic_dev, 1.0f, 0.0f, &result));
	zassert_within(result, 1.5707964f /* pi/2 */, CORDIC_TOL, "atan2(1,0) wrong: %f",
		       (double)result);

	zassert_ok(cordic_ifx_mxcordic_atan2(cordic_dev, 0.0f, 0.0f, &result));
	zassert_within(result, 0.0f, CORDIC_TOL, "atan2(0,0) wrong: %f", (double)result);
}

ZTEST(cordic_mxcordic, test_inverse_trig)
{
	float result;

	zassert_ok(cordic_ifx_mxcordic_asin(cordic_dev, 0.5f, &result));
	zassert_within(result, 0.5235988f /* pi/6 */, CORDIC_TOL, "asin(0.5) wrong: %f",
		       (double)result);

	zassert_ok(cordic_ifx_mxcordic_acos(cordic_dev, 0.5f, &result));
	zassert_within(result, 1.0471976f /* pi/3 */, CORDIC_TOL, "acos(0.5) wrong: %f",
		       (double)result);
}

ZTEST(cordic_mxcordic, test_sinh)
{
	float result, positive_result;

	zassert_ok(cordic_ifx_mxcordic_sinh(cordic_dev, 0.0f, &result));
	zassert_within(result, 0.0f, CORDIC_TOL, "sinh(0) wrong: %f", (double)result);

	zassert_ok(cordic_ifx_mxcordic_sinh(cordic_dev, 1.0f, &result));
	zassert_within(result, 1.1752012f, CORDIC_TOL, "sinh(1) wrong: %f", (double)result);
	positive_result = result;

	zassert_ok(cordic_ifx_mxcordic_sinh(cordic_dev, -1.0f, &result));
	zassert_within(result, -positive_result, CORDIC_TOL, "sinh(-1) must match -sinh(1): %f",
		       (double)result);
}

ZTEST(cordic_mxcordic, test_cosh)
{
	float result;

	zassert_ok(cordic_ifx_mxcordic_cosh(cordic_dev, 0.0f, &result));
	zassert_within(result, 1.0f, CORDIC_TOL, "cosh(0) wrong: %f", (double)result);

	zassert_ok(cordic_ifx_mxcordic_cosh(cordic_dev, 1.0f, &result));
	zassert_within(result, 1.5430806f, CORDIC_TOL, "cosh(1) wrong: %f", (double)result);
}

ZTEST(cordic_mxcordic, test_tanh)
{
	float result;

	zassert_ok(cordic_ifx_mxcordic_tanh(cordic_dev, 0.0f, &result));
	zassert_within(result, 0.0f, CORDIC_TOL, "tanh(0) wrong: %f", (double)result);

	zassert_ok(cordic_ifx_mxcordic_tanh(cordic_dev, 0.7853982f /* pi/4 */, &result));
	zassert_within(result, 0.6557942f, CORDIC_TOL, "tanh(pi/4) wrong: %f", (double)result);
}

ZTEST(cordic_mxcordic, test_atanh)
{
	float result;

	zassert_ok(cordic_ifx_mxcordic_atanh(cordic_dev, 0.0f, &result));
	zassert_within(result, 0.0f, CORDIC_TOL, "atanh(0) wrong: %f", (double)result);

	zassert_ok(cordic_ifx_mxcordic_atanh(cordic_dev, 0.5f, &result));
	zassert_within(result, 0.5493061f, CORDIC_TOL, "atanh(0.5) wrong: %f", (double)result);
}

ZTEST(cordic_mxcordic, test_sqrt)
{
	float result;

	zassert_ok(cordic_ifx_mxcordic_sqrt(cordic_dev, 0.25f, &result));
	zassert_within(result, 0.5f, CORDIC_TOL, "sqrt(0.25) wrong: %f", (double)result);

	zassert_ok(cordic_ifx_mxcordic_sqrt(cordic_dev, 0.81f, &result));
	zassert_within(result, 0.9f, CORDIC_TOL, "sqrt(0.81) wrong: %f", (double)result);

	zassert_ok(cordic_ifx_mxcordic_sqrt(cordic_dev, 1.0f, &result));
	zassert_within(result, 1.0f, CORDIC_TOL, "sqrt(1) wrong: %f", (double)result);
}

ZTEST(cordic_mxcordic, test_magnitude)
{
	float result;

	zassert_ok(cordic_ifx_mxcordic_magnitude(cordic_dev, 3.0f, 4.0f, &result));
	zassert_within(result, 5.0f, 5.0f * CORDIC_TOL, "magnitude(3,4) wrong: %f", (double)result);

	zassert_ok(cordic_ifx_mxcordic_magnitude(cordic_dev, 0.0f, 0.0f, &result));
	zassert_within(result, 0.0f, CORDIC_TOL, "magnitude(0,0) wrong: %f", (double)result);
}

ZTEST(cordic_mxcordic, test_park_transform)
{
	float d, q;

	zassert_ok(cordic_ifx_mxcordic_park_transform(cordic_dev, 0.7853982f /* pi/4 */, 0.1f, 0.5f,
						      &d, &q));
	zassert_within(d, 0.6987454f, CORDIC_TOL, "Park d-axis wrong: %f", (double)d);
	zassert_within(q, 0.4658303f, CORDIC_TOL, "Park q-axis wrong: %f", (double)q);
}

ZTEST(cordic_mxcordic, test_invalid_args)
{
	float result;

	zassert_equal(cordic_ifx_mxcordic_sin(cordic_dev, 0.0f, NULL), -EINVAL);
	zassert_equal(cordic_ifx_mxcordic_sqrt(cordic_dev, 1.5f, &result), -EINVAL);
	zassert_equal(cordic_ifx_mxcordic_sqrt(cordic_dev, 0.0f, &result), -EINVAL);
	zassert_equal(cordic_ifx_mxcordic_atanh(cordic_dev, 0.9f, &result), -EINVAL);
	zassert_equal(cordic_ifx_mxcordic_asin(cordic_dev, 1.1f, &result), -EINVAL);
	zassert_equal(cordic_ifx_mxcordic_acos(cordic_dev, -1.1f, &result), -EINVAL);
	zassert_equal(
		cordic_ifx_mxcordic_park_transform(cordic_dev, 0.0f, 0.0f, 0.0f, NULL, &result),
		-EINVAL);
}

ZTEST_SUITE(cordic_mxcordic, NULL, cordic_setup, NULL, NULL, NULL);
