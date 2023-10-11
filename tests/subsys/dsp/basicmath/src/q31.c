/*
 * Copyright (c) 2021 Stephanos Ioannidis <root@stephanos.io>
 * Copyright (C) 2010-2021 ARM Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/dsp/dsp.h>
#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <stdlib.h>
#include "common/test_common.h"

#include "q31.pat"

#define SNR_ERROR_THRESH	((float32_t)100)
#define ABS_ERROR_THRESH_Q31	((q31_t)4)
#define ABS_ERROR_THRESH_Q63	((q63_t)(1 << 17))

static void test_zdsp_add_q31(
	const q31_t *input1, const q31_t *input2, const q31_t *ref, size_t length)
{
	q31_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(q31_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	zdsp_add_q31(input1, input2, output, length);

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

DEFINE_TEST_VARIANT4(basic_math_q31, zdsp_add_q31, 3, in_com1, in_com2, ref_add, 3);
DEFINE_TEST_VARIANT4(basic_math_q31, zdsp_add_q31, 8, in_com1, in_com2, ref_add, 8);
DEFINE_TEST_VARIANT4(basic_math_q31, zdsp_add_q31, 11, in_com1, in_com2, ref_add, 11);
DEFINE_TEST_VARIANT4(basic_math_q31, zdsp_add_q31, possat, in_maxpos, in_maxpos, ref_add_possat, 9);
DEFINE_TEST_VARIANT4(basic_math_q31, zdsp_add_q31, negsat, in_maxneg, in_maxneg, ref_add_negsat, 9);
DEFINE_TEST_VARIANT4(basic_math_q31, zdsp_add_q31, long, in_com1, in_com2, ref_add,
		     ARRAY_SIZE(in_com1));

static void test_zdsp_sub_q31(
	const q31_t *input1, const q31_t *input2, const q31_t *ref, size_t length)
{
	q31_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(q31_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	zdsp_sub_q31(input1, input2, output, length);

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

DEFINE_TEST_VARIANT4(basic_math_q31, zdsp_sub_q31, 3, in_com1, in_com2, ref_sub, 3);
DEFINE_TEST_VARIANT4(basic_math_q31, zdsp_sub_q31, 8, in_com1, in_com2, ref_sub, 8);
DEFINE_TEST_VARIANT4(basic_math_q31, zdsp_sub_q31, 11, in_com1, in_com2, ref_sub, 11);
DEFINE_TEST_VARIANT4(basic_math_q31, zdsp_sub_q31, possat, in_maxpos, in_maxneg, ref_sub_possat, 9);
DEFINE_TEST_VARIANT4(basic_math_q31, zdsp_sub_q31, negsat, in_maxneg, in_maxpos, ref_sub_negsat, 9);
DEFINE_TEST_VARIANT4(basic_math_q31, zdsp_sub_q31, long, in_com1, in_com2, ref_sub,
		     ARRAY_SIZE(in_com1));

static void test_zdsp_mult_q31(
	const q31_t *input1, const q31_t *input2, const q31_t *ref, size_t length)
{
	q31_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(q31_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	zdsp_mult_q31(input1, input2, output, length);

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

DEFINE_TEST_VARIANT4(basic_math_q31, zdsp_mult_q31, 3, in_com1, in_com2, ref_mult, 3);
DEFINE_TEST_VARIANT4(basic_math_q31, zdsp_mult_q31, 8, in_com1, in_com2, ref_mult, 8);
DEFINE_TEST_VARIANT4(basic_math_q31, zdsp_mult_q31, 11, in_com1, in_com2, ref_mult, 11);
DEFINE_TEST_VARIANT4(basic_math_q31, zdsp_mult_q31, possat, in_maxneg2, in_maxneg2, ref_mult_possat,
		     9);
DEFINE_TEST_VARIANT4(basic_math_q31, zdsp_mult_q31, long, in_com1, in_com2, ref_mult,
		     ARRAY_SIZE(in_com1));

static void test_zdsp_negate_q31(
	const q31_t *input1, const q31_t *ref, size_t length)
{
	q31_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(q31_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	zdsp_negate_q31(input1, output, length);

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

DEFINE_TEST_VARIANT3(basic_math_q31, zdsp_negate_q31, 3, in_com1, ref_negate, 3);
DEFINE_TEST_VARIANT3(basic_math_q31, zdsp_negate_q31, 8, in_com1, ref_negate, 8);
DEFINE_TEST_VARIANT3(basic_math_q31, zdsp_negate_q31, 11, in_com1, ref_negate, 11);
DEFINE_TEST_VARIANT3(basic_math_q31, zdsp_negate_q31, possat, in_maxneg2, ref_negate_possat, 9);
DEFINE_TEST_VARIANT3(basic_math_q31, zdsp_negate_q31, long, in_com1, ref_negate,
		     ARRAY_SIZE(in_com1));

static void test_zdsp_offset_q31(
	const q31_t *input1, q31_t scalar, const q31_t *ref, size_t length)
{
	q31_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(q31_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	zdsp_offset_q31(input1, scalar, output, length);

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

DEFINE_TEST_VARIANT4(basic_math_q31, zdsp_offset_q31, 0p5_3, in_com1, 0x40000000, ref_offset, 3);
DEFINE_TEST_VARIANT4(basic_math_q31, zdsp_offset_q31, 0p5_8, in_com1, 0x40000000, ref_offset, 8);
DEFINE_TEST_VARIANT4(basic_math_q31, zdsp_offset_q31, 0p5_11, in_com1, 0x40000000, ref_offset, 11);
DEFINE_TEST_VARIANT4(basic_math_q31, zdsp_offset_q31, possat, in_maxpos, 0x73333333,
		     ref_offset_possat, 9);
DEFINE_TEST_VARIANT4(basic_math_q31, zdsp_offset_q31, negsat, in_maxneg, 0x8ccccccd,
		     ref_offset_negsat, 9);
DEFINE_TEST_VARIANT4(basic_math_q31, zdsp_offset_q31, long, in_com1, 0x40000000, ref_offset,
		     ARRAY_SIZE(in_com1));

static void test_zdsp_scale_q31(
	const q31_t *input1, q31_t scalar, const q31_t *ref, size_t length)
{
	q31_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(q31_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	zdsp_scale_q31(input1, scalar, 0, output, length);

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

DEFINE_TEST_VARIANT4(basic_math_q31, zdsp_scale_q31, 0p5_3, in_com1, 0x40000000, ref_scale, 3);
DEFINE_TEST_VARIANT4(basic_math_q31, zdsp_scale_q31, 0p5_8, in_com1, 0x40000000, ref_scale, 8);
DEFINE_TEST_VARIANT4(basic_math_q31, zdsp_scale_q31, 0p5_11, in_com1, 0x40000000, ref_scale, 11);
DEFINE_TEST_VARIANT4(basic_math_q31, zdsp_scale_q31, possat, in_maxneg2, 0x80000000,
		     ref_scale_possat, 9);
DEFINE_TEST_VARIANT4(basic_math_q31, zdsp_scale_q31, long, in_com1, 0x40000000, ref_scale,
		     ARRAY_SIZE(in_com1));

static void test_zdsp_dot_prod_q31(
	const q31_t *input1, const q31_t *input2, const q63_t *ref, size_t length)
{
	q63_t *output;

	/* Allocate output buffer */
	output = malloc(1 * sizeof(q63_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	zdsp_dot_prod_q31(input1, input2, length, &output[0]);

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

DEFINE_TEST_VARIANT4(basic_math_q31, zdsp_dot_prod_q31, 3, in_com1, in_com2, ref_dot_prod_3, 3);
DEFINE_TEST_VARIANT4(basic_math_q31, zdsp_dot_prod_q31, 8, in_com1, in_com2, ref_dot_prod_4, 8);
DEFINE_TEST_VARIANT4(basic_math_q31, zdsp_dot_prod_q31, 11, in_com1, in_com2, ref_dot_prod_4n1, 11);
DEFINE_TEST_VARIANT4(basic_math_q31, zdsp_dot_prod_q31, long, in_com1, in_com2, ref_dot_prod_long,
		     ARRAY_SIZE(in_com1));

static void test_zdsp_abs_q31(
	const q31_t *input1, const q31_t *ref, size_t length)
{
	q31_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(q31_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	zdsp_abs_q31(input1, output, length);

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

DEFINE_TEST_VARIANT3(basic_math_q31, zdsp_abs_q31, 3, in_com1, ref_abs, 3);
DEFINE_TEST_VARIANT3(basic_math_q31, zdsp_abs_q31, 8, in_com1, ref_abs, 8);
DEFINE_TEST_VARIANT3(basic_math_q31, zdsp_abs_q31, 11, in_com1, ref_abs, 11);
DEFINE_TEST_VARIANT3(basic_math_q31, zdsp_abs_q31, long, in_com1, ref_abs, ARRAY_SIZE(in_com1));

static void test_zdsp_shift_q31(
	const q31_t *input1, const q31_t *ref, size_t length)
{
	q31_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(q31_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	zdsp_shift_q31(input1, 1, output, length);

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

DEFINE_TEST_VARIANT3(basic_math_q31, zdsp_shift_q31, rand, in_rand, ref_shift, 9);
DEFINE_TEST_VARIANT3(basic_math_q31, zdsp_shift_q31, possat, in_maxpos, ref_shift_possat, 9);
DEFINE_TEST_VARIANT3(basic_math_q31, zdsp_shift_q31, negsat, in_maxneg, ref_shift_negsat, 9);

static void test_zdsp_and_u32(
	const uint32_t *input1, const uint32_t *input2, const uint32_t *ref, size_t length)
{
	uint32_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(uint32_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	zdsp_and_u32(input1, input2, output, length);

	/* Validate output */
	zassert_true(
		test_equal_q31(length, output, ref),
		ASSERT_MSG_INCORRECT_COMP_RESULT);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT4(basic_math_q31, zdsp_and_u32, 3, in_bitwise1, in_bitwise2, ref_and, 3);
DEFINE_TEST_VARIANT4(basic_math_q31, zdsp_and_u32, 8, in_bitwise1, in_bitwise2, ref_and, 8);
DEFINE_TEST_VARIANT4(basic_math_q31, zdsp_and_u32, 11, in_bitwise1, in_bitwise2, ref_and, 11);

static void test_zdsp_or_u32(
	const uint32_t *input1, const uint32_t *input2, const uint32_t *ref, size_t length)
{
	uint32_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(uint32_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	zdsp_or_u32(input1, input2, output, length);

	/* Validate output */
	zassert_true(
		test_equal_q31(length, output, ref),
		ASSERT_MSG_INCORRECT_COMP_RESULT);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT4(basic_math_q31, zdsp_or_u32, 3, in_bitwise1, in_bitwise2, ref_or, 3);
DEFINE_TEST_VARIANT4(basic_math_q31, zdsp_or_u32, 8, in_bitwise1, in_bitwise2, ref_or, 8);
DEFINE_TEST_VARIANT4(basic_math_q31, zdsp_or_u32, 11, in_bitwise1, in_bitwise2, ref_or, 11);

static void test_zdsp_not_u32(
	const uint32_t *input1, const uint32_t *ref, size_t length)
{
	uint32_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(uint32_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	zdsp_not_u32(input1, output, length);

	/* Validate output */
	zassert_true(
		test_equal_q31(length, output, ref),
		ASSERT_MSG_INCORRECT_COMP_RESULT);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT3(basic_math_q31, zdsp_not_u32, 3, in_bitwise1, ref_not, 3);
DEFINE_TEST_VARIANT3(basic_math_q31, zdsp_not_u32, 8, in_bitwise1, ref_not, 8);
DEFINE_TEST_VARIANT3(basic_math_q31, zdsp_not_u32, 11, in_bitwise1, ref_not, 11);

static void test_zdsp_xor_u32(
	const uint32_t *input1, const uint32_t *input2, const uint32_t *ref, size_t length)
{
	uint32_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(uint32_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	zdsp_xor_u32(input1, input2, output, length);

	/* Validate output */
	zassert_true(
		test_equal_q31(length, output, ref),
		ASSERT_MSG_INCORRECT_COMP_RESULT);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT4(basic_math_q31, zdsp_xor_u32, 3, in_bitwise1, in_bitwise2, ref_xor, 3);
DEFINE_TEST_VARIANT4(basic_math_q31, zdsp_xor_u32, 8, in_bitwise1, in_bitwise2, ref_xor, 8);
DEFINE_TEST_VARIANT4(basic_math_q31, zdsp_xor_u32, 11, in_bitwise1, in_bitwise2, ref_xor, 11);

static void test_zdsp_clip_q31(
	const q31_t *input, const q31_t *ref, q31_t min, q31_t max, size_t length)
{
	q31_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(q31_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	zdsp_clip_q31(input, output, min, max, length);

	/* Validate output */
	zassert_true(
		test_equal_q31(length, output, ref),
		ASSERT_MSG_INCORRECT_COMP_RESULT);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT5(basic_math_q31, zdsp_clip_q31, c0000000_f3333333, in_clip, ref_clip1,
		     0xc0000000, 0xf3333333, ARRAY_SIZE(ref_clip1));
DEFINE_TEST_VARIANT5(basic_math_q31, zdsp_clip_q31, c0000000_40000000, in_clip, ref_clip2,
		     0xc0000000, 0x40000000, ARRAY_SIZE(ref_clip2));
DEFINE_TEST_VARIANT5(basic_math_q31, zdsp_clip_q31, 0ccccccd_40000000, in_clip, ref_clip3,
		     0x0ccccccd, 0x40000000, ARRAY_SIZE(ref_clip3));

ZTEST_SUITE(basic_math_q31, NULL, NULL, NULL, NULL, NULL);
