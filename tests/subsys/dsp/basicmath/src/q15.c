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

#include "q15.pat"

#define SNR_ERROR_THRESH	((float32_t)70)
#define SNR_ERROR_THRESH_HIGH	((float32_t)60)
#define ABS_ERROR_THRESH_Q15	((q15_t)2)
#define ABS_ERROR_THRESH_Q63	((q63_t)(1 << 17))

static void test_zdsp_add_q15(
	const q15_t *input1, const q15_t *input2, const q15_t *ref,
	size_t length)
{
	q15_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(q15_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	zdsp_add_q15(input1, input2, output, length);

	/* Validate output */
	zassert_true(
		test_snr_error_q15(length, output, ref, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_near_equal_q15(length, output, ref, ABS_ERROR_THRESH_Q15),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT4(basic_math_q15, zdsp_add_q15, 7, in_com1, in_com2, ref_add, 7);
DEFINE_TEST_VARIANT4(basic_math_q15, zdsp_add_q15, 16, in_com1, in_com2, ref_add, 16);
DEFINE_TEST_VARIANT4(basic_math_q15, zdsp_add_q15, 23, in_com1, in_com2, ref_add, 23);
DEFINE_TEST_VARIANT4(basic_math_q15, zdsp_add_q15, possat, in_maxpos, in_maxpos, ref_add_possat,
		     17);
DEFINE_TEST_VARIANT4(basic_math_q15, zdsp_add_q15, negsat, in_maxneg, in_maxneg, ref_add_negsat,
		     17);
DEFINE_TEST_VARIANT4(basic_math_q15, zdsp_add_q15, long, in_com1, in_com2, ref_add,
		     ARRAY_SIZE(in_com1));

static void test_zdsp_sub_q15(
	const q15_t *input1, const q15_t *input2, const q15_t *ref,
	size_t length)
{
	q15_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(q15_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	zdsp_sub_q15(input1, input2, output, length);

	/* Validate output */
	zassert_true(
		test_snr_error_q15(length, output, ref, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_near_equal_q15(length, output, ref, ABS_ERROR_THRESH_Q15),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT4(basic_math_q15, zdsp_sub_q15, 7, in_com1, in_com2, ref_sub, 7);
DEFINE_TEST_VARIANT4(basic_math_q15, zdsp_sub_q15, 16, in_com1, in_com2, ref_sub, 16);
DEFINE_TEST_VARIANT4(basic_math_q15, zdsp_sub_q15, 23, in_com1, in_com2, ref_sub, 23);
DEFINE_TEST_VARIANT4(basic_math_q15, zdsp_sub_q15, possat, in_maxpos, in_maxneg, ref_sub_possat,
		     17);
DEFINE_TEST_VARIANT4(basic_math_q15, zdsp_sub_q15, negsat, in_maxneg, in_maxpos, ref_sub_negsat,
		     17);
DEFINE_TEST_VARIANT4(basic_math_q15, zdsp_sub_q15, long, in_com1, in_com2, ref_sub,
		     ARRAY_SIZE(in_com1));

static void test_zdsp_mult_q15(
	const q15_t *input1, const q15_t *input2, const q15_t *ref,
	size_t length)
{
	q15_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(q15_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	zdsp_mult_q15(input1, input2, output, length);

	/* Validate output */
	zassert_true(
		test_snr_error_q15(length, output, ref, SNR_ERROR_THRESH_HIGH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_near_equal_q15(length, output, ref, ABS_ERROR_THRESH_Q15),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT4(basic_math_q15, zdsp_mult_q15, 7, in_com1, in_com2, ref_mult, 7);
DEFINE_TEST_VARIANT4(basic_math_q15, zdsp_mult_q15, 16, in_com1, in_com2, ref_mult, 16);
DEFINE_TEST_VARIANT4(basic_math_q15, zdsp_mult_q15, 23, in_com1, in_com2, ref_mult, 23);
DEFINE_TEST_VARIANT4(basic_math_q15, zdsp_mult_q15, possat, in_maxneg2, in_maxneg2, ref_mult_possat,
		     17);
DEFINE_TEST_VARIANT4(basic_math_q15, zdsp_mult_q15, long, in_com1, in_com2, ref_mult,
		     ARRAY_SIZE(in_com1));

static void test_zdsp_negate_q15(
	const q15_t *input1, const q15_t *ref, size_t length)
{
	q15_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(q15_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	zdsp_negate_q15(input1, output, length);

	/* Validate output */
	zassert_true(
		test_snr_error_q15(length, output, ref, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_near_equal_q15(length, output, ref, ABS_ERROR_THRESH_Q15),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT3(basic_math_q15, zdsp_negate_q15, 7, in_com1, ref_negate, 7);
DEFINE_TEST_VARIANT3(basic_math_q15, zdsp_negate_q15, 16, in_com1, ref_negate, 16);
DEFINE_TEST_VARIANT3(basic_math_q15, zdsp_negate_q15, 23, in_com1, ref_negate, 23);
DEFINE_TEST_VARIANT3(basic_math_q15, zdsp_negate_q15, possat, in_maxneg2, ref_negate_possat, 17);
DEFINE_TEST_VARIANT3(basic_math_q15, zdsp_negate_q15, long, in_com1, ref_negate,
		     ARRAY_SIZE(in_com1));

static void test_zdsp_offset_q15(
	const q15_t *input1, q15_t scalar, const q15_t *ref, size_t length)
{
	q15_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(q15_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	zdsp_offset_q15(input1, scalar, output, length);

	/* Validate output */
	zassert_true(
		test_snr_error_q15(length, output, ref, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_near_equal_q15(length, output, ref, ABS_ERROR_THRESH_Q15),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT4(basic_math_q15, zdsp_offset_q15, 0p5_7, in_com1, 0x4000, ref_offset, 7);
DEFINE_TEST_VARIANT4(basic_math_q15, zdsp_offset_q15, 0p5_16, in_com1, 0x4000, ref_offset, 16);
DEFINE_TEST_VARIANT4(basic_math_q15, zdsp_offset_q15, 0p5_23, in_com1, 0x4000, ref_offset, 23);
DEFINE_TEST_VARIANT4(basic_math_q15, zdsp_offset_q15, possat, in_maxpos, 0x7333, ref_offset_possat,
		     17);
DEFINE_TEST_VARIANT4(basic_math_q15, zdsp_offset_q15, negsat, in_maxneg, 0x8ccd, ref_offset_negsat,
		     17);
DEFINE_TEST_VARIANT4(basic_math_q15, zdsp_offset_q15, long, in_com1, 0x4000, ref_offset,
		     ARRAY_SIZE(in_com1));

static void test_zdsp_scale_q15(
	const q15_t *input1, q15_t scalar, const q15_t *ref, size_t length)
{
	q15_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(q15_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	zdsp_scale_q15(input1, scalar, 0, output, length);

	/* Validate output */
	zassert_true(
		test_snr_error_q15(length, output, ref, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_near_equal_q15(length, output, ref, ABS_ERROR_THRESH_Q15),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT4(basic_math_q15, zdsp_scale_q15, 0p5_7, in_com1, 0x4000, ref_scale, 7);
DEFINE_TEST_VARIANT4(basic_math_q15, zdsp_scale_q15, 0p5_16, in_com1, 0x4000, ref_scale, 16);
DEFINE_TEST_VARIANT4(basic_math_q15, zdsp_scale_q15, 0p5_23, in_com1, 0x4000, ref_scale, 23);
DEFINE_TEST_VARIANT4(basic_math_q15, zdsp_scale_q15, possat, in_maxneg2, 0x8000, ref_scale_possat,
		     17);
DEFINE_TEST_VARIANT4(basic_math_q15, zdsp_scale_q15, long, in_com1, 0x4000, ref_scale,
		     ARRAY_SIZE(in_com1));

static void test_zdsp_dot_prod_q15(
	const q15_t *input1, const q15_t *input2, const q63_t *ref,
	size_t length)
{
	q63_t *output;

	/* Allocate output buffer */
	output = malloc(1 * sizeof(q63_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	zdsp_dot_prod_q15(input1, input2, length, &output[0]);

	/* Validate output */
	zassert_true(
		test_snr_error_q63(1, output, ref, SNR_ERROR_THRESH_HIGH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_near_equal_q63(1, output, ref, ABS_ERROR_THRESH_Q63),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT4(basic_math_q15, zdsp_dot_prod_q15, 7, in_com1, in_com2, ref_dot_prod_3, 7);
DEFINE_TEST_VARIANT4(basic_math_q15, zdsp_dot_prod_q15, 16, in_com1, in_com2, ref_dot_prod_4, 16);
DEFINE_TEST_VARIANT4(basic_math_q15, zdsp_dot_prod_q15, 23, in_com1, in_com2, ref_dot_prod_4n1, 23);
DEFINE_TEST_VARIANT4(basic_math_q15, zdsp_dot_prod_q15, long, in_com1, in_com2, ref_dot_prod_long,
		     ARRAY_SIZE(in_com1));

static void test_zdsp_abs_q15(
	const q15_t *input1, const q15_t *ref, size_t length)
{
	q15_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(q15_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	zdsp_abs_q15(input1, output, length);

	/* Validate output */
	zassert_true(
		test_snr_error_q15(length, output, ref, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_near_equal_q15(length, output, ref, ABS_ERROR_THRESH_Q15),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT3(basic_math_q15, zdsp_abs_q15, 7, in_com1, ref_abs, 7);
DEFINE_TEST_VARIANT3(basic_math_q15, zdsp_abs_q15, 16, in_com1, ref_abs, 16);
DEFINE_TEST_VARIANT3(basic_math_q15, zdsp_abs_q15, 23, in_com1, ref_abs, 23);
DEFINE_TEST_VARIANT3(basic_math_q15, zdsp_abs_q15, long, in_com1, ref_abs, ARRAY_SIZE(in_com1));

static void test_zdsp_shift_q15(
	const q15_t *input1, const q15_t *ref, size_t length)
{
	q15_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(q15_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	zdsp_shift_q15(input1, 1, output, length);

	/* Validate output */
	zassert_true(
		test_snr_error_q15(length, output, ref, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_near_equal_q15(length, output, ref, ABS_ERROR_THRESH_Q15),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT3(basic_math_q15, zdsp_shift_q15, rand, in_rand, ref_shift, 17);
DEFINE_TEST_VARIANT3(basic_math_q15, zdsp_shift_q15, possat, in_maxpos, ref_shift_possat, 17);
DEFINE_TEST_VARIANT3(basic_math_q15, zdsp_shift_q15, negsat, in_maxneg, ref_shift_negsat, 17);

static void test_zdsp_and_u16(
	const uint16_t *input1, const uint16_t *input2, const uint16_t *ref,
	size_t length)
{
	uint16_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(uint16_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	zdsp_and_u16(input1, input2, output, length);

	/* Validate output */
	zassert_true(
		test_equal_q15(length, output, ref),
		ASSERT_MSG_INCORRECT_COMP_RESULT);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT4(basic_math_q15, zdsp_and_u16, 7, in_bitwise1, in_bitwise2, ref_and, 7);
DEFINE_TEST_VARIANT4(basic_math_q15, zdsp_and_u16, 16, in_bitwise1, in_bitwise2, ref_and, 16);
DEFINE_TEST_VARIANT4(basic_math_q15, zdsp_and_u16, 23, in_bitwise1, in_bitwise2, ref_and, 23);

static void test_zdsp_or_u16(
	const uint16_t *input1, const uint16_t *input2, const uint16_t *ref,
	size_t length)
{
	uint16_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(uint16_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	zdsp_or_u16(input1, input2, output, length);

	/* Validate output */
	zassert_true(
		test_equal_q15(length, output, ref),
		ASSERT_MSG_INCORRECT_COMP_RESULT);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT4(basic_math_q15, zdsp_or_u16, 7, in_bitwise1, in_bitwise2, ref_or, 7);
DEFINE_TEST_VARIANT4(basic_math_q15, zdsp_or_u16, 16, in_bitwise1, in_bitwise2, ref_or, 16);
DEFINE_TEST_VARIANT4(basic_math_q15, zdsp_or_u16, 23, in_bitwise1, in_bitwise2, ref_or, 23);

static void test_zdsp_not_u16(
	const uint16_t *input1, const uint16_t *ref, size_t length)
{
	uint16_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(uint16_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	zdsp_not_u16(input1, output, length);

	/* Validate output */
	zassert_true(
		test_equal_q15(length, output, ref),
		ASSERT_MSG_INCORRECT_COMP_RESULT);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT3(basic_math_q15, zdsp_not_u16, 7, in_bitwise1, ref_not, 7);
DEFINE_TEST_VARIANT3(basic_math_q15, zdsp_not_u16, 16, in_bitwise1, ref_not, 16);
DEFINE_TEST_VARIANT3(basic_math_q15, zdsp_not_u16, 23, in_bitwise1, ref_not, 23);

static void test_zdsp_xor_u16(
	const uint16_t *input1, const uint16_t *input2, const uint16_t *ref,
	size_t length)
{
	uint16_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(uint16_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	zdsp_xor_u16(input1, input2, output, length);

	/* Validate output */
	zassert_true(
		test_equal_q15(length, output, ref),
		ASSERT_MSG_INCORRECT_COMP_RESULT);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT4(basic_math_q15, zdsp_xor_u16, 7, in_bitwise1, in_bitwise2, ref_xor, 7);
DEFINE_TEST_VARIANT4(basic_math_q15, zdsp_xor_u16, 16, in_bitwise1, in_bitwise2, ref_xor, 16);
DEFINE_TEST_VARIANT4(basic_math_q15, zdsp_xor_u16, 23, in_bitwise1, in_bitwise2, ref_xor, 23);

static void test_zdsp_clip_q15(
	const q15_t *input, const q15_t *ref, q15_t min, q15_t max, size_t length)
{
	q15_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(q15_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	zdsp_clip_q15(input, output, min, max, length);

	/* Validate output */
	zassert_true(
		test_equal_q15(length, output, ref),
		ASSERT_MSG_INCORRECT_COMP_RESULT);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT5(basic_math_q15, zdsp_clip_q15, c000_f333, in_clip, ref_clip1,
		     0xc000, 0xf333, ARRAY_SIZE(ref_clip1));
DEFINE_TEST_VARIANT5(basic_math_q15, zdsp_clip_q15, c000_4000, in_clip, ref_clip2,
		     0xc000, 0x4000, ARRAY_SIZE(ref_clip2));
DEFINE_TEST_VARIANT5(basic_math_q15, zdsp_clip_q15, 0ccd_4000, in_clip, ref_clip3,
		     0x0ccd, 0x4000, ARRAY_SIZE(ref_clip3));

ZTEST_SUITE(basic_math_q15, NULL, NULL, NULL, NULL, NULL);
