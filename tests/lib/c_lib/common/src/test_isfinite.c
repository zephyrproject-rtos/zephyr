/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Axonne Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <float.h>
#include <math.h>
#include <stdbool.h>

#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

struct isfinite_test_case {
	long double long_double_value;
	double double_value;
	float float_value;
	bool expected;
};

static const struct isfinite_test_case cases[] = {
	{0.0L, 0.0, 0.0f, true},
	{-0.0L, -0.0, -0.0f, true},
	{1.0L, 1.0, 1.0f, true},
	{-1.0L, -1.0, -1.0f, true},
	{LDBL_MIN, DBL_MIN, FLT_MIN, true},
	{-LDBL_MIN, -DBL_MIN, -FLT_MIN, true},
	{LDBL_MAX, DBL_MAX, FLT_MAX, true},
	{-LDBL_MAX, -DBL_MAX, -FLT_MAX, true},
	{LDBL_MIN / 2.0L, DBL_MIN / 2.0, FLT_MIN / 2.0f, true},
	{-LDBL_MIN / 2.0L, -DBL_MIN / 2.0, -FLT_MIN / 2.0f, true},
	{__builtin_infl(), __builtin_inf(), __builtin_inff(), false},
	{-__builtin_infl(), -__builtin_inf(), -__builtin_inff(), false},
	{__builtin_nanl(""), __builtin_nan(""), __builtin_nanf(""), false},
	{-__builtin_nanl(""), -__builtin_nan(""), -__builtin_nanf(""), false},
};

ZTEST(libc_common, test_isfinite_float)
{
	volatile float value;

	for (size_t i = 0; i < ARRAY_SIZE(cases); i++) {
		value = cases[i].float_value;
		zassert_equal(isfinite(value) != 0, cases[i].expected, "float case %zu", i);
	}

	value = 1.0f;
	zassert_true(isfinite(value++));
	zassert_equal(value, 2.0f, "isfinite must evaluate its argument once");
}

ZTEST(libc_common, test_isfinite_double)
{
	volatile double value;

	for (size_t i = 0; i < ARRAY_SIZE(cases); i++) {
		value = cases[i].double_value;
		zassert_equal(isfinite(value) != 0, cases[i].expected, "double case %zu", i);
	}

	value = 1.0;
	zassert_true(isfinite(value++));
	zassert_equal(value, 2.0, "isfinite must evaluate its argument once");
}

#ifdef TEST_LONG_DOUBLE
ZTEST(libc_common, test_isfinite_long_double)
{
	volatile long double value;

	for (size_t i = 0; i < ARRAY_SIZE(cases); i++) {
		value = cases[i].long_double_value;
		zassert_equal(isfinite(value) != 0, cases[i].expected, "long double case %zu", i);
	}

	value = 1.0L;
	zassert_true(isfinite(value++));
	zassert_equal(value, 2.0L, "isfinite must evaluate its argument once");
}
#endif
