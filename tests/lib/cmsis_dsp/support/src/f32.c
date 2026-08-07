/*
 * Copyright (c) 2021 Stephanos Ioannidis <root@stephanos.io>
 * Copyright (C) 2010-2021 ARM Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <stdlib.h>
#include <arm_math.h>
#include "../../common/test_common.h"

#include "f32.pat"

#define SNR_ERROR_THRESH	(120)
#define REL_ERROR_THRESH	(1.0e-5)
#define ABS_ERROR_THRESH_Q7	((q7_t)10)
#define ABS_ERROR_THRESH_Q15	((q15_t)10)
#define ABS_ERROR_THRESH_Q31	((q31_t)80)

static void test_arm_copy_f32(const uint32_t *input1, size_t length)
{
	float32_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(float32_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_copy_f32((float32_t *)input1, output, length);

	/* Validate output */
	zassert_true(
		test_equal_f32(length, (float32_t *)input1, output),
		ASSERT_MSG_INCORRECT_COMP_RESULT);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT2(support_f32, arm_copy_f32, 3, in_f32, 3);
DEFINE_TEST_VARIANT2(support_f32, arm_copy_f32, 8, in_f32, 8);
DEFINE_TEST_VARIANT2(support_f32, arm_copy_f32, 11, in_f32, 11);

