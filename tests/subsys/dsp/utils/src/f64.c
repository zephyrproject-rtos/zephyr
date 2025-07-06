/*
 * Copyright (C) 2024 OWL Services LLC. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/dsp/utils.h>

#include "common/test_common.h"

#define DEFINE_MULTIPLE_TEST_CASES(constructor, variant, array)                                    \
	FOR_EACH_IDX_FIXED_ARG(constructor, (), variant, array)

#define RE_DEFINE_TEST_VARIANT3(...) DEFINE_TEST_VARIANT3(__VA_ARGS__)

#define DEFINE_SHIFT_F64_CASE(n, args, func)                                                       \
	RE_DEFINE_TEST_VARIANT3(shift_f64, func, n, __DEBRACKET args)

#define TEST_CASES_SHIFT_F64_TO_Q7 (-1.0, 0, -128), (1.0, 0, 127), (1.0, 7, 1), (-1.0, 7, -1)

#define TEST_CASES_SHIFT_F64_TO_Q15 (-1.0, 0, -32768), (1.0, 0, 32767), (1.0, 15, 1), (-1.0, 15, -1)

#define TEST_CASES_SHIFT_F64_TO_Q31                                                                \
	(-1.0, 0, INT32_MIN), (1.0, 0, INT32_MAX), (1.0, 31, 1), (-1.0, 31, -1)

static void test_shift_f64_to_q7(const float64_t data, const uint32_t shift,
				 const DSP_DATA q7_t expected)
{
	const q7_t shifted_data = Z_SHIFT_F64_TO_Q7(data, shift);

	zassert_equal(shifted_data, expected,
		      "Conversion failed: %f shifted by %d = %d (expected %d)", data, shift,
		      shifted_data, expected);
}

static void test_shift_f64_to_q15(const float64_t data, const uint32_t shift,
				  const DSP_DATA q15_t expected)
{
	const q15_t shifted_data = Z_SHIFT_F64_TO_Q15(data, shift);

	zassert_equal(shifted_data, expected,
		      "Conversion failed: %f shifted by %d = %d (expected %d)", (double)data, shift,
		      shifted_data, expected);
}

static void test_shift_f64_to_q31(const float64_t data, const uint32_t shift,
				  const DSP_DATA q31_t expected)
{
	const q31_t shifted_data = Z_SHIFT_F64_TO_Q31(data, shift);

	zassert_equal(shifted_data, expected,
		      "Conversion failed: %f shifted by %d = %d (expected %d)", (double)data, shift,
		      shifted_data, expected);
}

DEFINE_MULTIPLE_TEST_CASES(DEFINE_SHIFT_F64_CASE, shift_f64_to_q7, TEST_CASES_SHIFT_F64_TO_Q7)
DEFINE_MULTIPLE_TEST_CASES(DEFINE_SHIFT_F64_CASE, shift_f64_to_q15, TEST_CASES_SHIFT_F64_TO_Q15)
DEFINE_MULTIPLE_TEST_CASES(DEFINE_SHIFT_F64_CASE, shift_f64_to_q31, TEST_CASES_SHIFT_F64_TO_Q31)

ZTEST_SUITE(shift_f64, NULL, NULL, NULL, NULL, NULL);
