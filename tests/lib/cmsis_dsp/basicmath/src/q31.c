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

#define SNR_ERROR_THRESH	((float32_t)100)
#define ABS_ERROR_THRESH_Q31	((q31_t)4)
#define ABS_ERROR_THRESH_Q63	((q63_t)(1 << 17))

static void test_arm_add_q31(
	const q31_t *input1, const q31_t *input2, const q31_t *ref, size_t length)
{
	q31_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(q31_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_add_q31(input1, input2, output, length);

	/* Validate output */
	zassert_true(
		test_snr_error_q31(length, output, ref, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_near_equal_q31(length, output, ref, ABS_ERROR_THRESH_Q31),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT4(arm_add_q31, 3, in_com1, in_com2, ref_add, 3);
DEFINE_TEST_VARIANT4(arm_add_q31, 8, in_com1, in_com2, ref_add, 8);
DEFINE_TEST_VARIANT4(arm_add_q31, 11, in_com1, in_com2, ref_add, 11);
DEFINE_TEST_VARIANT4(arm_add_q31, possat, in_maxpos, in_maxpos, ref_add_possat, 9);
DEFINE_TEST_VARIANT4(arm_add_q31, negsat, in_maxneg, in_maxneg, ref_add_negsat, 9);

static void test_arm_sub_q31(
	const q31_t *input1, const q31_t *input2, const q31_t *ref, size_t length)
{
	q31_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(q31_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_sub_q31(input1, input2, output, length);

	/* Validate output */
	zassert_true(
		test_snr_error_q31(length, output, ref, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_near_equal_q31(length, output, ref, ABS_ERROR_THRESH_Q31),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT4(arm_sub_q31, 3, in_com1, in_com2, ref_sub, 3);
DEFINE_TEST_VARIANT4(arm_sub_q31, 8, in_com1, in_com2, ref_sub, 8);
DEFINE_TEST_VARIANT4(arm_sub_q31, 11, in_com1, in_com2, ref_sub, 11);
DEFINE_TEST_VARIANT4(arm_sub_q31, possat, in_maxpos, in_maxneg, ref_sub_possat, 9);
DEFINE_TEST_VARIANT4(arm_sub_q31, negsat, in_maxneg, in_maxpos, ref_sub_negsat, 9);

static void test_arm_mult_q31(
	const q31_t *input1, const q31_t *input2, const q31_t *ref, size_t length)
{
	q31_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(q31_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_mult_q31(input1, input2, output, length);

	/* Validate output */
	zassert_true(
		test_snr_error_q31(length, output, ref, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_near_equal_q31(length, output, ref, ABS_ERROR_THRESH_Q31),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT4(arm_mult_q31, 3, in_com1, in_com2, ref_mult, 3);
DEFINE_TEST_VARIANT4(arm_mult_q31, 8, in_com1, in_com2, ref_mult, 8);
DEFINE_TEST_VARIANT4(arm_mult_q31, 11, in_com1, in_com2, ref_mult, 11);
DEFINE_TEST_VARIANT4(arm_mult_q31, possat, in_maxneg2, in_maxneg2, ref_mult_possat, 9);

static void test_arm_negate_q31(
	const q31_t *input1, const q31_t *ref, size_t length)
{
	q31_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(q31_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_negate_q31(input1, output, length);

	/* Validate output */
	zassert_true(
		test_snr_error_q31(length, output, ref, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_near_equal_q31(length, output, ref, ABS_ERROR_THRESH_Q31),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT3(arm_negate_q31, 3, in_com1, ref_negate, 3);
DEFINE_TEST_VARIANT3(arm_negate_q31, 8, in_com1, ref_negate, 8);
DEFINE_TEST_VARIANT3(arm_negate_q31, 11, in_com1, ref_negate, 11);
DEFINE_TEST_VARIANT3(arm_negate_q31, possat, in_maxneg2, ref_negate_possat, 9);

static void test_arm_offset_q31(
	const q31_t *input1, q31_t scalar, const q31_t *ref, size_t length)
{
	q31_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(q31_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_offset_q31(input1, scalar, output, length);

	/* Validate output */
	zassert_true(
		test_snr_error_q31(length, output, ref, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_near_equal_q31(length, output, ref, ABS_ERROR_THRESH_Q31),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT4(arm_offset_q31, 0p5_3, in_com1, 0x40000000, ref_offset, 3);
DEFINE_TEST_VARIANT4(arm_offset_q31, 0p5_8, in_com1, 0x40000000, ref_offset, 8);
DEFINE_TEST_VARIANT4(arm_offset_q31, 0p5_11, in_com1, 0x40000000, ref_offset, 11);
DEFINE_TEST_VARIANT4(arm_offset_q31, possat, in_maxpos, 0x73333333, ref_offset_possat, 9);
DEFINE_TEST_VARIANT4(arm_offset_q31, negsat, in_maxneg, 0x8ccccccd, ref_offset_negsat, 9);

static void test_arm_scale_q31(
	const q31_t *input1, q31_t scalar, const q31_t *ref, size_t length)
{
	q31_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(q31_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_scale_q31(input1, scalar, 0, output, length);

	/* Validate output */
	zassert_true(
		test_snr_error_q31(length, output, ref, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_near_equal_q31(length, output, ref, ABS_ERROR_THRESH_Q31),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT4(arm_scale_q31, 0p5_3, in_com1, 0x40000000, ref_scale, 3);
DEFINE_TEST_VARIANT4(arm_scale_q31, 0p5_8, in_com1, 0x40000000, ref_scale, 8);
DEFINE_TEST_VARIANT4(arm_scale_q31, 0p5_11, in_com1, 0x40000000, ref_scale, 11);
DEFINE_TEST_VARIANT4(arm_scale_q31, possat, in_maxneg2, 0x80000000, ref_scale_possat, 9);

static void test_arm_dot_prod_q31(
	const q31_t *input1, const q31_t *input2, const q63_t *ref, size_t length)
{
	q63_t *output;

	/* Allocate output buffer */
	output = malloc(1 * sizeof(q63_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_dot_prod_q31(input1, input2, length, &output[0]);

	/* Validate output */
	zassert_true(
		test_snr_error_q63(1, output, ref, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_near_equal_q63(1, output, ref, ABS_ERROR_THRESH_Q63),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT4(arm_dot_prod_q31, 3, in_com1, in_com2, ref_dot_prod_3, 3);
DEFINE_TEST_VARIANT4(arm_dot_prod_q31, 8, in_com1, in_com2, ref_dot_prod_4, 8);
DEFINE_TEST_VARIANT4(arm_dot_prod_q31, 11, in_com1, in_com2, ref_dot_prod_4n1, 11);

static void test_arm_abs_q31(
	const q31_t *input1, const q31_t *ref, size_t length)
{
	q31_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(q31_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_abs_q31(input1, output, length);

	/* Validate output */
	zassert_true(
		test_snr_error_q31(length, output, ref, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_near_equal_q31(length, output, ref, ABS_ERROR_THRESH_Q31),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT3(arm_abs_q31, 3, in_com1, ref_abs, 3);
DEFINE_TEST_VARIANT3(arm_abs_q31, 8, in_com1, ref_abs, 8);
DEFINE_TEST_VARIANT3(arm_abs_q31, 11, in_com1, ref_abs, 11);

static void test_arm_shift_q31(
	const q31_t *input1, const q31_t *ref, size_t length)
{
	q31_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(q31_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_shift_q31(input1, 1, output, length);

	/* Validate output */
	zassert_true(
		test_snr_error_q31(length, output, ref, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_near_equal_q31(length, output, ref, ABS_ERROR_THRESH_Q31),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT3(arm_shift_q31, rand, in_rand, ref_shift, 9);
DEFINE_TEST_VARIANT3(arm_shift_q31, possat, in_maxpos, ref_shift_possat, 9);
DEFINE_TEST_VARIANT3(arm_shift_q31, negsat, in_maxneg, ref_shift_negsat, 9);

static void test_arm_and_u32(
	const uint32_t *input1, const uint32_t *input2, const uint32_t *ref, size_t length)
{
	uint32_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(uint32_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_and_u32(input1, input2, output, length);

	/* Validate output */
	zassert_true(
		test_equal_q31(length, output, ref),
		ASSERT_MSG_INCORRECT_COMP_RESULT);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT4(arm_and_u32, 3, in_bitwise1, in_bitwise2, ref_and, 3);
DEFINE_TEST_VARIANT4(arm_and_u32, 8, in_bitwise1, in_bitwise2, ref_and, 8);
DEFINE_TEST_VARIANT4(arm_and_u32, 11, in_bitwise1, in_bitwise2, ref_and, 11);

static void test_arm_or_u32(
	const uint32_t *input1, const uint32_t *input2, const uint32_t *ref, size_t length)
{
	uint32_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(uint32_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_or_u32(input1, input2, output, length);

	/* Validate output */
	zassert_true(
		test_equal_q31(length, output, ref),
		ASSERT_MSG_INCORRECT_COMP_RESULT);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT4(arm_or_u32, 3, in_bitwise1, in_bitwise2, ref_or, 3);
DEFINE_TEST_VARIANT4(arm_or_u32, 8, in_bitwise1, in_bitwise2, ref_or, 8);
DEFINE_TEST_VARIANT4(arm_or_u32, 11, in_bitwise1, in_bitwise2, ref_or, 11);

static void test_arm_not_u32(
	const uint32_t *input1, const uint32_t *ref, size_t length)
{
	uint32_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(uint32_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_not_u32(input1, output, length);

	/* Validate output */
	zassert_true(
		test_equal_q31(length, output, ref),
		ASSERT_MSG_INCORRECT_COMP_RESULT);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT3(arm_not_u32, 3, in_bitwise1, ref_not, 3);
DEFINE_TEST_VARIANT3(arm_not_u32, 8, in_bitwise1, ref_not, 8);
DEFINE_TEST_VARIANT3(arm_not_u32, 11, in_bitwise1, ref_not, 11);

static void test_arm_xor_u32(
	const uint32_t *input1, const uint32_t *input2, const uint32_t *ref, size_t length)
{
	uint32_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(uint32_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_xor_u32(input1, input2, output, length);

	/* Validate output */
	zassert_true(
		test_equal_q31(length, output, ref),
		ASSERT_MSG_INCORRECT_COMP_RESULT);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT4(arm_xor_u32, 3, in_bitwise1, in_bitwise2, ref_xor, 3);
DEFINE_TEST_VARIANT4(arm_xor_u32, 8, in_bitwise1, in_bitwise2, ref_xor, 8);
DEFINE_TEST_VARIANT4(arm_xor_u32, 11, in_bitwise1, in_bitwise2, ref_xor, 11);

void test_basicmath_q31(void)
{
	ztest_test_suite(basicmath_q31,
		ztest_unit_test(test_arm_add_q31_3),
		ztest_unit_test(test_arm_add_q31_8),
		ztest_unit_test(test_arm_add_q31_11),
		ztest_unit_test(test_arm_sub_q31_3),
		ztest_unit_test(test_arm_sub_q31_8),
		ztest_unit_test(test_arm_sub_q31_11),
		ztest_unit_test(test_arm_mult_q31_3),
		ztest_unit_test(test_arm_mult_q31_8),
		ztest_unit_test(test_arm_mult_q31_11),
		ztest_unit_test(test_arm_negate_q31_3),
		ztest_unit_test(test_arm_negate_q31_8),
		ztest_unit_test(test_arm_negate_q31_11),
		ztest_unit_test(test_arm_offset_q31_0p5_3),
		ztest_unit_test(test_arm_offset_q31_0p5_8),
		ztest_unit_test(test_arm_offset_q31_0p5_11),
		ztest_unit_test(test_arm_scale_q31_0p5_3),
		ztest_unit_test(test_arm_scale_q31_0p5_8),
		ztest_unit_test(test_arm_scale_q31_0p5_11),
		ztest_unit_test(test_arm_dot_prod_q31_3),
		ztest_unit_test(test_arm_dot_prod_q31_8),
		ztest_unit_test(test_arm_dot_prod_q31_11),
		ztest_unit_test(test_arm_abs_q31_3),
		ztest_unit_test(test_arm_abs_q31_8),
		ztest_unit_test(test_arm_abs_q31_11),
		ztest_unit_test(test_arm_shift_q31_rand),
		ztest_unit_test(test_arm_add_q31_possat),
		ztest_unit_test(test_arm_add_q31_negsat),
		ztest_unit_test(test_arm_sub_q31_possat),
		ztest_unit_test(test_arm_sub_q31_negsat),
		ztest_unit_test(test_arm_mult_q31_possat),
		ztest_unit_test(test_arm_negate_q31_possat),
		ztest_unit_test(test_arm_offset_q31_possat),
		ztest_unit_test(test_arm_offset_q31_negsat),
		ztest_unit_test(test_arm_scale_q31_possat),
		ztest_unit_test(test_arm_shift_q31_possat),
		ztest_unit_test(test_arm_shift_q31_negsat),
		ztest_unit_test(test_arm_and_u32_3),
		ztest_unit_test(test_arm_and_u32_8),
		ztest_unit_test(test_arm_and_u32_11),
		ztest_unit_test(test_arm_or_u32_3),
		ztest_unit_test(test_arm_or_u32_8),
		ztest_unit_test(test_arm_or_u32_11),
		ztest_unit_test(test_arm_not_u32_3),
		ztest_unit_test(test_arm_not_u32_8),
		ztest_unit_test(test_arm_not_u32_11),
		ztest_unit_test(test_arm_xor_u32_3),
		ztest_unit_test(test_arm_xor_u32_8),
		ztest_unit_test(test_arm_xor_u32_11)
		);

	ztest_run_test_suite(basicmath_q31);
}