static void test_arm_fill_f32(size_t length)
{
	size_t index;
	float32_t *output;
	float32_t val = 1.1;

	/* Allocate output buffer */
	output = malloc(length * sizeof(float32_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_fill_f32(val, output, length);

	/* Validate output */
	for (index = 0; index < length; index++) {
		zassert_equal(
			output[index], val, ASSERT_MSG_INCORRECT_COMP_RESULT);
	}

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT1(support_f32, arm_fill_f32, 3, 3);
DEFINE_TEST_VARIANT1(support_f32, arm_fill_f32, 8, 8);
DEFINE_TEST_VARIANT1(support_f32, arm_fill_f32, 11, 11);

static void test_arm_float_to_q31(
	const uint32_t *input1, const q31_t *ref, size_t length)
{
	q31_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(q31_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_float_to_q31((float32_t *)input1, output, length);

	/* Validate output */
	zassert_true(
		test_near_equal_q31(length, ref, output, ABS_ERROR_THRESH_Q31),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT3(support_f32, arm_float_to_q31, 3, in_f32, ref_q31, 3);
DEFINE_TEST_VARIANT3(support_f32, arm_float_to_q31, 8, in_f32, ref_q31, 8);
DEFINE_TEST_VARIANT3(support_f32, arm_float_to_q31, 11, in_f32, ref_q31, 11);

static void test_arm_float_to_q15(
	const uint32_t *input1, const q15_t *ref, size_t length)
{
	q15_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(q15_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_float_to_q15((float32_t *)input1, output, length);

	/* Validate output */
	zassert_true(
		test_near_equal_q15(length, ref, output, ABS_ERROR_THRESH_Q15),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT3(support_f32, arm_float_to_q15, 7, in_f32, ref_q15, 7);
DEFINE_TEST_VARIANT3(support_f32, arm_float_to_q15, 16, in_f32, ref_q15, 16);
DEFINE_TEST_VARIANT3(support_f32, arm_float_to_q15, 17, in_f32, ref_q15, 17);

static void test_arm_float_to_q7(
	const uint32_t *input1, const q7_t *ref, size_t length)
{
	q7_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(q7_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_float_to_q7((float32_t *)input1, output, length);

	/* Validate output */
	zassert_true(
		test_near_equal_q7(length, ref, output, ABS_ERROR_THRESH_Q7),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT3(support_f32, arm_float_to_q7, 15, in_f32, ref_q7, 15);
DEFINE_TEST_VARIANT3(support_f32, arm_float_to_q7, 32, in_f32, ref_q7, 32);
DEFINE_TEST_VARIANT3(support_f32, arm_float_to_q7, 33, in_f32, ref_q7, 33);

static void test_arm_weighted_average_f32(
	int ref_offset, size_t length)
{
	const float32_t *val = (const float32_t *)in_weighted_sum_val;
	const float32_t *coeff = (const float32_t *)in_weighted_sum_coeff;
	const float32_t *ref = (const float32_t *)ref_weighted_sum;
	float32_t *output;

	/* Allocate output buffer */
	output = malloc(1 * sizeof(float32_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	output[0] = arm_weighted_average_f32(val, coeff, length);

	/* Validate output */
	zassert_true(
		test_rel_error_f32(1, output, &ref[ref_offset],
			REL_ERROR_THRESH),
		ASSERT_MSG_REL_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT2(support_f32, arm_weighted_average_f32, 3, 0, 3);
DEFINE_TEST_VARIANT2(support_f32, arm_weighted_average_f32, 8, 1, 8);
DEFINE_TEST_VARIANT2(support_f32, arm_weighted_average_f32, 11, 2, 11);

static void test_arm_sort_out(
	const uint32_t *input1, const uint32_t *ref, size_t length,
	arm_sort_alg alg, arm_sort_dir dir)
{
	float32_t *output;
	arm_sort_instance_f32 inst;

	/* Allocate output buffer */
	output = malloc(length * sizeof(float32_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Initialise sorter */
	arm_sort_init_f32(&inst, alg, dir);

	/* Run test function */
	arm_sort_f32(&inst, (float32_t *)input1, output, length);

	/* Validate output */
	zassert_true(
		test_equal_f32(length, output, (float32_t *)ref),
		ASSERT_MSG_INCORRECT_COMP_RESULT);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT5(support_f32, arm_sort_out, bitonic_16,
	in_sort_bitonic_16, ref_sort_bitonic_16, 16,
	ARM_SORT_BITONIC, ARM_SORT_ASCENDING);

DEFINE_TEST_VARIANT5(support_f32, arm_sort_out, bitonic_32,
	in_sort_bitonic_32, ref_sort_bitonic_32, 32,
	ARM_SORT_BITONIC, ARM_SORT_ASCENDING);

DEFINE_TEST_VARIANT5(support_f32, arm_sort_out, bubble_11,
	in_sort, ref_sort, 11,
	ARM_SORT_BUBBLE, ARM_SORT_ASCENDING);

DEFINE_TEST_VARIANT5(support_f32, arm_sort_out, heap_11,
	in_sort, ref_sort, 11,
	ARM_SORT_HEAP, ARM_SORT_ASCENDING);

DEFINE_TEST_VARIANT5(support_f32, arm_sort_out, insertion_11,
	in_sort, ref_sort, 11,
	ARM_SORT_INSERTION, ARM_SORT_ASCENDING);

DEFINE_TEST_VARIANT5(support_f32, arm_sort_out, quick_11,
	in_sort, ref_sort, 11,
	ARM_SORT_QUICK, ARM_SORT_ASCENDING);

DEFINE_TEST_VARIANT5(support_f32, arm_sort_out, selection_11,
	in_sort, ref_sort, 11,
	ARM_SORT_SELECTION, ARM_SORT_ASCENDING);

static void test_arm_merge_sort_out(
	const uint32_t *input1, const uint32_t *ref, size_t length,
	arm_sort_dir dir)
{
	float32_t *output;
	float32_t *scratch;
	arm_merge_sort_instance_f32 inst;

	/* Allocate output buffer */
	output = malloc(length * sizeof(float32_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Allocate scratch buffer */
	scratch = malloc(length * sizeof(float32_t));
	zassert_not_null(scratch, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Initialise sorter */
	arm_merge_sort_init_f32(&inst, dir, scratch);

	/* Run test function */
	arm_merge_sort_f32(&inst, (float32_t *)input1, output, length);

	/* Validate output */
	zassert_true(
		test_equal_f32(length, output, (float32_t *)ref),
		ASSERT_MSG_INCORRECT_COMP_RESULT);

	/* Free output buffer */
	free(output);
	free(scratch);
}

DEFINE_TEST_VARIANT4(support_f32, arm_merge_sort_out, 11,
	in_sort, ref_sort, 11, ARM_SORT_ASCENDING);

static void test_arm_sort_in(
	const uint32_t *input1, const uint32_t *ref, size_t length,
	arm_sort_alg alg, arm_sort_dir dir)
{
	float32_t *output;
	arm_sort_instance_f32 inst;

	/* Allocate output buffer */
	output = malloc(length * sizeof(float32_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Copy input to the output buffer */
	memcpy(output, input1, length * sizeof(float32_t));

	/* Initialise sorter */
	arm_sort_init_f32(&inst, alg, dir);

	/* Run test function */
	arm_sort_f32(&inst, output, output, length);

	/* Validate output */
	zassert_true(
		test_equal_f32(length, output, (float32_t *)ref),
		ASSERT_MSG_INCORRECT_COMP_RESULT);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT5(support_f32, arm_sort_in, bitonic_32,
	in_sort_bitonic_32, ref_sort_bitonic_32, 32,
	ARM_SORT_BITONIC, ARM_SORT_ASCENDING);

DEFINE_TEST_VARIANT5(support_f32, arm_sort_in, bubble_11,
	in_sort, ref_sort, 11,
	ARM_SORT_BUBBLE, ARM_SORT_ASCENDING);

DEFINE_TEST_VARIANT5(support_f32, arm_sort_in, heap_11,
	in_sort, ref_sort, 11,
	ARM_SORT_HEAP, ARM_SORT_ASCENDING);

DEFINE_TEST_VARIANT5(support_f32, arm_sort_in, insertion_11,
	in_sort, ref_sort, 11,
	ARM_SORT_INSERTION, ARM_SORT_ASCENDING);

DEFINE_TEST_VARIANT5(support_f32, arm_sort_in, quick_11,
	in_sort, ref_sort, 11,
	ARM_SORT_QUICK, ARM_SORT_ASCENDING);

DEFINE_TEST_VARIANT5(support_f32, arm_sort_in, selection_11,
	in_sort, ref_sort, 11,
	ARM_SORT_SELECTION, ARM_SORT_ASCENDING);

static void test_arm_sort_const(
	const uint32_t *input1, const uint32_t *ref, size_t length,
	arm_sort_alg alg, arm_sort_dir dir)
{
	float32_t *output;
	arm_sort_instance_f32 inst;

	/* Allocate output buffer */
	output = malloc(length * sizeof(float32_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Initialise sorter */
	arm_sort_init_f32(&inst, alg, dir);

	/* Run test function */
	arm_sort_f32(&inst, (float32_t *)input1, output, length);

	/* Validate output */
	zassert_true(
		test_equal_f32(length, output, (float32_t *)ref),
		ASSERT_MSG_INCORRECT_COMP_RESULT);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT5(support_f32, arm_sort_const, bitonic_16,
	in_sort_const, ref_sort_const, 16,
	ARM_SORT_BITONIC, ARM_SORT_ASCENDING);

DEFINE_TEST_VARIANT5(support_f32, arm_sort_const, bubble_16,
	in_sort_const, ref_sort_const, 16,
	ARM_SORT_BUBBLE, ARM_SORT_ASCENDING);

DEFINE_TEST_VARIANT5(support_f32, arm_sort_const, heap_16,
	in_sort_const, ref_sort_const, 16,
	ARM_SORT_HEAP, ARM_SORT_ASCENDING);

DEFINE_TEST_VARIANT5(support_f32, arm_sort_const, insertion_16,
	in_sort_const, ref_sort_const, 16,
	ARM_SORT_INSERTION, ARM_SORT_ASCENDING);

DEFINE_TEST_VARIANT5(support_f32, arm_sort_const, quick_16,
	in_sort_const, ref_sort_const, 16,
	ARM_SORT_QUICK, ARM_SORT_ASCENDING);

DEFINE_TEST_VARIANT5(support_f32, arm_sort_const, selection_16,
	in_sort_const, ref_sort_const, 16,
	ARM_SORT_SELECTION, ARM_SORT_ASCENDING);

static void test_arm_merge_sort_const(
	const uint32_t *input1, const uint32_t *ref, size_t length,
	arm_sort_dir dir)
{
	float32_t *output;
	float32_t *scratch;
	arm_merge_sort_instance_f32 inst;

	/* Allocate output buffer */
	output = malloc(length * sizeof(float32_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Allocate scratch buffer */
	scratch = malloc(length * sizeof(float32_t));
	zassert_not_null(scratch, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Initialise sorter */
	arm_merge_sort_init_f32(&inst, dir, scratch);

	/* Run test function */
	arm_merge_sort_f32(&inst, (float32_t *)input1, output, length);

	/* Validate output */
	zassert_true(
		test_equal_f32(length, output, (float32_t *)ref),
		ASSERT_MSG_INCORRECT_COMP_RESULT);

	/* Free output buffer */
	free(output);
	free(scratch);
}

DEFINE_TEST_VARIANT4(support_f32, arm_merge_sort_const, 16,
	in_sort_const, ref_sort_const, 16, ARM_SORT_ASCENDING);

ZTEST_SUITE(support_f32, NULL, NULL, NULL, NULL, NULL);
