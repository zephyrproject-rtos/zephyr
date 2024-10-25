/*
 * Copyright (c) 2021 Stephanos Ioannidis <root@stephanos.io>
 * Copyright (C) 2010-2021 ARM Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/dsp/macros.h>
#include <zephyr/ztest.h>
#include <zephyr/sys/util.h>

#include <limits.h>

ZTEST(zdsp_macros_q7, test_zdsp_macros_int_to_q7)
{
	q7_t e = 1;

	/* Test a few arbitrary values */
	zassert_within(Q7i(12, 10, 100), -122, e);
	zassert_within(Q7i(120, 10, 234), -3, e);
	zassert_within(Q7i(-1232, -91875010, 129308519), -23, e);
	zassert_within(Q7i(-100000000, -91875010, -129308519), -73, e);

	/* Test the 1:1 conversion scale factor */
	for (int i = INT8_MIN; i <= INT8_MAX; i++) {
		zassert_equal(Q7i(i, INT8_MIN, INT8_MAX), i);
	}

	/* Test edge cases on the output range */
	zassert_equal(Q7i(0, 0, 1), MINq7);
	zassert_equal(Q7i(1, 0, 1), MAXq7);
	zassert_within(Q7i(1, 0, 2), 0, e);
	zassert_equal(Q7i(1000, 1000, 1001), MINq7);
	zassert_equal(Q7i(1001, 1000, 1001), MAXq7);
	zassert_within(Q7i(1001, 1000, 1002), 0, e);
	zassert_equal(Q7i(-1001, -1001, -1000), MINq7);
	zassert_equal(Q7i(-1000, -1001, -1000), MAXq7);
	zassert_within(Q7i(-1001, -1002, -1000), 0, e);

	/* Test boundary cases on the input range */
	zassert_equal(Q7i(0, INT8_MIN, 0), MAXq7);
	zassert_equal(Q7i(0, 0, INT8_MAX), MINq7);
	zassert_equal(Q7i(0, 0, UINT8_MAX), MINq7);
	zassert_equal(Q7i(0, INT16_MIN, 0), MAXq7);
	zassert_equal(Q7i(0, 0, INT16_MAX), MINq7);
	zassert_equal(Q7i(0, 0, UINT16_MAX), MINq7);
	zassert_equal(Q7i(0, INT32_MIN, 0), MAXq7);
	zassert_equal(Q7i(0, 0, INT32_MAX), MINq7);
	zassert_equal(Q7i(0, 0, UINT32_MAX), MINq7);
	zassert_equal(Q7i(0, -(1LL << 54), 0), MAXq7);
	zassert_equal(Q7i(0, 0, (1LL << 54) - 1), MINq7);

	/* Test that saturation happens if aiming above the output range */
	zassert_equal(Q7i(-1, 10, 20), MINq7);
	zassert_equal(Q7i(200, 10, 20), MAXq7);
}

ZTEST(zdsp_macros_q7, test_zdsp_macros_float_to_q7)
{
	q7_t e = 1;

	/* Test floatability for well-known values */
	zassert_within(Q7f(1.0), MAXq7, e);
	zassert_within(Q7f(0.5), MAXq7 / 2, e);
	zassert_within(Q7f(0.25), MAXq7 / 4, e);
	zassert_within(Q7f(0.125), MAXq7 / 8, e);
	zassert_within(Q7f(0.0), 0, e);
	zassert_within(Q7f(-0.125), MINq7 / 8, e);
	zassert_within(Q7f(-0.25), MINq7 / 4, e);
	zassert_within(Q7f(-0.5), MINq7 / 2, e);
	zassert_within(Q7f(-1.0), MINq7, e);

	/* Test saturation */
	zassert_within(Q7f(-1.1), MINq7, e);
	zassert_within(Q7f(-1000000), MINq7, e);
	zassert_within(Q7f(1.1), MAXq7, e);
	zassert_within(Q7f(1000000), MAXq7, e);
}

ZTEST(zdsp_macros_q7, test_zdsp_macros_int_from_q7)
{
	q7_t e = 1;

	/* Test edge cases for selected output ranges */
	zassert_within(INTq7(MINq7, 10, 100), 10, e);
	zassert_within(INTq7(MAXq7, 10, 100), 100, e);
	zassert_within(INTq7(MINq7, -100, -10), -100, e);
	zassert_within(INTq7(MAXq7, -100, -10), -10, e);

	/* Test the 1:1 conversion scale factor */
	for (int i = MINq7; i <= MAXq7; i++) {
		zassert_within(INTq7(i, INT8_MIN, INT8_MAX), i, e);
	}
}

