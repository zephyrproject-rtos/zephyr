/* Copyright (c) 2022 Google Inc
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/math/fp.h>
#include <zephyr/ztest.h>

#ifdef CONFIG_MATH_UTIL_FLOAT
#define TEST_EPSILON (1.0e-6f)
#else
/* For every 2 bits of float precision double the epsilon */
#define TEST_EPSILON (INT32_C(1) << (CONFIG_MATH_UTIL_FP_BITS / 2))
#endif

ZTEST_SUITE(fpmath, nullptr, nullptr, nullptr, nullptr, nullptr);

ZTEST(fpmath, test_int_addition)
{
	fp_t expect;
	fp_t result;

	expect = INT_TO_FP(22);
	result = INT_TO_FP(5) + INT_TO_FP(17);
	zassert_within(expect, result, TEST_EPSILON,
		       "Expected %" PRIf " to be within %" PRIf " of %" PRIf, PRIf_ARG(result),
		       PRIf_ARG(TEST_EPSILON), PRIf_ARG(expect));

	expect = INT_TO_FP(126);
	result = INT_TO_FP(100) + INT_TO_FP(26);
	zassert_within(expect, result, TEST_EPSILON,
		       "Expected %" PRIf " to be within %" PRIf " of %" PRIf, PRIf_ARG(result),
		       PRIf_ARG(TEST_EPSILON), PRIf_ARG(expect));

	expect = INT_TO_FP(-17);
	result = INT_TO_FP(30) - INT_TO_FP(47);
	zassert_within(expect, result, TEST_EPSILON,
		       "Expected %" PRIf " to be within %" PRIf " of %" PRIf, PRIf_ARG(result),
		       PRIf_ARG(TEST_EPSILON), PRIf_ARG(expect));
}

ZTEST(fpmath, test_int_negation)
{
	fp_t expect;
	fp_t result;

	expect = INT_TO_FP(7);
	result = -INT_TO_FP(-7);
	zassert_within(expect, result, TEST_EPSILON,
		       "Expected %" PRIf " to be within %" PRIf " of %" PRIf, PRIf_ARG(result),
		       PRIf_ARG(TEST_EPSILON), PRIf_ARG(expect));

	expect = INT_TO_FP(-102);
	result = -INT_TO_FP(102);
	zassert_within(expect, result, TEST_EPSILON,
		       "Expected %" PRIf " to be within %" PRIf " of %" PRIf, PRIf_ARG(result),
		       PRIf_ARG(TEST_EPSILON), PRIf_ARG(expect));
}

ZTEST(fpmath, test_fp_addition)
{
	fp_t expect;
	fp_t result;

	expect = FLOAT_TO_FP(22.63f);
	result = FLOAT_TO_FP(5.3f) + FLOAT_TO_FP(17.33f);
	zassert_within(expect, result, TEST_EPSILON,
		       "Expected %" PRIf " to be within %" PRIf " of %" PRIf, PRIf_ARG(result),
		       PRIf_ARG(TEST_EPSILON), PRIf_ARG(expect));

	expect = FLOAT_TO_FP(126.778f);
	result = FLOAT_TO_FP(100.17f) + FLOAT_TO_FP(26.608f);
	zassert_within(expect, result, TEST_EPSILON,
		       "Expected %" PRIf " to be within %" PRIf " of %" PRIf, PRIf_ARG(result),
		       PRIf_ARG(TEST_EPSILON), PRIf_ARG(expect));

	expect = FLOAT_TO_FP(-17.33f);
	result = FLOAT_TO_FP(30.7f) - FLOAT_TO_FP(48.03f);
	zassert_within(expect, result, TEST_EPSILON,
		       "Expected %" PRIf " to be within %" PRIf " of %" PRIf, PRIf_ARG(result),
		       PRIf_ARG(TEST_EPSILON), PRIf_ARG(expect));
}

ZTEST(fpmath, test_fp_negation)
{
	fp_t expect;
	fp_t result;

	expect = FLOAT_TO_FP(52.43f);
	result = -FLOAT_TO_FP(-52.43f);
	zassert_within(expect, result, TEST_EPSILON,
		       "Expected %" PRIf " to be within %" PRIf " of %" PRIf, PRIf_ARG(result),
		       PRIf_ARG(TEST_EPSILON), PRIf_ARG(expect));

	expect = FLOAT_TO_FP(-72.77f);
	result = -FLOAT_TO_FP(72.77f);
	zassert_within(expect, result, TEST_EPSILON,
		       "Expected %" PRIf " to be within %" PRIf " of %" PRIf, PRIf_ARG(result),
		       PRIf_ARG(TEST_EPSILON), PRIf_ARG(expect));
}

