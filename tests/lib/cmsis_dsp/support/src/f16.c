/*
 * Copyright (c) 2021 Stephanos Ioannidis <root@stephanos.io>
 * Copyright (C) 2010-2021 ARM Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <zephyr/zephyr.h>
#include <stdlib.h>
#include <arm_math_f16.h>
#include "../../common/test_common.h"

#include "f16.pat"

#define SNR_ERROR_THRESH	(120)

#define ABS_ERROR_THRESH_F16	(1.0e-5)
#define REL_ERROR_THRESH_F16	(1.0e-5)

#define ABS_ERROR_THRESH_F32	(1.0e-3)
#define REL_ERROR_THRESH_F32	(1.0e-3)

#define ABS_ERROR_THRESH_Q7	((q15_t)10)
#define ABS_ERROR_THRESH_Q15	((q15_t)10)
#define ABS_ERROR_THRESH_Q31	((q31_t)80)

#define ABS_ERROR_THRESH_WS	(1.0e-1)
#define REL_ERROR_THRESH_WS	(5.0e-3)

static void test_arm_copy_f16(const uint16_t *input1, size_t length)
{
	float16_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(float16_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_copy_f16((float16_t *)input1, output, length);

	/* Validate output */
	zassert_true(
		test_equal_f16(length, (float16_t *)input1, output),
		ASSERT_MSG_INCORRECT_COMP_RESULT);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT2(arm_copy_f16, 7, ref_f16, 7);
DEFINE_TEST_VARIANT2(arm_copy_f16, 16, ref_f16, 16);
DEFINE_TEST_VARIANT2(arm_copy_f16, 23, ref_f16, 23);

static void test_arm_fill_f16(size_t length)
{
	size_t index;
	float16_t *output;
	float16_t val = 1.1;

	/* Allocate output buffer */
	output = malloc(length * sizeof(float16_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_fill_f16(val, output, length);

	/* Validate output */
	for (index = 0; index < length; index++) {
		zassert_equal(
			output[index], val, ASSERT_MSG_INCORRECT_COMP_RESULT);
	}

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT1(arm_fill_f16, 7, 7);
DEFINE_TEST_VARIANT1(arm_fill_f16, 16, 16);
DEFINE_TEST_VARIANT1(arm_fill_f16, 23, 23);

static void test_arm_f16_to_q15(
	const uint16_t *input1, const q15_t *ref, size_t length)
{
	q15_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(q15_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_f16_to_q15((float16_t *)input1, output, length);

	/* Validate output */
	zassert_true(
		test_near_equal_q15(length, ref, output, ABS_ERROR_THRESH_Q15),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT3(arm_f16_to_q15, 7, ref_f16, ref_q15, 7);
DEFINE_TEST_VARIANT3(arm_f16_to_q15, 16, ref_f16, ref_q15, 16);
DEFINE_TEST_VARIANT3(arm_f16_to_q15, 23, ref_f16, ref_q15, 23);

static void test_arm_f16_to_float(
	const uint16_t *input1, const uint32_t *ref, size_t length)
{
	float32_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(float32_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_f16_to_float((float16_t *)input1, output, length);

	/* Validate output */
	zassert_true(
		test_rel_error_f32(length, (float32_t *)ref, output,
			REL_ERROR_THRESH_F32),
		ASSERT_MSG_REL_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT3(arm_f16_to_float, 7, ref_f16, ref_f32, 7);
DEFINE_TEST_VARIANT3(arm_f16_to_float, 16, ref_f16, ref_f32, 16);
DEFINE_TEST_VARIANT3(arm_f16_to_float, 23, ref_f16, ref_f32, 23);

static void test_arm_q15_to_f16(
	const q15_t *input1, const uint16_t *ref, size_t length)
{
	float16_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(float16_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_q15_to_f16(input1, output, length);

	/* Validate output */
	zassert_true(
		test_rel_error_f16(length, (float16_t *)ref, output,
			REL_ERROR_THRESH_F16),
		ASSERT_MSG_REL_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT3(arm_q15_to_f16, 7, ref_q15, ref_f16, 7);
DEFINE_TEST_VARIANT3(arm_q15_to_f16, 16, ref_q15, ref_f16, 16);
DEFINE_TEST_VARIANT3(arm_q15_to_f16, 23, ref_q15, ref_f16, 23);

static void test_arm_float_to_f16(
	const uint32_t *input1, const uint16_t *ref, size_t length)
{
	float16_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(float16_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_float_to_f16((float32_t *)input1, output, length);

	/* Validate output */
	zassert_true(
		test_rel_error_f16(length, (float16_t *)ref, output,
			REL_ERROR_THRESH_F16),
		ASSERT_MSG_REL_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT3(arm_float_to_f16, 7, ref_f32, ref_f16, 7);
DEFINE_TEST_VARIANT3(arm_float_to_f16, 16, ref_f32, ref_f16, 16);
DEFINE_TEST_VARIANT3(arm_float_to_f16, 23, ref_f32, ref_f16, 23);

static void test_arm_weighted_sum_f16(
	int ref_offset, size_t length)
{
	const float16_t *val = (const float16_t *)in_weighted_sum_val;
	const float16_t *coeff = (const float16_t *)in_weighted_sum_coeff;
	const float16_t *ref = (const float16_t *)ref_weighted_sum;
	float16_t *output;

	/* Allocate output buffer */
	output = malloc(1 * sizeof(float16_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	output[0] = arm_weighted_sum_f16(val, coeff, length);

	/* Validate output */
	zassert_true(
		test_close_error_f16(1, output, &ref[ref_offset],
			ABS_ERROR_THRESH_WS, REL_ERROR_THRESH_WS),
		ASSERT_MSG_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT2(arm_weighted_sum_f16, 7, 0, 7);
DEFINE_TEST_VARIANT2(arm_weighted_sum_f16, 16, 1, 16);
DEFINE_TEST_VARIANT2(arm_weighted_sum_f16, 23, 2, 23);

void test_support_f16(void)
{
	ztest_test_suite(support_f16,
		ztest_unit_test(test_arm_copy_f16_7),
		ztest_unit_test(test_arm_copy_f16_16),
		ztest_unit_test(test_arm_copy_f16_23),
		ztest_unit_test(test_arm_fill_f16_7),
		ztest_unit_test(test_arm_fill_f16_16),
		ztest_unit_test(test_arm_fill_f16_23),
		ztest_unit_test(test_arm_f16_to_q15_7),
		ztest_unit_test(test_arm_f16_to_q15_16),
		ztest_unit_test(test_arm_f16_to_q15_23),
		ztest_unit_test(test_arm_f16_to_float_7),
		ztest_unit_test(test_arm_f16_to_float_16),
		ztest_unit_test(test_arm_f16_to_float_23),
		ztest_unit_test(test_arm_q15_to_f16_7),
		ztest_unit_test(test_arm_q15_to_f16_16),
		ztest_unit_test(test_arm_q15_to_f16_23),
		ztest_unit_test(test_arm_float_to_f16_7),
		ztest_unit_test(test_arm_float_to_f16_16),
		ztest_unit_test(test_arm_float_to_f16_23),
		ztest_unit_test(test_arm_weighted_sum_f16_7),
		ztest_unit_test(test_arm_weighted_sum_f16_16),
		ztest_unit_test(test_arm_weighted_sum_f16_23)
		);

	ztest_run_test_suite(support_f16);
}
