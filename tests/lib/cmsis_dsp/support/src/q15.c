/*
 * Copyright (c) 2020 Stephanos Ioannidis <root@stephanos.io>
 * Copyright (C) 2010-2020 ARM Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <zephyr.h>
#include <stdlib.h>
#include <arm_math.h>
#include "../../common/test_common.h"

#include "q15.pat"

#define REL_ERROR_THRESH	(1.0e-3)
#define ABS_ERROR_THRESH_Q7	((q7_t)10)
#define ABS_ERROR_THRESH_Q15	((q15_t)10)
#define ABS_ERROR_THRESH_Q31	((q31_t)40000)

static void test_arm_copy_q15(const q15_t *input1, size_t length)
{
	q15_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(q15_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_copy_q15(input1, output, length);

	/* Validate output */
	zassert_true(
		test_equal_q15(length, input1, output),
		ASSERT_MSG_INCORRECT_COMP_RESULT);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT2(arm_copy_q15, 7, in_q15, 7);
DEFINE_TEST_VARIANT2(arm_copy_q15, 16, in_q15, 16);
DEFINE_TEST_VARIANT2(arm_copy_q15, 23, in_q15, 23);

static void test_arm_fill_q15(size_t length)
{
	size_t index;
	q15_t *output;
	q15_t val = 0x4000;

	/* Allocate output buffer */
	output = malloc(length * sizeof(q15_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_fill_q15(val, output, length);

	/* Validate output */
	for (index = 0; index < length; index++) {
		zassert_equal(
			output[index], val, ASSERT_MSG_INCORRECT_COMP_RESULT);
	}

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT1(arm_fill_q15, 7, 7);
DEFINE_TEST_VARIANT1(arm_fill_q15, 16, 16);
DEFINE_TEST_VARIANT1(arm_fill_q15, 23, 23);

static void test_arm_q15_to_float(
	const q15_t *input1, const uint32_t *ref, size_t length)
{
	float32_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(float32_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_q15_to_float(input1, output, length);

	/* Validate output */
	zassert_true(
		test_rel_error_f32(
			length, (float32_t *)ref, output, REL_ERROR_THRESH),
		ASSERT_MSG_REL_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT3(arm_q15_to_float, 7, in_q15, ref_f32, 7);
DEFINE_TEST_VARIANT3(arm_q15_to_float, 16, in_q15, ref_f32, 16);
DEFINE_TEST_VARIANT3(arm_q15_to_float, 23, in_q15, ref_f32, 23);

static void test_arm_q15_to_q31(
	const q15_t *input1, const q31_t *ref, size_t length)
{
	q31_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(q31_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_q15_to_q31(input1, output, length);

	/* Validate output */
	zassert_true(
		test_near_equal_q31(
			length, ref, output, ABS_ERROR_THRESH_Q31),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT3(arm_q15_to_q31, 7, in_q15, ref_q31, 7);
DEFINE_TEST_VARIANT3(arm_q15_to_q31, 16, in_q15, ref_q31, 16);
DEFINE_TEST_VARIANT3(arm_q15_to_q31, 23, in_q15, ref_q31, 23);

static void test_arm_q15_to_q7(
	const q15_t *input1, const q7_t *ref, size_t length)
{
	q7_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(q7_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_q15_to_q7(input1, output, length);

	/* Validate output */
	zassert_true(
		test_near_equal_q7(length, ref, output, ABS_ERROR_THRESH_Q7),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT3(arm_q15_to_q7, 7, in_q15, ref_q7, 7);
DEFINE_TEST_VARIANT3(arm_q15_to_q7, 16, in_q15, ref_q7, 16);
DEFINE_TEST_VARIANT3(arm_q15_to_q7, 23, in_q15, ref_q7, 23);

void test_support_q15(void)
{
	ztest_test_suite(support_q15,
		ztest_unit_test(test_arm_copy_q15_7),
		ztest_unit_test(test_arm_copy_q15_16),
		ztest_unit_test(test_arm_copy_q15_23),
		ztest_unit_test(test_arm_fill_q15_7),
		ztest_unit_test(test_arm_fill_q15_16),
		ztest_unit_test(test_arm_fill_q15_23),
		ztest_unit_test(test_arm_q15_to_float_7),
		ztest_unit_test(test_arm_q15_to_float_16),
		ztest_unit_test(test_arm_q15_to_float_23),
		ztest_unit_test(test_arm_q15_to_q31_7),
		ztest_unit_test(test_arm_q15_to_q31_16),
		ztest_unit_test(test_arm_q15_to_q31_23),
		ztest_unit_test(test_arm_q15_to_q7_7),
		ztest_unit_test(test_arm_q15_to_q7_16),
		ztest_unit_test(test_arm_q15_to_q7_23)
		);

	ztest_run_test_suite(support_q15);
}
