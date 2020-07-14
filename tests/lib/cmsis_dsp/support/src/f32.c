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

DEFINE_TEST_VARIANT2(arm_copy_f32, 3, in_f32, 3);
DEFINE_TEST_VARIANT2(arm_copy_f32, 8, in_f32, 8);
DEFINE_TEST_VARIANT2(arm_copy_f32, 11, in_f32, 11);

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

DEFINE_TEST_VARIANT1(arm_fill_f32, 3, 3);
DEFINE_TEST_VARIANT1(arm_fill_f32, 8, 8);
DEFINE_TEST_VARIANT1(arm_fill_f32, 11, 11);

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

DEFINE_TEST_VARIANT3(arm_float_to_q31, 3, in_f32, ref_q31, 3);
DEFINE_TEST_VARIANT3(arm_float_to_q31, 8, in_f32, ref_q31, 8);
DEFINE_TEST_VARIANT3(arm_float_to_q31, 11, in_f32, ref_q31, 11);

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

DEFINE_TEST_VARIANT3(arm_float_to_q15, 7, in_f32, ref_q15, 7);
DEFINE_TEST_VARIANT3(arm_float_to_q15, 16, in_f32, ref_q15, 16);
DEFINE_TEST_VARIANT3(arm_float_to_q15, 17, in_f32, ref_q15, 17);

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

DEFINE_TEST_VARIANT3(arm_float_to_q7, 15, in_f32, ref_q7, 15);
DEFINE_TEST_VARIANT3(arm_float_to_q7, 32, in_f32, ref_q7, 32);
DEFINE_TEST_VARIANT3(arm_float_to_q7, 33, in_f32, ref_q7, 33);

