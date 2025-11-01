/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/q31.h>
#include <zephyr/ztest.h>

ZTEST_SUITE(sys_q31_types, NULL, NULL, NULL, NULL, NULL);

ZTEST(sys_q31_types, test_conversions)
{
	zassert_equal(SYS_Q31_NANO(1000000000), INT32_MAX);
	zassert_equal(SYS_Q31_NANO(500000000), INT32_MAX / 2);
	zassert_equal(SYS_Q31_NANO(-1000000000), INT32_MIN);
	zassert_equal(SYS_Q31_NANO(-500000000), INT32_MIN / 2);
	zassert_equal(SYS_Q31_NANO(0), 0);
	zassert_equal(SYS_Q31_MICRO(1000000), INT32_MAX);
	zassert_equal(SYS_Q31_MICRO(500000), INT32_MAX / 2);
	zassert_equal(SYS_Q31_MICRO(-1000000), INT32_MIN);
	zassert_equal(SYS_Q31_MICRO(-500000), INT32_MIN / 2);
	zassert_equal(SYS_Q31_MICRO(0), 0);
	zassert_equal(SYS_Q31_MILLI(1000), INT32_MAX);
	zassert_equal(SYS_Q31_MILLI(500), INT32_MAX / 2);
	zassert_equal(SYS_Q31_MILLI(-1000), INT32_MIN);
	zassert_equal(SYS_Q31_MILLI(-500), INT32_MIN / 2);
	zassert_equal(SYS_Q31_MILLI(0), 0);
	zassert_equal(SYS_Q31_DEC(1), INT32_MAX);
	zassert_equal(SYS_Q31_DEC(-1), INT32_MIN);
	zassert_equal(SYS_Q31_DEC(0), 0);
	zassert_equal(SYS_Q31_INVERT(INT32_MAX), INT32_MIN);
	zassert_equal(SYS_Q31_INVERT(2), -2);
	zassert_equal(SYS_Q31_INVERT(1), -1);
	zassert_equal(SYS_Q31_INVERT(0), 0);
	zassert_equal(SYS_Q31_INVERT(-1), 0);
	zassert_equal(SYS_Q31_INVERT(-2), 1);
	zassert_equal(SYS_Q31_INVERT(INT32_MIN), INT32_MAX);
	zassert_equal(SYS_Q31_SCALE(INT32_MAX, 10), 10);
	zassert_equal(SYS_Q31_SCALE(INT32_MAX / 2, 10), 5);
	zassert_equal(SYS_Q31_SCALE(INT32_MIN, 10), -10);
	zassert_equal(SYS_Q31_SCALE(INT32_MIN / 2, 10), -5);
	zassert_equal(SYS_Q31_SCALE(0, 10), 0);
	zassert_equal(SYS_Q31_RANGE(INT32_MAX, -5, 15), 15);
	zassert_equal(SYS_Q31_RANGE(INT32_MIN, -5, 15), -5);
	zassert_equal(SYS_Q31_RANGE(INT32_MAX / 2, -5, 15), 10);
	zassert_equal(SYS_Q31_RANGE(INT32_MIN / 2, -5, 15), 0);
	zassert_equal(SYS_Q31_RANGE(INT32_MAX, INT32_MIN + 1, INT32_MAX), INT32_MAX);
	zassert_equal(SYS_Q31_RANGE(INT32_MIN, INT32_MIN + 1, INT32_MAX), INT32_MIN + 1);
	zassert_equal(SYS_Q31_RANGE(INT32_MAX / 2, INT32_MIN + 1, INT32_MAX), INT32_MAX / 2);
	zassert_equal(SYS_Q31_RANGE(INT32_MIN / 2, INT32_MIN + 1, INT32_MAX), INT32_MIN / 2);
}
