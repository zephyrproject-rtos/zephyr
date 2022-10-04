/*
 * Copyright (c) 2021 Stephanos Ioannidis <root@stephanos.io>
 * Copyright (C) 2010-2021 ARM Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <zephyr/zephyr.h>
#include <stdlib.h>
#include <arm_math.h>
#include "../../common/test_common.h"

#include "q7.pat"

#define SNR_ERROR_THRESH	((float32_t)20)
#define ABS_ERROR_THRESH_Q7	((q7_t)2)
#define ABS_ERROR_THRESH_Q31	((q31_t)(1 << 15))

static void test_arm_add_q7(
	const q7_t *input1, const q7_t *input2, const q7_t *ref, size_t length)
{
	q7_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(q7_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_add_q7(input1, input2, output, length);

	/* Validate output */
	zassert_true(
		test_snr_error_q7(length, output, ref, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_near_equal_q7(length, output, ref, ABS_ERROR_THRESH_Q7),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT4(basic_math_q7, arm_add_q7, 15, in_com1, in_com2, ref_add, 15);
DEFINE_TEST_VARIANT4(basic_math_q7, arm_add_q7, 32, in_com1, in_com2, ref_add, 32);
DEFINE_TEST_VARIANT4(basic_math_q7, arm_add_q7, 47, in_com1, in_com2, ref_add, 47);
DEFINE_TEST_VARIANT4(basic_math_q7, arm_add_q7, possat, in_maxpos, in_maxpos, ref_add_possat, 33);
DEFINE_TEST_VARIANT4(basic_math_q7, arm_add_q7, negsat, in_maxneg, in_maxneg, ref_add_negsat, 33);
DEFINE_TEST_VARIANT4(basic_math_q7, arm_add_q7, long, in_com1, in_com2, ref_add,
		     ARRAY_SIZE(in_com1));

static void test_arm_sub_q7(
	const q7_t *input1, const q7_t *input2, const q7_t *ref, size_t length)
{
	q7_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(q7_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_sub_q7(input1, input2, output, length);

	/* Validate output */
	zassert_true(
		test_snr_error_q7(length, output, ref, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_near_equal_q7(length, output, ref, ABS_ERROR_THRESH_Q7),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT4(basic_math_q7, arm_sub_q7, 15, in_com1, in_com2, ref_sub, 15);
DEFINE_TEST_VARIANT4(basic_math_q7, arm_sub_q7, 32, in_com1, in_com2, ref_sub, 32);
DEFINE_TEST_VARIANT4(basic_math_q7, arm_sub_q7, 47, in_com1, in_com2, ref_sub, 47);
DEFINE_TEST_VARIANT4(basic_math_q7, arm_sub_q7, possat, in_maxpos, in_maxneg, ref_sub_possat, 33);
DEFINE_TEST_VARIANT4(basic_math_q7, arm_sub_q7, negsat, in_maxneg, in_maxpos, ref_sub_negsat, 33);
DEFINE_TEST_VARIANT4(basic_math_q7, arm_sub_q7, long, in_com1, in_com2, ref_sub,
		     ARRAY_SIZE(in_com1));

static void test_arm_mult_q7(
	const q7_t *input1, const q7_t *input2, const q7_t *ref, size_t length)
{
	q7_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(q7_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_mult_q7(input1, input2, output, length);

	/* Validate output */
	zassert_true(
		test_snr_error_q7(length, output, ref, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_near_equal_q7(length, output, ref, ABS_ERROR_THRESH_Q7),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT4(basic_math_q7, arm_mult_q7, 15, in_com1, in_com2, ref_mult, 15);
DEFINE_TEST_VARIANT4(basic_math_q7, arm_mult_q7, 32, in_com1, in_com2, ref_mult, 32);
DEFINE_TEST_VARIANT4(basic_math_q7, arm_mult_q7, 47, in_com1, in_com2, ref_mult, 47);
DEFINE_TEST_VARIANT4(basic_math_q7, arm_mult_q7, possat, in_maxneg2, in_maxneg2, ref_mult_possat,
		     33);
DEFINE_TEST_VARIANT4(basic_math_q7, arm_mult_q7, long, in_com1, in_com2, ref_mult,
		     ARRAY_SIZE(in_com1));

static void test_arm_negate_q7(
	const q7_t *input1, const q7_t *ref, size_t length)
{
	q7_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(q7_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_negate_q7(input1, output, length);

	/* Validate output */
	zassert_true(
		test_snr_error_q7(length, output, ref, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_near_equal_q7(length, output, ref, ABS_ERROR_THRESH_Q7),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT3(basic_math_q7, arm_negate_q7, 15, in_com1, ref_negate, 15);
DEFINE_TEST_VARIANT3(basic_math_q7, arm_negate_q7, 32, in_com1, ref_negate, 32);
DEFINE_TEST_VARIANT3(basic_math_q7, arm_negate_q7, 47, in_com1, ref_negate, 47);
DEFINE_TEST_VARIANT3(basic_math_q7, arm_negate_q7, possat, in_maxneg2, ref_negate_possat, 33);
DEFINE_TEST_VARIANT3(basic_math_q7, arm_negate_q7, long, in_com1, ref_negate, ARRAY_SIZE(in_com1));

static void test_arm_offset_q7(
	const q7_t *input1, q7_t scalar, const q7_t *ref, size_t length)
{
	q7_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(q7_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_offset_q7(input1, scalar, output, length);

	/* Validate output */
	zassert_true(
		test_snr_error_q7(length, output, ref, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_near_equal_q7(length, output, ref, ABS_ERROR_THRESH_Q7),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT4(basic_math_q7, arm_offset_q7, 0p5_15, in_com1, 0x40, ref_offset, 15);
DEFINE_TEST_VARIANT4(basic_math_q7, arm_offset_q7, 0p5_32, in_com1, 0x40, ref_offset, 32);
DEFINE_TEST_VARIANT4(basic_math_q7, arm_offset_q7, 0p5_47, in_com1, 0x40, ref_offset, 47);
DEFINE_TEST_VARIANT4(basic_math_q7, arm_offset_q7, possat, in_maxpos, 0x73, ref_offset_possat, 33);
DEFINE_TEST_VARIANT4(basic_math_q7, arm_offset_q7, negsat, in_maxneg, 0x8d, ref_offset_negsat, 33);
DEFINE_TEST_VARIANT4(basic_math_q7, arm_offset_q7, long, in_com1, 0x40, ref_offset,
		     ARRAY_SIZE(in_com1));

static void test_arm_scale_q7(
	const q7_t *input1, q7_t scalar, const q7_t *ref, size_t length)
{
	q7_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(q7_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_scale_q7(input1, scalar, 0, output, length);

	/* Validate output */
	zassert_true(
		test_snr_error_q7(length, output, ref, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_near_equal_q7(length, output, ref, ABS_ERROR_THRESH_Q7),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT4(basic_math_q7, arm_scale_q7, 0p5_15, in_com1, 0x40, ref_scale, 15);
DEFINE_TEST_VARIANT4(basic_math_q7, arm_scale_q7, 0p5_32, in_com1, 0x40, ref_scale, 32);
DEFINE_TEST_VARIANT4(basic_math_q7, arm_scale_q7, 0p5_47, in_com1, 0x40, ref_scale, 47);
DEFINE_TEST_VARIANT4(basic_math_q7, arm_scale_q7, possat, in_maxneg2, 0x80, ref_scale_possat, 33);
DEFINE_TEST_VARIANT4(basic_math_q7, arm_scale_q7, long, in_com1, 0x40, ref_scale,
		     ARRAY_SIZE(in_com1));

static void test_arm_dot_prod_q7(
	const q7_t *input1, const q7_t *input2, const q31_t *ref,
	size_t length)
{
	q31_t *output;

	/* Allocate output buffer */
	output = malloc(1 * sizeof(q31_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_dot_prod_q7(input1, input2, length, &output[0]);

	/* Validate output */
	zassert_true(
		test_snr_error_q31(1, output, ref, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_near_equal_q31(1, output, ref, ABS_ERROR_THRESH_Q31),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT4(basic_math_q7, arm_dot_prod_q7, 15, in_com1, in_com2, ref_dot_prod_3, 15);
DEFINE_TEST_VARIANT4(basic_math_q7, arm_dot_prod_q7, 32, in_com1, in_com2, ref_dot_prod_4, 32);
DEFINE_TEST_VARIANT4(basic_math_q7, arm_dot_prod_q7, 47, in_com1, in_com2, ref_dot_prod_4n1, 47);
DEFINE_TEST_VARIANT4(basic_math_q7, arm_dot_prod_q7, long, in_com1, in_com2, ref_dot_prod_long,
		     ARRAY_SIZE(in_com1));

static void test_arm_abs_q7(
	const q7_t *input1, const q7_t *ref, size_t length)
{
	q7_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(q7_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_abs_q7(input1, output, length);

	/* Validate output */
	zassert_true(
		test_snr_error_q7(length, output, ref, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_near_equal_q7(length, output, ref, ABS_ERROR_THRESH_Q7),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT3(basic_math_q7, arm_abs_q7, 15, in_com1, ref_abs, 15);
DEFINE_TEST_VARIANT3(basic_math_q7, arm_abs_q7, 32, in_com1, ref_abs, 32);
DEFINE_TEST_VARIANT3(basic_math_q7, arm_abs_q7, 47, in_com1, ref_abs, 47);
DEFINE_TEST_VARIANT3(basic_math_q7, arm_abs_q7, long, in_com1, ref_abs, ARRAY_SIZE(ref_abs));

static void test_arm_shift_q7(
	const q7_t *input1, const q7_t *ref, size_t length)
{
	q7_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(q7_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_shift_q7(input1, 1, output, length);

	/* Validate output */
	zassert_true(
		test_snr_error_q7(length, output, ref, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_near_equal_q7(length, output, ref, ABS_ERROR_THRESH_Q7),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT3(basic_math_q7, arm_shift_q7, rand, in_rand, ref_shift, 33);
DEFINE_TEST_VARIANT3(basic_math_q7, arm_shift_q7, possat, in_maxpos, ref_shift_possat, 33);
DEFINE_TEST_VARIANT3(basic_math_q7, arm_shift_q7, negsat, in_maxneg, ref_shift_negsat, 33);

static void test_arm_and_u8(
	const uint8_t *input1, const uint8_t *input2, const uint8_t *ref, size_t length)
{
	uint8_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(uint8_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_and_u8(input1, input2, output, length);

	/* Validate output */
	zassert_true(
		test_equal_q7(length, output, ref),
		ASSERT_MSG_INCORRECT_COMP_RESULT);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT4(basic_math_q7, arm_and_u8, 15, in_bitwise1, in_bitwise2, ref_and, 15);
DEFINE_TEST_VARIANT4(basic_math_q7, arm_and_u8, 32, in_bitwise1, in_bitwise2, ref_and, 32);
DEFINE_TEST_VARIANT4(basic_math_q7, arm_and_u8, 47, in_bitwise1, in_bitwise2, ref_and, 47);

static void test_arm_or_u8(
	const uint8_t *input1, const uint8_t *input2, const uint8_t *ref, size_t length)
{
	uint8_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(uint8_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_or_u8(input1, input2, output, length);

	/* Validate output */
	zassert_true(
		test_equal_q7(length, output, ref),
		ASSERT_MSG_INCORRECT_COMP_RESULT);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT4(basic_math_q7, arm_or_u8, 15, in_bitwise1, in_bitwise2, ref_or, 15);
DEFINE_TEST_VARIANT4(basic_math_q7, arm_or_u8, 32, in_bitwise1, in_bitwise2, ref_or, 32);
DEFINE_TEST_VARIANT4(basic_math_q7, arm_or_u8, 47, in_bitwise1, in_bitwise2, ref_or, 47);

static void test_arm_not_u8(
	const uint8_t *input1, const uint8_t *ref, size_t length)
{
	uint8_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(uint8_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_not_u8(input1, output, length);

	/* Validate output */
	zassert_true(
		test_equal_q7(length, output, ref),
		ASSERT_MSG_INCORRECT_COMP_RESULT);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT3(basic_math_q7, arm_not_u8, 15, in_bitwise1, ref_not, 15);
DEFINE_TEST_VARIANT3(basic_math_q7, arm_not_u8, 32, in_bitwise1, ref_not, 32);
DEFINE_TEST_VARIANT3(basic_math_q7, arm_not_u8, 47, in_bitwise1, ref_not, 47);

static void test_arm_xor_u8(
	const uint8_t *input1, const uint8_t *input2, const uint8_t *ref, size_t length)
{
	uint8_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(uint8_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_xor_u8(input1, input2, output, length);

	/* Validate output */
	zassert_true(
		test_equal_q7(length, output, ref),
		ASSERT_MSG_INCORRECT_COMP_RESULT);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT4(basic_math_q7, arm_xor_u8, 15, in_bitwise1, in_bitwise2, ref_xor, 15);
DEFINE_TEST_VARIANT4(basic_math_q7, arm_xor_u8, 32, in_bitwise1, in_bitwise2, ref_xor, 32);
DEFINE_TEST_VARIANT4(basic_math_q7, arm_xor_u8, 47, in_bitwise1, in_bitwise2, ref_xor, 47);

static void test_arm_clip_q7(
	const q7_t *input, const q7_t *ref, q7_t min, q7_t max, size_t length)
{
	q7_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(q7_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_clip_q7(input, output, min, max, length);

	/* Validate output */
	zassert_true(
		test_equal_q7(length, output, ref),
		ASSERT_MSG_INCORRECT_COMP_RESULT);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT5(basic_math_q7, arm_clip_q7, c0_f3, in_clip, ref_clip1, 0xc0, 0xf3,
		     ARRAY_SIZE(ref_clip1));
DEFINE_TEST_VARIANT5(basic_math_q7, arm_clip_q7, c0_40, in_clip, ref_clip2, 0xc0, 0x40,
		     ARRAY_SIZE(ref_clip2));
DEFINE_TEST_VARIANT5(basic_math_q7, arm_clip_q7, 0d_40, in_clip, ref_clip3, 0x0d, 0x40,
		     ARRAY_SIZE(ref_clip3));

ZTEST_SUITE(basic_math_q7, NULL, NULL, NULL, NULL, NULL);
