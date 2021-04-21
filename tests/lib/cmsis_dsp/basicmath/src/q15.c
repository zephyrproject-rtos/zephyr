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

#define SNR_ERROR_THRESH	((float32_t)70)
#define SNR_ERROR_THRESH_HIGH	((float32_t)60)
#define ABS_ERROR_THRESH_Q15	((q15_t)2)
#define ABS_ERROR_THRESH_Q63	((q63_t)(1 << 17))

static void test_arm_add_q15(
	const q15_t *input1, const q15_t *input2, const q15_t *ref,
	size_t length)
{
	q15_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(q15_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_add_q15(input1, input2, output, length);

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

DEFINE_TEST_VARIANT4(arm_add_q15, 7, in_com1, in_com2, ref_add, 7);
DEFINE_TEST_VARIANT4(arm_add_q15, 16, in_com1, in_com2, ref_add, 16);
DEFINE_TEST_VARIANT4(arm_add_q15, 23, in_com1, in_com2, ref_add, 23);
DEFINE_TEST_VARIANT4(arm_add_q15, possat, in_maxpos, in_maxpos, ref_add_possat, 17);
DEFINE_TEST_VARIANT4(arm_add_q15, negsat, in_maxneg, in_maxneg, ref_add_negsat, 17);

static void test_arm_sub_q15(
	const q15_t *input1, const q15_t *input2, const q15_t *ref,
	size_t length)
{
	q15_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(q15_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_sub_q15(input1, input2, output, length);

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

DEFINE_TEST_VARIANT4(arm_sub_q15, 7, in_com1, in_com2, ref_sub, 7);
DEFINE_TEST_VARIANT4(arm_sub_q15, 16, in_com1, in_com2, ref_sub, 16);
DEFINE_TEST_VARIANT4(arm_sub_q15, 23, in_com1, in_com2, ref_sub, 23);
DEFINE_TEST_VARIANT4(arm_sub_q15, possat, in_maxpos, in_maxneg, ref_sub_possat, 17);
DEFINE_TEST_VARIANT4(arm_sub_q15, negsat, in_maxneg, in_maxpos, ref_sub_negsat, 17);

static void test_arm_mult_q15(
	const q15_t *input1, const q15_t *input2, const q15_t *ref,
	size_t length)
{
	q15_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(q15_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_mult_q15(input1, input2, output, length);

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

DEFINE_TEST_VARIANT4(arm_mult_q15, 7, in_com1, in_com2, ref_mult, 7);
DEFINE_TEST_VARIANT4(arm_mult_q15, 16, in_com1, in_com2, ref_mult, 16);
DEFINE_TEST_VARIANT4(arm_mult_q15, 23, in_com1, in_com2, ref_mult, 23);
DEFINE_TEST_VARIANT4(arm_mult_q15, possat, in_maxneg2, in_maxneg2, ref_mult_possat, 17);

static void test_arm_negate_q15(
	const q15_t *input1, const q15_t *ref, size_t length)
{
	q15_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(q15_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_negate_q15(input1, output, length);

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

DEFINE_TEST_VARIANT3(arm_negate_q15, 7, in_com1, ref_negate, 7);
DEFINE_TEST_VARIANT3(arm_negate_q15, 16, in_com1, ref_negate, 16);
DEFINE_TEST_VARIANT3(arm_negate_q15, 23, in_com1, ref_negate, 23);
DEFINE_TEST_VARIANT3(arm_negate_q15, possat, in_maxneg2, ref_negate_possat, 17);

static void test_arm_offset_q15(
	const q15_t *input1, q15_t scalar, const q15_t *ref, size_t length)
{
	q15_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(q15_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_offset_q15(input1, scalar, output, length);

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

DEFINE_TEST_VARIANT4(arm_offset_q15, 0p5_7, in_com1, 0x4000, ref_offset, 7);
DEFINE_TEST_VARIANT4(arm_offset_q15, 0p5_16, in_com1, 0x4000, ref_offset, 16);
DEFINE_TEST_VARIANT4(arm_offset_q15, 0p5_23, in_com1, 0x4000, ref_offset, 23);
DEFINE_TEST_VARIANT4(arm_offset_q15, possat, in_maxpos, 0x7333, ref_offset_possat, 17);
DEFINE_TEST_VARIANT4(arm_offset_q15, negsat, in_maxneg, 0x8ccd, ref_offset_negsat, 17);

static void test_arm_scale_q15(
	const q15_t *input1, q15_t scalar, const q15_t *ref, size_t length)
{
	q15_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(q15_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_scale_q15(input1, scalar, 0, output, length);

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

DEFINE_TEST_VARIANT4(arm_scale_q15, 0p5_7, in_com1, 0x4000, ref_scale, 7);
DEFINE_TEST_VARIANT4(arm_scale_q15, 0p5_16, in_com1, 0x4000, ref_scale, 16);
DEFINE_TEST_VARIANT4(arm_scale_q15, 0p5_23, in_com1, 0x4000, ref_scale, 23);
DEFINE_TEST_VARIANT4(arm_scale_q15, possat, in_maxneg2, 0x8000, ref_scale_possat, 17);

static void test_arm_dot_prod_q15(
	const q15_t *input1, const q15_t *input2, const q63_t *ref,
	size_t length)
{
	q63_t *output;

	/* Allocate output buffer */
	output = malloc(1 * sizeof(q63_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_dot_prod_q15(input1, input2, length, &output[0]);

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

DEFINE_TEST_VARIANT4(arm_dot_prod_q15, 7, in_com1, in_com2, ref_dot_prod_3, 7);
DEFINE_TEST_VARIANT4(arm_dot_prod_q15, 16, in_com1, in_com2, ref_dot_prod_4, 16);
DEFINE_TEST_VARIANT4(arm_dot_prod_q15, 23, in_com1, in_com2, ref_dot_prod_4n1, 23);

static void test_arm_abs_q15(
	const q15_t *input1, const q15_t *ref, size_t length)
{
	q15_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(q15_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_abs_q15(input1, output, length);

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

DEFINE_TEST_VARIANT3(arm_abs_q15, 7, in_com1, ref_abs, 7);
DEFINE_TEST_VARIANT3(arm_abs_q15, 16, in_com1, ref_abs, 16);
DEFINE_TEST_VARIANT3(arm_abs_q15, 23, in_com1, ref_abs, 23);

static void test_arm_shift_q15(
	const q15_t *input1, const q15_t *ref, size_t length)
{
	q15_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(q15_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_shift_q15(input1, 1, output, length);

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

DEFINE_TEST_VARIANT3(arm_shift_q15, rand, in_rand, ref_shift, 17);
DEFINE_TEST_VARIANT3(arm_shift_q15, possat, in_maxpos, ref_shift_possat, 17);
DEFINE_TEST_VARIANT3(arm_shift_q15, negsat, in_maxneg, ref_shift_negsat, 17);

static void test_arm_and_u16(
	const uint16_t *input1, const uint16_t *input2, const uint16_t *ref,
	size_t length)
{
	uint16_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(uint16_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_and_u16(input1, input2, output, length);

	/* Validate output */
	zassert_true(
		test_equal_q15(length, output, ref),
		ASSERT_MSG_INCORRECT_COMP_RESULT);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT4(arm_and_u16, 7, in_bitwise1, in_bitwise2, ref_and, 7);
DEFINE_TEST_VARIANT4(arm_and_u16, 16, in_bitwise1, in_bitwise2, ref_and, 16);
DEFINE_TEST_VARIANT4(arm_and_u16, 23, in_bitwise1, in_bitwise2, ref_and, 23);

static void test_arm_or_u16(
	const uint16_t *input1, const uint16_t *input2, const uint16_t *ref,
	size_t length)
{
	uint16_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(uint16_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_or_u16(input1, input2, output, length);

	/* Validate output */
	zassert_true(
		test_equal_q15(length, output, ref),
		ASSERT_MSG_INCORRECT_COMP_RESULT);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT4(arm_or_u16, 7, in_bitwise1, in_bitwise2, ref_or, 7);
DEFINE_TEST_VARIANT4(arm_or_u16, 16, in_bitwise1, in_bitwise2, ref_or, 16);
DEFINE_TEST_VARIANT4(arm_or_u16, 23, in_bitwise1, in_bitwise2, ref_or, 23);

static void test_arm_not_u16(
	const uint16_t *input1, const uint16_t *ref, size_t length)
{
	uint16_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(uint16_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_not_u16(input1, output, length);

	/* Validate output */
	zassert_true(
		test_equal_q15(length, output, ref),
		ASSERT_MSG_INCORRECT_COMP_RESULT);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT3(arm_not_u16, 7, in_bitwise1, ref_not, 7);
DEFINE_TEST_VARIANT3(arm_not_u16, 16, in_bitwise1, ref_not, 16);
DEFINE_TEST_VARIANT3(arm_not_u16, 23, in_bitwise1, ref_not, 23);

static void test_arm_xor_u16(
	const uint16_t *input1, const uint16_t *input2, const uint16_t *ref,
	size_t length)
{
	uint16_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(uint16_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_xor_u16(input1, input2, output, length);

	/* Validate output */
	zassert_true(
		test_equal_q15(length, output, ref),
		ASSERT_MSG_INCORRECT_COMP_RESULT);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT4(arm_xor_u16, 7, in_bitwise1, in_bitwise2, ref_xor, 7);
DEFINE_TEST_VARIANT4(arm_xor_u16, 16, in_bitwise1, in_bitwise2, ref_xor, 16);
DEFINE_TEST_VARIANT4(arm_xor_u16, 23, in_bitwise1, in_bitwise2, ref_xor, 23);

void test_basicmath_q15(void)
{
	ztest_test_suite(basicmath_q15,
		ztest_unit_test(test_arm_add_q15_7),
		ztest_unit_test(test_arm_add_q15_16),
		ztest_unit_test(test_arm_add_q15_23),
		ztest_unit_test(test_arm_sub_q15_7),
		ztest_unit_test(test_arm_sub_q15_16),
		ztest_unit_test(test_arm_sub_q15_23),
		ztest_unit_test(test_arm_mult_q15_7),
		ztest_unit_test(test_arm_mult_q15_16),
		ztest_unit_test(test_arm_mult_q15_23),
		ztest_unit_test(test_arm_negate_q15_7),
		ztest_unit_test(test_arm_negate_q15_16),
		ztest_unit_test(test_arm_negate_q15_23),
		ztest_unit_test(test_arm_offset_q15_0p5_7),
		ztest_unit_test(test_arm_offset_q15_0p5_16),
		ztest_unit_test(test_arm_offset_q15_0p5_23),
		ztest_unit_test(test_arm_scale_q15_0p5_7),
		ztest_unit_test(test_arm_scale_q15_0p5_16),
		ztest_unit_test(test_arm_scale_q15_0p5_23),
		ztest_unit_test(test_arm_dot_prod_q15_7),
		ztest_unit_test(test_arm_dot_prod_q15_16),
		ztest_unit_test(test_arm_dot_prod_q15_23),
		ztest_unit_test(test_arm_abs_q15_7),
		ztest_unit_test(test_arm_abs_q15_16),
		ztest_unit_test(test_arm_abs_q15_23),
		ztest_unit_test(test_arm_shift_q15_rand),
		ztest_unit_test(test_arm_add_q15_possat),
		ztest_unit_test(test_arm_add_q15_negsat),
		ztest_unit_test(test_arm_sub_q15_possat),
		ztest_unit_test(test_arm_sub_q15_negsat),
		ztest_unit_test(test_arm_mult_q15_possat),
		ztest_unit_test(test_arm_negate_q15_possat),
		ztest_unit_test(test_arm_offset_q15_possat),
		ztest_unit_test(test_arm_offset_q15_negsat),
		ztest_unit_test(test_arm_scale_q15_possat),
		ztest_unit_test(test_arm_shift_q15_possat),
		ztest_unit_test(test_arm_shift_q15_negsat),
		ztest_unit_test(test_arm_and_u16_7),
		ztest_unit_test(test_arm_and_u16_16),
		ztest_unit_test(test_arm_and_u16_23),
		ztest_unit_test(test_arm_or_u16_7),
		ztest_unit_test(test_arm_or_u16_16),
		ztest_unit_test(test_arm_or_u16_23),
		ztest_unit_test(test_arm_not_u16_7),
		ztest_unit_test(test_arm_not_u16_16),
		ztest_unit_test(test_arm_not_u16_23),
		ztest_unit_test(test_arm_xor_u16_7),
		ztest_unit_test(test_arm_xor_u16_16),
		ztest_unit_test(test_arm_xor_u16_23)
		);

	ztest_run_test_suite(basicmath_q15);
}