static void test_arm_weighted_sum_f32(
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
	output[0] = arm_weighted_sum_f32(val, coeff, length);

	/* Validate output */
	zassert_true(
		test_rel_error_f32(1, output, &ref[ref_offset],
			REL_ERROR_THRESH),
		ASSERT_MSG_REL_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT2(arm_weighted_sum_f32, 3, 0, 3);
DEFINE_TEST_VARIANT2(arm_weighted_sum_f32, 8, 1, 8);
DEFINE_TEST_VARIANT2(arm_weighted_sum_f32, 11, 2, 11);

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

DEFINE_TEST_VARIANT5(arm_sort_out, bitonic_16,
	in_sort_bitonic_16, ref_sort_bitonic_16, 16,
	ARM_SORT_BITONIC, ARM_SORT_ASCENDING);

DEFINE_TEST_VARIANT5(arm_sort_out, bitonic_32,
	in_sort_bitonic_32, ref_sort_bitonic_32, 32,
	ARM_SORT_BITONIC, ARM_SORT_ASCENDING);

DEFINE_TEST_VARIANT5(arm_sort_out, bubble_11,
	in_sort, ref_sort, 11,
	ARM_SORT_BUBBLE, ARM_SORT_ASCENDING);

DEFINE_TEST_VARIANT5(arm_sort_out, heap_11,
	in_sort, ref_sort, 11,
	ARM_SORT_HEAP, ARM_SORT_ASCENDING);

DEFINE_TEST_VARIANT5(arm_sort_out, insertion_11,
	in_sort, ref_sort, 11,
	ARM_SORT_INSERTION, ARM_SORT_ASCENDING);

DEFINE_TEST_VARIANT5(arm_sort_out, quick_11,
	in_sort, ref_sort, 11,
	ARM_SORT_QUICK, ARM_SORT_ASCENDING);

DEFINE_TEST_VARIANT5(arm_sort_out, selection_11,
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

DEFINE_TEST_VARIANT4(arm_merge_sort_out, 11,
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

DEFINE_TEST_VARIANT5(arm_sort_in, bitonic_32,
	in_sort_bitonic_32, ref_sort_bitonic_32, 32,
	ARM_SORT_BITONIC, ARM_SORT_ASCENDING);

DEFINE_TEST_VARIANT5(arm_sort_in, bubble_11,
	in_sort, ref_sort, 11,
	ARM_SORT_BUBBLE, ARM_SORT_ASCENDING);

DEFINE_TEST_VARIANT5(arm_sort_in, heap_11,
	in_sort, ref_sort, 11,
	ARM_SORT_HEAP, ARM_SORT_ASCENDING);

DEFINE_TEST_VARIANT5(arm_sort_in, insertion_11,
	in_sort, ref_sort, 11,
	ARM_SORT_INSERTION, ARM_SORT_ASCENDING);

DEFINE_TEST_VARIANT5(arm_sort_in, quick_11,
	in_sort, ref_sort, 11,
	ARM_SORT_QUICK, ARM_SORT_ASCENDING);

DEFINE_TEST_VARIANT5(arm_sort_in, selection_11,
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

DEFINE_TEST_VARIANT5(arm_sort_const, bitonic_16,
	in_sort_const, ref_sort_const, 16,
	ARM_SORT_BITONIC, ARM_SORT_ASCENDING);

DEFINE_TEST_VARIANT5(arm_sort_const, bubble_16,
	in_sort_const, ref_sort_const, 16,
	ARM_SORT_BUBBLE, ARM_SORT_ASCENDING);

DEFINE_TEST_VARIANT5(arm_sort_const, heap_16,
	in_sort_const, ref_sort_const, 16,
	ARM_SORT_HEAP, ARM_SORT_ASCENDING);

DEFINE_TEST_VARIANT5(arm_sort_const, insertion_16,
	in_sort_const, ref_sort_const, 16,
	ARM_SORT_INSERTION, ARM_SORT_ASCENDING);

DEFINE_TEST_VARIANT5(arm_sort_const, quick_16,
	in_sort_const, ref_sort_const, 16,
	ARM_SORT_QUICK, ARM_SORT_ASCENDING);

DEFINE_TEST_VARIANT5(arm_sort_const, selection_16,
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

DEFINE_TEST_VARIANT4(arm_merge_sort_const, 16,
	in_sort_const, ref_sort_const, 16, ARM_SORT_ASCENDING);

static void test_arm_spline(
	const uint32_t *input_x, const uint32_t *input_y,
	const uint32_t *input_xq, const uint32_t *ref, size_t length,
	uint32_t n, arm_spline_type type)
{
	float32_t *output;
	float32_t *scratch;
	float32_t *coeff;
	arm_spline_instance_f32 inst;

	/* Allocate output buffer */
	output = malloc(length * sizeof(float32_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Allocate scratch buffer */
	scratch = malloc(((n * 2) - 1) * sizeof(float32_t));
	zassert_not_null(scratch, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Allocate coefficient buffer */
	coeff = malloc(((n - 1) * 3) * sizeof(float32_t));
	zassert_not_null(coeff, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Initialise spline */
	arm_spline_init_f32(&inst, type,
		(float32_t *)input_x, (float32_t *)input_y, n, coeff, scratch);

	/* Run test function */
	arm_spline_f32(&inst, (float32_t *)input_xq, output, length);

	/* Validate output */
	zassert_true(
		test_snr_error_f32(length, output, (float32_t *)ref,
			SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
	free(scratch);
	free(coeff);
}

DEFINE_TEST_VARIANT7(arm_spline, square_20,
	in_spline_squ_x, in_spline_squ_y, in_spline_squ_xq, ref_spline_squ, 20,
	4, ARM_SPLINE_PARABOLIC_RUNOUT);

DEFINE_TEST_VARIANT7(arm_spline, sine_33,
	in_spline_sin_x, in_spline_sin_y, in_spline_sin_xq, ref_spline_sin, 33,
	9, ARM_SPLINE_NATURAL);

DEFINE_TEST_VARIANT7(arm_spline, ramp_30,
	in_spline_ram_x, in_spline_ram_y, in_spline_ram_xq, ref_spline_ram, 30,
	3, ARM_SPLINE_PARABOLIC_RUNOUT);

void test_support_f32(void)
{
	ztest_test_suite(support_f32,
		ztest_unit_test(test_arm_copy_f32_3),
		ztest_unit_test(test_arm_copy_f32_8),
		ztest_unit_test(test_arm_copy_f32_11),
		ztest_unit_test(test_arm_fill_f32_3),
		ztest_unit_test(test_arm_fill_f32_8),
		ztest_unit_test(test_arm_fill_f32_11),
		ztest_unit_test(test_arm_float_to_q31_3),
		ztest_unit_test(test_arm_float_to_q31_8),
		ztest_unit_test(test_arm_float_to_q31_11),
		ztest_unit_test(test_arm_float_to_q15_7),
		ztest_unit_test(test_arm_float_to_q15_16),
		ztest_unit_test(test_arm_float_to_q15_17),
		ztest_unit_test(test_arm_float_to_q7_15),
		ztest_unit_test(test_arm_float_to_q7_32),
		ztest_unit_test(test_arm_float_to_q7_33),
		ztest_unit_test(test_arm_weighted_sum_f32_3),
		ztest_unit_test(test_arm_weighted_sum_f32_8),
		ztest_unit_test(test_arm_weighted_sum_f32_11),
		ztest_unit_test(test_arm_sort_out_bitonic_16),
		ztest_unit_test(test_arm_sort_out_bitonic_32),
		ztest_unit_test(test_arm_sort_in_bitonic_32),
		ztest_unit_test(test_arm_sort_const_bitonic_16),
		ztest_unit_test(test_arm_sort_out_bubble_11),
		ztest_unit_test(test_arm_sort_in_bubble_11),
		ztest_unit_test(test_arm_sort_const_bubble_16),
		ztest_unit_test(test_arm_sort_out_heap_11),
		ztest_unit_test(test_arm_sort_in_heap_11),
		ztest_unit_test(test_arm_sort_const_heap_16),
		ztest_unit_test(test_arm_sort_out_insertion_11),
		ztest_unit_test(test_arm_sort_in_insertion_11),
		ztest_unit_test(test_arm_sort_const_insertion_16),
		ztest_unit_test(test_arm_sort_out_quick_11),
		ztest_unit_test(test_arm_sort_in_quick_11),
		ztest_unit_test(test_arm_sort_const_quick_16),
		ztest_unit_test(test_arm_sort_out_selection_11),
		ztest_unit_test(test_arm_sort_in_selection_11),
		ztest_unit_test(test_arm_sort_const_selection_16),
		ztest_unit_test(test_arm_merge_sort_out_11),
		ztest_unit_test(test_arm_merge_sort_const_16),
		ztest_unit_test(test_arm_spline_square_20),
		ztest_unit_test(test_arm_spline_sine_33),
		ztest_unit_test(test_arm_spline_ramp_30)
		);

	ztest_run_test_suite(support_f32);
}