ZTEST(zdsp_macros_q7, test_zdsp_macros_equiv_q7)
{
	/* Acceptable precision related to the change of range */
	int64_t e = ((int64_t)INT32_MAX - (int64_t)INT32_MIN) / (MAXq7 - MINq7);

	/* Test a few selected values within [INT32_MIN, INT32_MAX] */
	zassert_within(INTq7(Q7i(INT32_MIN, INT32_MIN, INT32_MAX), INT32_MIN, INT32_MAX), INT32_MIN, e);
	zassert_within(INTq7(Q7i(-1032, INT32_MIN, INT32_MAX), INT32_MIN, INT32_MAX), -1032, e);
	zassert_within(INTq7(Q7i(0, INT32_MIN, INT32_MAX), INT32_MIN, INT32_MAX), 0, e);
	zassert_within(INTq7(Q7i(1032, INT32_MIN, INT32_MAX), INT32_MIN, INT32_MAX), 1032, e);
	zassert_within(INTq7(Q7i(INT32_MAX, INT32_MIN, INT32_MAX), INT32_MIN, INT32_MAX), INT32_MAX, e);

	/* Test a few selected values within arbitrary ranges */
	zassert_within(INTq7(Q7i(132, 0, 1000), 0, 1000), 132, e);
	zassert_within(INTq7(Q7i(-132, -1000, 0), -1000, 0), -132, e);
	zassert_within(INTq7(Q7i(132, -1000, 1000), -1000, 1000), 132, e);

	/* Test a much larger range */
	for (int64_t i = INT32_MIN; i <= INT32_MAX; i += 1000009) {
		zassert_within(INTq7(Q7i(i, INT32_MIN, INT32_MAX), INT32_MIN, INT32_MAX), i, e);
		zassert_within(INTq7(Q7i(i, i, INT32_MAX), i, INT32_MAX), i, e);
		zassert_within(INTq7(Q7i(i, INT32_MIN, i), INT32_MIN, i), i, e);
	}
}

ZTEST(zdsp_macros_q7, test_zdsp_macros_q15_from_q7)
{
	q15_t e = 1 << 8;

	/* Test through selected values through a whole range for Q.15 */
	zassert_within(Q15q7(Q7f(-1.0)), Q15f(-1.0), e);
	zassert_within(Q15q7(Q7f(-0.3)), Q15f(-0.3), e);
	zassert_within(Q15q7(Q7f(0.0)), Q15f(0.0), e);
	zassert_within(Q15q7(Q7f(0.7)), Q15f(0.7), e);
	zassert_within(Q15q7(Q7f(1.0)), Q15f(1.0), e);
}

ZTEST(zdsp_macros_q7, test_zdsp_macros_q31_from_q7)
{
	q31_t e = 1 << 24;

	/* Test through selected values through a whole range for Q.31 */
	zassert_within(Q31q7(Q7f(-1.0)), Q31f(-1.0), e);
	zassert_within(Q31q7(Q7f(-0.3)), Q31f(-0.3), e);
	zassert_within(Q31q7(Q7f(0.0)), Q31f(0.0), e);
	zassert_within(Q31q7(Q7f(0.7)), Q31f(0.7), e);
	zassert_within(Q31q7(Q7f(1.0)), Q31f(1.0), e);
}

ZTEST(zdsp_macros_q7, test_zdsp_macros_add_q7)
{
	q7_t e = 1;

	/* Test arbitrary values in range */
	zassert_equal(ADDq7(Q7f(0.0), Q7f(0.0)), Q7f(0.0));
	zassert_within(ADDq7(Q7f(0.3), Q7f(0.3)), Q7f(0.6), e);
	zassert_within(ADDq7(Q7f(0.3), Q7f(-0.3)), Q7f(0.0), e);
	zassert_within(ADDq7(Q7f(0.1), Q7f(0.9)), Q7f(1.0), e);
	zassert_within(ADDq7(Q7f(0.1), Q7f(0.2)), Q7f(0.3), e);
	zassert_within(ADDq7(Q7f(0.3123), Q7f(0.4123)), Q7f(0.7246), e);

	/* Test saturation */
	zassert_equal(ADDq7(Q7f(0.9), Q7f(0.5)), Q7f(1.0));
	zassert_equal(ADDq7(Q7f(-0.9), Q7f(-0.5)), Q7f(-1.0));
	zassert_equal(ADDq7(Q7f(1.1), Q7f(1.2)), Q7f(1.0));
	zassert_equal(ADDq7(Q7f(-1.1), Q7f(-1.2)), Q7f(-1.0));
	zassert_within(ADDq7(Q7f(1.1), Q7f(-1.2)), Q7f(0.0), e);
}

