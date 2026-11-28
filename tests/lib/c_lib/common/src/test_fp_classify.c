/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <float.h>
#include <math.h>
#include <zephyr/ztest.h>

/* Classify v, of class cls and with sign bit sign. volatile keeps the
 * compiler from folding the classification.
 */
#define CHECK(v, cls, sign)                                                                        \
	do {                                                                                       \
		volatile __typeof__(v) x = (v);                                                    \
                                                                                                   \
		zassert_equal(fpclassify(x), (cls), #v);                                           \
		zassert_equal(isfinite(x) != 0, ((cls) != FP_NAN) && ((cls) != FP_INFINITE), #v);  \
		zassert_equal(isinf(x) != 0, (cls) == FP_INFINITE, #v);                            \
		zassert_equal(isnan(x) != 0, (cls) == FP_NAN, #v);                                 \
		zassert_equal(isnormal(x) != 0, (cls) == FP_NORMAL, #v);                           \
		zassert_equal(signbit(x) != 0, (sign), #v);                                        \
	} while (false)

ZTEST(libc_common, test_fp_classify_float)
{
	CHECK(0.0f, FP_ZERO, false);
	CHECK(-0.0f, FP_ZERO, true);
	CHECK(1.5f, FP_NORMAL, false);
	CHECK(-FLT_MIN, FP_NORMAL, true);
	CHECK(FLT_MIN / 2.0f, FP_SUBNORMAL, false);
	CHECK(-FLT_MIN / 2.0f, FP_SUBNORMAL, true);
	CHECK(INFINITY, FP_INFINITE, false);
	CHECK(-INFINITY, FP_INFINITE, true);
	CHECK(NAN, FP_NAN, false);
	CHECK(-NAN, FP_NAN, true);
}

ZTEST(libc_common, test_fp_classify_double)
{
	CHECK(0.0, FP_ZERO, false);
	CHECK(-0.0, FP_ZERO, true);
	CHECK(1.5, FP_NORMAL, false);
	CHECK(-DBL_MIN, FP_NORMAL, true);
	CHECK(DBL_MIN / 2.0, FP_SUBNORMAL, false);
	CHECK(-DBL_MIN / 2.0, FP_SUBNORMAL, true);
	CHECK((double)INFINITY, FP_INFINITE, false);
	CHECK(-(double)INFINITY, FP_INFINITE, true);
	CHECK((double)NAN, FP_NAN, false);
	CHECK(-(double)NAN, FP_NAN, true);
}
