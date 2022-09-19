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

#include "misc_q7.pat"

#define SNR_ERROR_THRESH	((float32_t)15)
#define ABS_ERROR_THRESH_Q7	((q7_t)5)

static void test_arm_correlate_q7(
	size_t in1_length, size_t in2_length, const q7_t *ref,
	size_t ref_length)
{
	q7_t *output;

	/* Allocate output buffer */
	output = calloc(ref_length, sizeof(q7_t));

	/* Run test function */
	arm_correlate_q7(in_com1, in1_length, in_com2, in2_length, output);

	/* Validate output */
	zassert_true(
		test_snr_error_q7(ref_length, ref, output, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_near_equal_q7(ref_length, ref, output,
			ABS_ERROR_THRESH_Q7),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

#define DEFINE_CORRELATE_TEST(a, b) \
	DEFINE_TEST_VARIANT4(filtering_misc_q7, \
		arm_correlate_q7, a##_##b, a, b, \
		ref_correlate_##a##_##b, ARRAY_SIZE(ref_correlate_##a##_##b))

DEFINE_CORRELATE_TEST(30, 31);
DEFINE_CORRELATE_TEST(30, 32);
DEFINE_CORRELATE_TEST(30, 33);
DEFINE_CORRELATE_TEST(30, 34);
DEFINE_CORRELATE_TEST(30, 49);
DEFINE_CORRELATE_TEST(31, 31);
DEFINE_CORRELATE_TEST(31, 32);
DEFINE_CORRELATE_TEST(31, 33);
DEFINE_CORRELATE_TEST(31, 34);
DEFINE_CORRELATE_TEST(31, 49);
DEFINE_CORRELATE_TEST(32, 31);
DEFINE_CORRELATE_TEST(32, 32);
DEFINE_CORRELATE_TEST(32, 33);
DEFINE_CORRELATE_TEST(32, 34);
DEFINE_CORRELATE_TEST(32, 49);
DEFINE_CORRELATE_TEST(33, 31);
DEFINE_CORRELATE_TEST(33, 32);
DEFINE_CORRELATE_TEST(33, 33);
DEFINE_CORRELATE_TEST(33, 34);
DEFINE_CORRELATE_TEST(33, 49);
DEFINE_CORRELATE_TEST(48, 31);
DEFINE_CORRELATE_TEST(48, 32);
DEFINE_CORRELATE_TEST(48, 33);
DEFINE_CORRELATE_TEST(48, 34);
DEFINE_CORRELATE_TEST(48, 49);

static void test_arm_conv_q7(
	size_t in1_length, size_t in2_length, const q7_t *ref,
	size_t ref_length)
{
	q7_t *output;

	/* Allocate output buffer */
	output = calloc(ref_length, sizeof(q7_t));

	/* Run test function */
	arm_conv_q7(in_com1, in1_length, in_com2, in2_length, output);

	/* Validate output */
	zassert_true(
		test_snr_error_q7(ref_length, ref, output, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_near_equal_q7(ref_length, ref, output,
			ABS_ERROR_THRESH_Q7),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

#define DEFINE_CONV_TEST(a, b) \
	DEFINE_TEST_VARIANT4(filtering_misc_q7, \
		arm_conv_q7, a##_##b, a, b, \
		ref_conv_##a##_##b, ARRAY_SIZE(ref_conv_##a##_##b))

DEFINE_CONV_TEST(30, 31);
DEFINE_CONV_TEST(30, 32);
DEFINE_CONV_TEST(30, 33);
DEFINE_CONV_TEST(30, 34);
DEFINE_CONV_TEST(30, 49);
DEFINE_CONV_TEST(31, 31);
DEFINE_CONV_TEST(31, 32);
DEFINE_CONV_TEST(31, 33);
DEFINE_CONV_TEST(31, 34);
DEFINE_CONV_TEST(31, 49);
DEFINE_CONV_TEST(32, 31);
DEFINE_CONV_TEST(32, 32);
DEFINE_CONV_TEST(32, 33);
DEFINE_CONV_TEST(32, 34);
DEFINE_CONV_TEST(32, 49);
DEFINE_CONV_TEST(33, 31);
DEFINE_CONV_TEST(33, 32);
DEFINE_CONV_TEST(33, 33);
DEFINE_CONV_TEST(33, 34);
DEFINE_CONV_TEST(33, 49);
DEFINE_CONV_TEST(48, 31);
DEFINE_CONV_TEST(48, 32);
DEFINE_CONV_TEST(48, 33);
DEFINE_CONV_TEST(48, 34);
DEFINE_CONV_TEST(48, 49);

#ifdef CONFIG_CMSIS_DSP_TEST_FILTERING_MISC_CONV_PARTIAL
static void test_arm_conv_partial_q7(
	size_t first, size_t in1_length, size_t in2_length, const q7_t *ref,
	size_t ref_length)
{
	q7_t *output;
	q7_t *temp;
	arm_status status;

	/* Allocate output buffer */
	output = calloc(first + ref_length, sizeof(q7_t));
	temp = calloc(ref_length, sizeof(q7_t));

	/* Run test function */
	status = arm_conv_partial_q7(
			in_partial1, in1_length, in_partial2, in2_length,
			output, first, ref_length);

	zassert_equal(status, ARM_MATH_SUCCESS,
		      ASSERT_MSG_INCORRECT_COMP_RESULT);

	memcpy(temp, &output[first], ref_length * sizeof(q7_t));

	/* Validate output */
	zassert_true(
		test_snr_error_q7(ref_length, ref, temp, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_near_equal_q7(ref_length, ref, temp,
			ABS_ERROR_THRESH_Q7),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
	free(temp);
}

static void test_arm_conv_partial_opt_q7(
	size_t first, size_t in1_length, size_t in2_length, const q7_t *ref,
	size_t ref_length)
{
	q7_t *output;
	q7_t *temp;
	q15_t *scratch1, *scratch2;
	arm_status status;

	/* Allocate output buffer */
	output = calloc(first + ref_length, sizeof(q7_t));
	temp = calloc(ref_length, sizeof(q7_t));
	scratch1 = calloc(24, sizeof(q15_t));
	scratch2 = calloc(24, sizeof(q15_t));

	/* Run test function */
	status = arm_conv_partial_opt_q7(
			in_partial1, in1_length, in_partial2, in2_length,
			output, first, ref_length,
			scratch1, scratch2);

	zassert_equal(status, ARM_MATH_SUCCESS,
		      ASSERT_MSG_INCORRECT_COMP_RESULT);

	memcpy(temp, &output[first], ref_length * sizeof(q7_t));

	/* Validate output */
	zassert_true(
		test_snr_error_q7(ref_length, ref, temp, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_near_equal_q7(ref_length, ref, temp,
			ABS_ERROR_THRESH_Q7),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
	free(temp);
	free(scratch1);
	free(scratch2);
}
#else
static void test_arm_conv_partial_q7(
	size_t first, size_t in1_length, size_t in2_length, const q7_t *ref,
	size_t ref_length)
{
	ztest_test_skip();
}

static void test_arm_conv_partial_opt_q7(
	size_t first, size_t in1_length, size_t in2_length, const q7_t *ref,
	size_t ref_length)
{
	ztest_test_skip();
}
#endif /* CONFIG_CMSIS_DSP_TEST_FILTERING_MISC_CONV_PARTIAL */

#define DEFINE_CONV_PARTIAL_TEST(a, b, c) \
	DEFINE_TEST_VARIANT5(filtering_misc_q7, \
		arm_conv_partial_q7, a##_##b##_##c, a, b, c, \
		ref_conv_partial_##a##_##b##_##c, \
		ARRAY_SIZE(ref_conv_partial_##a##_##b##_##c)) \
	DEFINE_TEST_VARIANT5(filtering_misc_q7, \
		arm_conv_partial_opt_q7, a##_##b##_##c, a, b, c, \
		ref_conv_partial_##a##_##b##_##c, \
		ARRAY_SIZE(ref_conv_partial_##a##_##b##_##c))

DEFINE_CONV_PARTIAL_TEST(3, 6, 8);
DEFINE_CONV_PARTIAL_TEST(9, 6, 8);
DEFINE_CONV_PARTIAL_TEST(7, 6, 8);

ZTEST_SUITE(filtering_misc_q7, NULL, NULL, NULL, NULL, NULL);
