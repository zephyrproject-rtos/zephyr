/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <math.h>
#include <zephyr/ztest.h>

/* Compare x and y, where gt, eq, lt and un say whether x > y, x == y,
 * x < y or either is a NaN. volatile keeps the compiler from folding the
 * comparisons.
 */
#define CHECK_CMP(a, b, gt, eq, lt, un)                                                            \
	do {                                                                                       \
		volatile double x = (a);                                                           \
		volatile double y = (b);                                                           \
                                                                                                   \
		zassert_equal(isgreater(x, y) != 0, (gt), #a ", " #b);                             \
		zassert_equal(isgreaterequal(x, y) != 0, (gt) || (eq), #a ", " #b);                \
		zassert_equal(isless(x, y) != 0, (lt), #a ", " #b);                                \
		zassert_equal(islessequal(x, y) != 0, (lt) || (eq), #a ", " #b);                   \
		zassert_equal(islessgreater(x, y) != 0, (lt) || (gt), #a ", " #b);                 \
		zassert_equal(isunordered(x, y) != 0, (un), #a ", " #b);                           \
	} while (false)

ZTEST(libc_common, test_fp_compare)
{
	CHECK_CMP(2.0, 1.0, true, false, false, false);
	CHECK_CMP(1.0, 2.0, false, false, true, false);
	CHECK_CMP(1.0, 1.0, false, true, false, false);
	CHECK_CMP(0.0, -0.0, false, true, false, false);
	CHECK_CMP(-(double)INFINITY, (double)INFINITY, false, false, true, false);
	CHECK_CMP((double)NAN, 1.0, false, false, false, true);
	CHECK_CMP(1.0, (double)NAN, false, false, false, true);
	CHECK_CMP((double)NAN, (double)NAN, false, false, false, true);
}

ZTEST(libc_common, test_fabs_copysign)
{
	volatile double neg_zero = -0.0;
	volatile double neg_inf = -(double)INFINITY;
	volatile double neg_nan = -(double)NAN;
	volatile float neg_zero_f = -0.0f;
	volatile float neg_nan_f = -NAN;

	zassert_equal(fabs(-1.5), 1.5);
	zassert_equal(fabs(neg_inf), (double)INFINITY);
	zassert_false(signbit(fabs(neg_zero)));
	zassert_false(signbit(fabs(neg_nan)));
	zassert_true(fabsf(-1.5f) == 1.5f);
	zassert_false(signbit(fabsf(neg_zero_f)));
	zassert_false(signbit(fabsf(neg_nan_f)));

	zassert_equal(copysign(1.5, -2.0), -1.5);
	zassert_equal(copysign(-1.5, 2.0), 1.5);
	zassert_true(signbit(copysign(0.0, neg_zero)));
	zassert_false(signbit(copysign(neg_zero, 1.0)));
	zassert_true(signbit(copysign((double)NAN, -1.0)));
	zassert_true(isnan(copysign(neg_nan, 1.0)) && !signbit(copysign(neg_nan, 1.0)));
	zassert_equal(copysign((double)INFINITY, -1.0), -(double)INFINITY);
	zassert_true(copysignf(1.5f, -2.0f) == -1.5f);
	zassert_true(signbit(copysignf(0.0f, neg_zero_f)));
	zassert_false(signbit(copysignf(neg_nan_f, 1.0f)));
}