ZTEST(zdsp_macros_q7, test_zdsp_macros_sub_q7)
{
	q7_t e = 1;

	/* Test arbitrary values in range */
	zassert_equal(SUBq7(Q7f(0.0), Q7f(0.0)), Q7f(0.0));
	zassert_equal(SUBq7(Q7f(0.3), Q7f(0.3)), Q7f(0.0));
	zassert_within(SUBq7(Q7f(0.1), Q7f(0.9)), Q7f(-0.8), e);
	zassert_within(SUBq7(Q7f(0.1), Q7f(0.2)), Q7f(-0.1), e);
	zassert_within(SUBq7(Q7f(0.3123), Q7f(0.4123)), Q7f(-0.1), e);

	/* Test saturation */
	zassert_within(SUBq7(Q7f(-0.1), Q7f(1.5)), Q7f(-1.0), e);
	zassert_within(SUBq7(Q7f(-1.0), Q7f(0.3)), Q7f(-1.0), e);
	zassert_equal(SUBq7(Q7f(0.2), Q7f(-1.6)), Q7f(1.0));
	zassert_equal(SUBq7(Q7f(-1.0), Q7f(0.4)), Q7f(-1.0));
}

ZTEST(zdsp_macros_q7, test_zdsp_macros_mul_q7)
{
	q7_t e = 1;

	/* Test arbitrary values */
	zassert_within(MULq7(Q7f(0.1), Q7f(0.2)), Q7f(0.02), e);
	zassert_within(MULq7(Q7f(0.2), Q7f(0.2)), Q7f(0.04), e);
	zassert_within(MULq7(Q7f(-0.1), Q7f(-0.2)), Q7f(0.02), e);
	zassert_within(MULq7(Q7f(-0.1), Q7f(-0.2)), Q7f(0.02), e);

	/* Test identity. Note that with fixed-points 1.0 * 1.0 is slightly smaller than 1.0 */
	for (double f = -1.0; f <= 1.0; f += 0.001) {
		zassert_within(MULq7(Q7f(f), Q7f(1.0)), Q7f(f), e);
		zassert_within(MULq7(Q7f(f), Q7f(-1.0)), Q7f(-f), e);
		zassert_within(MULq7(Q7f(-f), Q7f(1.0)), Q7f(-f), e);
		zassert_within(MULq7(Q7f(-f), Q7f(-1.0)), Q7f(f), e);
	}
}

ZTEST(zdsp_macros_q7, test_zdsp_macros_div_q7)
{
	q7_t e = 3;

	/* Test arbitrary values */
	zassert_within(DIVq7(Q7f(0.1), Q7f(0.1)), Q7f(1.0), e);
	zassert_within(DIVq7(Q7f(0.1), Q7f(0.2)), Q7f(0.5), e);
	zassert_within(DIVq7(Q7f(0.1), Q7f(0.4)), Q7f(0.25), e);
	zassert_within(DIVq7(Q7f(0.4), Q7f(0.5)), Q7f(0.8), e);

	/* Test saturation */
	zassert_within(DIVq7(Q7f(1.0), Q7f(0.2)), Q7f(1.0), e);
	zassert_within(DIVq7(Q7f(1.0), Q7f(-0.9)), Q7f(-1.0), e);
	zassert_within(DIVq7(Q7f(-1.0), Q7f(0.6)), Q7f(-1.0), e);
	zassert_within(DIVq7(Q7f(-0.9), Q7f(-0.6)), Q7f(1.0), e);

	/* Test identity */
	for (double f = -1.0; f <= 1.0; f += 0.001) {
		zassert_within(DIVq7(Q7f(f), Q7f(1.0)), Q7f(f), e);
		zassert_within(DIVq7(Q7f(f), Q7f(-1.0)), Q7f(-f), e);
		zassert_within(DIVq7(Q7f(-f), Q7f(1.0)), Q7f(-f), e);
		zassert_within(DIVq7(Q7f(-f), Q7f(-1.0)), Q7f(f), e);
	}
}

ZTEST(zdsp_macros_q7, test_zdsp_macros_neg_q7)
{
	q7_t e = 1;

	/* Test arbitrary values */
	zassert_equal(NEGq7(Q7f(-1.0)), Q7f(1.0));
	zassert_equal(NEGq7(Q7f(-0.3)), Q7f(0.3));
	zassert_equal(NEGq7(Q7f(0.0)), Q7f(0.0));
	zassert_equal(NEGq7(Q7f(0.5)), Q7f(-0.5));
	zassert_within(NEGq7(Q7f(1.0)), Q7f(-1.0), e);
}

ZTEST(zdsp_macros_q7, test_zdsp_macros_abs_q7)
{
	/* Test arbitrary values */
	zassert_equal(ABSq7(Q7f(-1.0)), Q7f(1.0));
	zassert_equal(ABSq7(Q7f(-0.4)), Q7f(0.4));
	zassert_equal(ABSq7(Q7f(0.0)), Q7f(0.0));
	zassert_equal(ABSq7(Q7f(0.4)), Q7f(0.4));
	zassert_equal(ABSq7(Q7f(1.0)), Q7f(1.0));
}

ZTEST(zdsp_macros_q7, test_zdsp_macros_complex_q7)
{
	/*  */
	zassert_equal(ABSq7(Q7f(-1.0)), Q7f(1.0));
}

ZTEST_SUITE(zdsp_macros_q7, NULL, NULL, NULL, NULL, NULL);