ZTEST(fpmath, test_fp_to_int)
{
	int32_t expect;
	int32_t result;

	expect = (int32_t)3.75f;
	result = FP_TO_INT(FLOAT_TO_FP(3.75f));
	zassert_equal(expect, result, "Expected %d, but got %d", expect, result);

	expect = (int32_t)7.5f;
	result = FP_TO_INT(FLOAT_TO_FP(7.5f));
	zassert_equal(expect, result, "Expected %d, but got %d", expect, result);

	expect = (int32_t)12.25f;
	result = FP_TO_INT(FLOAT_TO_FP(12.25f));
	zassert_equal(expect, result, "Expected %d, but got %d", expect, result);

	expect = (int32_t)-3.75f;
	result = FP_TO_INT(FLOAT_TO_FP(-3.75f));
	zassert_equal(expect, result, "Expected %d, but got %d", expect, result);

	expect = (int32_t)-7.5f;
	result = FP_TO_INT(FLOAT_TO_FP(-7.5f));
	zassert_equal(expect, result, "Expected %d, but got %d", expect, result);

	expect = (int32_t)-12.25f;
	result = FP_TO_INT(FLOAT_TO_FP(-12.25f));
	zassert_equal(expect, result, "Expected %d, but got %d", expect, result);
}

ZTEST(fpmath, test_multiply)
{
	fp_t expect;
	fp_t result;

	expect = FLOAT_TO_FP(3.5f * 2.6f);
	result = fp_mul(FLOAT_TO_FP(3.5f), FLOAT_TO_FP(2.6f));
	zassert_within(expect, result, TEST_EPSILON,
		       "Expected %" PRIf " to be within %" PRIf " of %" PRIf, PRIf_ARG(result),
		       PRIf_ARG(TEST_EPSILON), PRIf_ARG(expect));

	expect = FLOAT_TO_FP(5.2f * -4.33f);
	result = fp_mul(FLOAT_TO_FP(5.2f), FLOAT_TO_FP(-4.33f));
	zassert_within(expect, result, TEST_EPSILON,
		       "Expected %" PRIf " to be within %" PRIf " of %" PRIf, PRIf_ARG(result),
		       PRIf_ARG(TEST_EPSILON), PRIf_ARG(expect));

	expect = FLOAT_TO_FP(-7.812f * -3.135f);
	result = fp_mul(FLOAT_TO_FP(-7.812f), FLOAT_TO_FP(-3.135f));
	zassert_within(expect, result, TEST_EPSILON,
		       "Expected %" PRIf " to be within %" PRIf " of %" PRIf, PRIf_ARG(result),
		       PRIf_ARG(TEST_EPSILON), PRIf_ARG(expect));
}

ZTEST(fpmath, test_divide)
{
	fp_t expect;
	fp_t result;

	expect = FLOAT_TO_FP(52.35f / 2.6f);
	result = fp_div(FLOAT_TO_FP(52.35f), FLOAT_TO_FP(2.6f));
	zassert_within(expect, result, TEST_EPSILON,
		       "Expected %" PRIf " to be within %" PRIf " of %" PRIf, PRIf_ARG(result),
		       PRIf_ARG(TEST_EPSILON), PRIf_ARG(expect));

	expect = FLOAT_TO_FP(78.52f / -4.33f);
	result = fp_div(FLOAT_TO_FP(78.52f), FLOAT_TO_FP(-4.33f));
	zassert_within(expect, result, TEST_EPSILON,
		       "Expected %" PRIf " to be within %" PRIf " of %" PRIf, PRIf_ARG(result),
		       PRIf_ARG(TEST_EPSILON), PRIf_ARG(expect));

	expect = FLOAT_TO_FP(-17.812f / -3.135f);
	result = fp_div(FLOAT_TO_FP(-17.812f), FLOAT_TO_FP(-3.135f));
	zassert_within(expect, result, TEST_EPSILON,
		       "Expected %" PRIf " to be within %" PRIf " of %" PRIf, PRIf_ARG(result),
		       PRIf_ARG(TEST_EPSILON), PRIf_ARG(expect));
}
