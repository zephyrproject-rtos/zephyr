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

#include "q31.pat"

#define REL_ERROR_THRESH	(1.0e-5)
#define ABS_ERROR_THRESH_Q7	((q7_t)10)
#define ABS_ERROR_THRESH_Q15	((q15_t)10)
#define ABS_ERROR_THRESH_Q31	((q31_t)80)

static void test_arm_copy_q31(const q31_t *input1, size_t length)
{
	q31_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(q31_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_copy_q31(input1, output, length);

	/* Validate output */
	zassert_true(
		test_equal_q31(length, input1, output),
		ASSERT_MSG_INCORRECT_COMP_RESULT);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT2(arm_copy_q31, 3, in_q31, 3);
DEFINE_TEST_VARIANT2(arm_copy_q31, 8, in_q31, 8);
DEFINE_TEST_VARIANT2(arm_copy_q31, 11, in_q31, 11);

static void test_arm_fill_q31(size_t length)
{
	size_t index;
	q31_t *output;
	q31_t val = 0x40000000;

	/* Allocate output buffer */
	output = malloc(length * sizeof(q31_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_fill_q31(val, output, length);

	/* Validate output */
	for (index = 0; index < length; index++) {
		zassert_equal(
			output[index], val, ASSERT_MSG_INCORRECT_COMP_RESULT);
	}

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT1(arm_fill_q31, 3, 3);
DEFINE_TEST_VARIANT1(arm_fill_q31, 8, 8);
DEFINE_TEST_VARIANT1(arm_fill_q31, 11, 11);

static void test_arm_q31_to_float(
	const q31_t *input1, const uint32_t *ref, size_t length)
{
	float32_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(float32_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_q31_to_float(input1, output, length);

	/* Validate output */
	zassert_true(
		test_rel_error_f32(
			length, (float32_t *)ref, output, REL_ERROR_THRESH),
		ASSERT_MSG_REL_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT3(arm_q31_to_float, 7, in_q31, ref_f32, 7);
DEFINE_TEST_VARIANT3(arm_q31_to_float, 16, in_q31, ref_f32, 16);
DEFINE_TEST_VARIANT3(arm_q31_to_float, 17, in_q31, ref_f32, 17);

static void test_arm_q31_to_q15(
	const q31_t *input1, const q15_t *ref, size_t length)
{
	q15_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(q15_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_q31_to_q15(input1, output, length);

	/* Validate output */
	zassert_true(
		test_near_equal_q15(length, ref, output, ABS_ERROR_THRESH_Q15),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT3(arm_q31_to_q15, 3, in_q31, ref_q15, 3);
DEFINE_TEST_VARIANT3(arm_q31_to_q15, 8, in_q31, ref_q15, 8);
DEFINE_TEST_VARIANT3(arm_q31_to_q15, 11, in_q31, ref_q15, 11);

static void test_arm_q31_to_q7(
	const q31_t *input1, const q7_t *ref, size_t length)
{
	q7_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(q7_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_q31_to_q7(input1, output, length);

	/* Validate output */
	zassert_true(
		test_near_equal_q7(length, ref, output, ABS_ERROR_THRESH_Q7),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT3(arm_q31_to_q7, 15, in_q31, ref_q7, 15);
DEFINE_TEST_VARIANT3(arm_q31_to_q7, 32, in_q31, ref_q7, 32);
DEFINE_TEST_VARIANT3(arm_q31_to_q7, 33, in_q31, ref_q7, 33);

void test_support_q31(void)
{
	ztest_test_suite(support_q31,
		ztest_unit_test(test_arm_copy_q31_3),
		ztest_unit_test(test_arm_copy_q31_8),
		ztest_unit_test(test_arm_copy_q31_11),
		ztest_unit_test(test_arm_fill_q31_3),
		ztest_unit_test(test_arm_fill_q31_8),
		ztest_unit_test(test_arm_fill_q31_11),
		ztest_unit_test(test_arm_q31_to_float_7),
		ztest_unit_test(test_arm_q31_to_float_16),
		ztest_unit_test(test_arm_q31_to_float_17),
		ztest_unit_test(test_arm_q31_to_q15_3),
		ztest_unit_test(test_arm_q31_to_q15_8),
		ztest_unit_test(test_arm_q31_to_q15_11),
		ztest_unit_test(test_arm_q31_to_q7_15),
		ztest_unit_test(test_arm_q31_to_q7_32),
		ztest_unit_test(test_arm_q31_to_q7_33)
		);

	ztest_run_test_suite(support_q31);
}
