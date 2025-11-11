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

#define DEFINE_SHIFT_Q31_CASE(n, args, func)                                                       \
	RE_DEFINE_TEST_VARIANT3(shift_q31, func, n, __DEBRACKET args)

#define TEST_CORNER_CASES_SHIFT_Q31_TO_F32                                                         \
	(2147483647, 0, 1.0F), (-2147483648, 0, -1.0F), (-2147483648, 31, -2147483648.0F),         \
		(2147483647, 31, 2147483647.0F)

#define TEST_CORNER_CASES_SHIFT_Q31_TO_F64                                                         \
	(2147483647, 0, 0.9999999995343387), (-2147483648, 0, -1.0),                               \
		(-2147483648, 31, -2147483648.0), (2147483647, 31, 2147483647.0)

static void test_shift_q31_to_f32(const q31_t data, const uint32_t shift, const float32_t expected)
{
	const float32_t shifted_data = Z_SHIFT_Q31_TO_F32(data, shift);

	zassert_equal(shifted_data, expected,
		      "Conversion failed: 0x%08x shifted by %d = %f (expected %f)", data, shift,
		      (double)shifted_data, (double)expected);
}

static void test_shift_q31_to_f64(const q31_t data, const uint32_t shift, const float64_t expected)
{
	const float64_t shifted_data = Z_SHIFT_Q31_TO_F64(data, shift);

	zassert_equal(shifted_data, expected,
		      "Conversion failed: 0x%08x shifted by %d = %f (expected %f)", data, shift,
		      shifted_data, expected);
}

DEFINE_MULTIPLE_TEST_CASES(DEFINE_SHIFT_Q31_CASE, shift_q31_to_f32,
			   TEST_CORNER_CASES_SHIFT_Q31_TO_F32)
DEFINE_MULTIPLE_TEST_CASES(DEFINE_SHIFT_Q31_CASE, shift_q31_to_f64,
			   TEST_CORNER_CASES_SHIFT_Q31_TO_F64)

ZTEST_SUITE(shift_q31, NULL, NULL, NULL, NULL, NULL);
