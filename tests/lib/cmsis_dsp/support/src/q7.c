/*
 * Copyright (c) 2020 Stephanos Ioannidis <root@stephanos.io>
 * Copyright (C) 2010-2020 ARM Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <zephyr/zephyr.h>
#include <stdlib.h>
#include <arm_math.h>
#include "../../common/test_common.h"

#include "q7.pat"

#define REL_ERROR_THRESH	(1.0e-5)
#define ABS_ERROR_THRESH_Q7	((q7_t)10)
#define ABS_ERROR_THRESH_Q15	((q15_t)(1 << 8))
#define ABS_ERROR_THRESH_Q31	((q31_t)(1 << 24))

static void test_arm_copy_q7(const q7_t *input1, size_t length)
{
	q7_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(q7_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_copy_q7(input1, output, length);

	/* Validate output */
	zassert_true(
		test_equal_q7(length, input1, output),
		ASSERT_MSG_INCORRECT_COMP_RESULT);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT2(arm_copy_q7, 15, in_q7, 15);
DEFINE_TEST_VARIANT2(arm_copy_q7, 32, in_q7, 32);
DEFINE_TEST_VARIANT2(arm_copy_q7, 47, in_q7, 47);

static void test_arm_fill_q7(size_t length)
{
	size_t index;
	q7_t *output;
	q7_t val = 0x40;

	/* Allocate output buffer */
	output = malloc(length * sizeof(q7_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_fill_q7(val, output, length);

	/* Validate output */
	for (index = 0; index < length; index++) {
		zassert_equal(
			output[index], val, ASSERT_MSG_INCORRECT_COMP_RESULT);
	}

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT1(arm_fill_q7, 15, 15);
DEFINE_TEST_VARIANT1(arm_fill_q7, 32, 32);
DEFINE_TEST_VARIANT1(arm_fill_q7, 47, 47);

static void test_arm_q7_to_float(
	const q7_t *input1, const uint32_t *ref, size_t length)
{
	float32_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(float32_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_q7_to_float(input1, output, length);

	/* Validate output */
	zassert_true(
		test_close_error_f32(length, (float32_t *)ref, output,
			0.01, REL_ERROR_THRESH),
		ASSERT_MSG_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT3(arm_q7_to_float, 15, in_q7, ref_f32, 15);
DEFINE_TEST_VARIANT3(arm_q7_to_float, 32, in_q7, ref_f32, 32);
DEFINE_TEST_VARIANT3(arm_q7_to_float, 47, in_q7, ref_f32, 47);

static void test_arm_q7_to_q31(
	const q7_t *input1, const q31_t *ref, size_t length)
{
	q31_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(q31_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_q7_to_q31(input1, output, length);

	/* Validate output */
	zassert_true(
		test_near_equal_q31(length, ref, output, ABS_ERROR_THRESH_Q31),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT3(arm_q7_to_q31, 15, in_q7, ref_q31, 15);
DEFINE_TEST_VARIANT3(arm_q7_to_q31, 32, in_q7, ref_q31, 32);
DEFINE_TEST_VARIANT3(arm_q7_to_q31, 47, in_q7, ref_q31, 47);

static void test_arm_q7_to_q15(
	const q7_t *input1, const q15_t *ref, size_t length)
{
	q15_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(q15_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_q7_to_q15(input1, output, length);

	/* Validate output */
	zassert_true(
		test_near_equal_q15(length, ref, output, ABS_ERROR_THRESH_Q15),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT3(arm_q7_to_q15, 15, in_q7, ref_q15, 15);
DEFINE_TEST_VARIANT3(arm_q7_to_q15, 32, in_q7, ref_q15, 32);
DEFINE_TEST_VARIANT3(arm_q7_to_q15, 47, in_q7, ref_q15, 47);

void test_support_q7(void)
{
	ztest_test_suite(support_q7,
		ztest_unit_test(test_arm_copy_q7_15),
		ztest_unit_test(test_arm_copy_q7_32),
		ztest_unit_test(test_arm_copy_q7_47),
		ztest_unit_test(test_arm_fill_q7_15),
		ztest_unit_test(test_arm_fill_q7_32),
		ztest_unit_test(test_arm_fill_q7_47),
		ztest_unit_test(test_arm_q7_to_float_15),
		ztest_unit_test(test_arm_q7_to_float_32),
		ztest_unit_test(test_arm_q7_to_float_47),
		ztest_unit_test(test_arm_q7_to_q31_15),
		ztest_unit_test(test_arm_q7_to_q31_32),
		ztest_unit_test(test_arm_q7_to_q31_47),
		ztest_unit_test(test_arm_q7_to_q15_15),
		ztest_unit_test(test_arm_q7_to_q15_32),
		ztest_unit_test(test_arm_q7_to_q15_47)
		);

	ztest_run_test_suite(support_q7);
}
