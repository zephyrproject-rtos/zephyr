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

#include "misc_q15.pat"

#define SNR_ERROR_THRESH		((float32_t)70)
#define ABS_ERROR_THRESH_Q15		((q15_t)10)
#define ABS_ERROR_THRESH_FAST_Q15	((q15_t)20)

static void test_arm_correlate_q15(
	size_t in1_length, size_t in2_length, const q15_t *ref,
	size_t ref_length)
{
	q15_t *output;

	/* Allocate output buffer */
	output = calloc(ref_length, sizeof(q15_t));

	/* Run test function */
	arm_correlate_q15(in_com1, in1_length, in_com2, in2_length, output);

	/* Validate output */
	zassert_true(
		test_snr_error_q15(ref_length, ref, output, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_near_equal_q15(ref_length, ref, output,
			ABS_ERROR_THRESH_Q15),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

#define DEFINE_CORRELATE_TEST(a, b) \
	DEFINE_TEST_VARIANT4( \
		arm_correlate_q15, a##_##b, a, b, \
		ref_correlate_##a##_##b, ARRAY_SIZE(ref_correlate_##a##_##b))

DEFINE_CORRELATE_TEST(14, 15);
DEFINE_CORRELATE_TEST(14, 16);
DEFINE_CORRELATE_TEST(14, 17);
DEFINE_CORRELATE_TEST(14, 18);
DEFINE_CORRELATE_TEST(14, 33);
DEFINE_CORRELATE_TEST(15, 15);
DEFINE_CORRELATE_TEST(15, 16);
DEFINE_CORRELATE_TEST(15, 17);
DEFINE_CORRELATE_TEST(15, 18);
DEFINE_CORRELATE_TEST(15, 33);
DEFINE_CORRELATE_TEST(16, 15);
DEFINE_CORRELATE_TEST(16, 16);
DEFINE_CORRELATE_TEST(16, 17);
DEFINE_CORRELATE_TEST(16, 18);
DEFINE_CORRELATE_TEST(16, 33);
DEFINE_CORRELATE_TEST(17, 15);
DEFINE_CORRELATE_TEST(17, 16);
DEFINE_CORRELATE_TEST(17, 17);
DEFINE_CORRELATE_TEST(17, 18);
DEFINE_CORRELATE_TEST(17, 33);
DEFINE_CORRELATE_TEST(32, 15);
DEFINE_CORRELATE_TEST(32, 16);
DEFINE_CORRELATE_TEST(32, 17);
DEFINE_CORRELATE_TEST(32, 18);
DEFINE_CORRELATE_TEST(32, 33);

static void test_arm_conv_q15(
	size_t in1_length, size_t in2_length, const q15_t *ref,
	size_t ref_length)
{
	q15_t *output;

	/* Allocate output buffer */
	output = calloc(ref_length, sizeof(q15_t));

	/* Run test function */
	arm_conv_q15(in_com1, in1_length, in_com2, in2_length, output);

	/* Validate output */
	zassert_true(
		test_snr_error_q15(ref_length, ref, output, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_near_equal_q15(ref_length, ref, output,
			ABS_ERROR_THRESH_Q15),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

#define DEFINE_CONV_TEST(a, b) \
	DEFINE_TEST_VARIANT4( \
		arm_conv_q15, a##_##b, a, b, \
		ref_conv_##a##_##b, ARRAY_SIZE(ref_conv_##a##_##b))

DEFINE_CONV_TEST(14, 15);
DEFINE_CONV_TEST(14, 16);
DEFINE_CONV_TEST(14, 17);
DEFINE_CONV_TEST(14, 18);
DEFINE_CONV_TEST(14, 33);
DEFINE_CONV_TEST(15, 15);
DEFINE_CONV_TEST(15, 16);
DEFINE_CONV_TEST(15, 17);
DEFINE_CONV_TEST(15, 18);
DEFINE_CONV_TEST(15, 33);
DEFINE_CONV_TEST(16, 15);
DEFINE_CONV_TEST(16, 16);
DEFINE_CONV_TEST(16, 17);
DEFINE_CONV_TEST(16, 18);
DEFINE_CONV_TEST(16, 33);
DEFINE_CONV_TEST(17, 15);
DEFINE_CONV_TEST(17, 16);
DEFINE_CONV_TEST(17, 17);
DEFINE_CONV_TEST(17, 18);
DEFINE_CONV_TEST(17, 33);
DEFINE_CONV_TEST(32, 15);
DEFINE_CONV_TEST(32, 16);
DEFINE_CONV_TEST(32, 17);
DEFINE_CONV_TEST(32, 18);
DEFINE_CONV_TEST(32, 33);

#ifdef CONFIG_CMSIS_DSP_TEST_FILTERING_MISC_CONV_PARTIAL
static void test_arm_conv_partial_q15(
	size_t first, size_t in1_length, size_t in2_length, const q15_t *ref,
	size_t ref_length)
{
	q15_t *output;
	q15_t *temp;
	arm_status status;

	/* Allocate output buffer */
	output = calloc(first + ref_length, sizeof(q15_t));
	temp = calloc(ref_length, sizeof(q15_t));

	/* Run test function */
	status = arm_conv_partial_q15(
			in_partial1, in1_length, in_partial2, in2_length,
			output, first, ref_length);

	zassert_equal(status, ARM_MATH_SUCCESS,
		      ASSERT_MSG_INCORRECT_COMP_RESULT);

	memcpy(temp, &output[first], ref_length * sizeof(q15_t));

	/* Validate output */
	zassert_true(
		test_snr_error_q15(ref_length, ref, temp, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_near_equal_q15(ref_length, ref, temp,
			ABS_ERROR_THRESH_Q15),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
	free(temp);
}

static void test_arm_conv_partial_fast_q15(
	size_t first, size_t in1_length, size_t in2_length, const q15_t *ref,
	size_t ref_length)
{
	q15_t *output;
	q15_t *temp;
	arm_status status;

	/* Allocate output buffer */
	output = calloc(first + ref_length, sizeof(q15_t));
	temp = calloc(ref_length, sizeof(q15_t));

	/* Run test function */
	status = arm_conv_partial_fast_q15(
			in_partial1, in1_length, in_partial2, in2_length,
			output, first, ref_length);

	zassert_equal(status, ARM_MATH_SUCCESS,
		      ASSERT_MSG_INCORRECT_COMP_RESULT);

	memcpy(temp, &output[first], ref_length * sizeof(q15_t));

	/* Validate output */
	zassert_true(
		test_snr_error_q15(ref_length, ref, temp, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_near_equal_q15(ref_length, ref, temp,
			ABS_ERROR_THRESH_FAST_Q15),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
	free(temp);
}

static void test_arm_conv_partial_opt_q15(
	size_t first, size_t in1_length, size_t in2_length, const q15_t *ref,
	size_t ref_length)
{
	q15_t *output;
	q15_t *temp;
	q15_t *scratch1, *scratch2;
	arm_status status;

	/* Allocate output buffer */
	output = calloc(first + ref_length, sizeof(q15_t));
	temp = calloc(ref_length, sizeof(q15_t));
	scratch1 = calloc(24, sizeof(q15_t));
	scratch2 = calloc(24, sizeof(q15_t));

	/* Run test function */
	status = arm_conv_partial_opt_q15(
			in_partial1, in1_length, in_partial2, in2_length,
			output, first, ref_length,
			scratch1, scratch2);

	zassert_equal(status, ARM_MATH_SUCCESS,
		      ASSERT_MSG_INCORRECT_COMP_RESULT);

	memcpy(temp, &output[first], ref_length * sizeof(q15_t));

	/* Validate output */
	zassert_true(
		test_snr_error_q15(ref_length, ref, temp, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_near_equal_q15(ref_length, ref, temp,
			ABS_ERROR_THRESH_FAST_Q15),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
	free(temp);
	free(scratch1);
	free(scratch2);
}

static void test_arm_conv_partial_fast_opt_q15(
	size_t first, size_t in1_length, size_t in2_length, const q15_t *ref,
	size_t ref_length)
{
	q15_t *output;
	q15_t *temp;
	q15_t *scratch1, *scratch2;
	arm_status status;

	/* Allocate output buffer */
	output = calloc(first + ref_length, sizeof(q15_t));
	temp = calloc(ref_length, sizeof(q15_t));
	scratch1 = calloc(24, sizeof(q15_t));
	scratch2 = calloc(24, sizeof(q15_t));

	/* Run test function */
	status = arm_conv_partial_fast_opt_q15(
			in_partial1, in1_length, in_partial2, in2_length,
			output, first, ref_length,
			scratch1, scratch2);

	zassert_equal(status, ARM_MATH_SUCCESS,
		      ASSERT_MSG_INCORRECT_COMP_RESULT);

	memcpy(temp, &output[first], ref_length * sizeof(q15_t));

	/* Validate output */
	zassert_true(
		test_snr_error_q15(ref_length, ref, temp, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_near_equal_q15(ref_length, ref, temp,
			ABS_ERROR_THRESH_FAST_Q15),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
	free(temp);
	free(scratch1);
	free(scratch2);
}
#else
static void test_arm_conv_partial_q15(
	size_t first, size_t in1_length, size_t in2_length, const q15_t *ref,
	size_t ref_length)
{
	ztest_test_skip();
}

static void test_arm_conv_partial_fast_q15(
	size_t first, size_t in1_length, size_t in2_length, const q15_t *ref,
	size_t ref_length)
{
	ztest_test_skip();
}

static void test_arm_conv_partial_opt_q15(
	size_t first, size_t in1_length, size_t in2_length, const q15_t *ref,
	size_t ref_length)
{
	ztest_test_skip();
}

static void test_arm_conv_partial_fast_opt_q15(
	size_t first, size_t in1_length, size_t in2_length, const q15_t *ref,
	size_t ref_length)
{
	ztest_test_skip();
}
#endif /* CONFIG_CMSIS_DSP_TEST_FILTERING_MISC_CONV_PARTIAL */

#define DEFINE_CONV_PARTIAL_TEST(a, b, c) \
	DEFINE_TEST_VARIANT5( \
		arm_conv_partial_q15, a##_##b##_##c, a, b, c, \
		ref_conv_partial_##a##_##b##_##c, \
		ARRAY_SIZE(ref_conv_partial_##a##_##b##_##c)) \
	DEFINE_TEST_VARIANT5( \
		arm_conv_partial_fast_q15, a##_##b##_##c, a, b, c, \
		ref_conv_partial_##a##_##b##_##c, \
		ARRAY_SIZE(ref_conv_partial_##a##_##b##_##c)) \
	DEFINE_TEST_VARIANT5( \
		arm_conv_partial_opt_q15, a##_##b##_##c, a, b, c, \
		ref_conv_partial_##a##_##b##_##c, \
		ARRAY_SIZE(ref_conv_partial_##a##_##b##_##c)) \
	DEFINE_TEST_VARIANT5( \
		arm_conv_partial_fast_opt_q15, a##_##b##_##c, a, b, c, \
		ref_conv_partial_##a##_##b##_##c, \
		ARRAY_SIZE(ref_conv_partial_##a##_##b##_##c))

DEFINE_CONV_PARTIAL_TEST(3, 6, 8);
DEFINE_CONV_PARTIAL_TEST(9, 6, 8);
DEFINE_CONV_PARTIAL_TEST(7, 6, 8);

void test_filtering_misc_q15(void)
{
	ztest_test_suite(filtering_misc_q15,
		ztest_unit_test(test_arm_correlate_q15_14_15),
		ztest_unit_test(test_arm_correlate_q15_14_16),
		ztest_unit_test(test_arm_correlate_q15_14_17),
		ztest_unit_test(test_arm_correlate_q15_14_18),
		ztest_unit_test(test_arm_correlate_q15_14_33),
		ztest_unit_test(test_arm_correlate_q15_15_15),
		ztest_unit_test(test_arm_correlate_q15_15_16),
		ztest_unit_test(test_arm_correlate_q15_15_17),
		ztest_unit_test(test_arm_correlate_q15_15_18),
		ztest_unit_test(test_arm_correlate_q15_15_33),
		ztest_unit_test(test_arm_correlate_q15_16_15),
		ztest_unit_test(test_arm_correlate_q15_16_16),
		ztest_unit_test(test_arm_correlate_q15_16_17),
		ztest_unit_test(test_arm_correlate_q15_16_18),
		ztest_unit_test(test_arm_correlate_q15_16_33),
		ztest_unit_test(test_arm_correlate_q15_17_15),
		ztest_unit_test(test_arm_correlate_q15_17_16),
		ztest_unit_test(test_arm_correlate_q15_17_17),
		ztest_unit_test(test_arm_correlate_q15_17_18),
		ztest_unit_test(test_arm_correlate_q15_17_33),
		ztest_unit_test(test_arm_correlate_q15_32_15),
		ztest_unit_test(test_arm_correlate_q15_32_16),
		ztest_unit_test(test_arm_correlate_q15_32_17),
		ztest_unit_test(test_arm_correlate_q15_32_18),
		ztest_unit_test(test_arm_correlate_q15_32_33),
		ztest_unit_test(test_arm_conv_q15_14_15),
		ztest_unit_test(test_arm_conv_q15_14_16),
		ztest_unit_test(test_arm_conv_q15_14_17),
		ztest_unit_test(test_arm_conv_q15_14_18),
		ztest_unit_test(test_arm_conv_q15_14_33),
		ztest_unit_test(test_arm_conv_q15_15_15),
		ztest_unit_test(test_arm_conv_q15_15_16),
		ztest_unit_test(test_arm_conv_q15_15_17),
		ztest_unit_test(test_arm_conv_q15_15_18),
		ztest_unit_test(test_arm_conv_q15_15_33),
		ztest_unit_test(test_arm_conv_q15_16_15),
		ztest_unit_test(test_arm_conv_q15_16_16),
		ztest_unit_test(test_arm_conv_q15_16_17),
		ztest_unit_test(test_arm_conv_q15_16_18),
		ztest_unit_test(test_arm_conv_q15_16_33),
		ztest_unit_test(test_arm_conv_q15_17_15),
		ztest_unit_test(test_arm_conv_q15_17_16),
		ztest_unit_test(test_arm_conv_q15_17_17),
		ztest_unit_test(test_arm_conv_q15_17_18),
		ztest_unit_test(test_arm_conv_q15_17_33),
		ztest_unit_test(test_arm_conv_q15_32_15),
		ztest_unit_test(test_arm_conv_q15_32_16),
		ztest_unit_test(test_arm_conv_q15_32_17),
		ztest_unit_test(test_arm_conv_q15_32_18),
		ztest_unit_test(test_arm_conv_q15_32_33),
		ztest_unit_test(test_arm_conv_partial_q15_3_6_8),
		ztest_unit_test(test_arm_conv_partial_q15_9_6_8),
		ztest_unit_test(test_arm_conv_partial_q15_7_6_8),
		ztest_unit_test(test_arm_conv_partial_fast_q15_3_6_8),
		ztest_unit_test(test_arm_conv_partial_fast_q15_9_6_8),
		ztest_unit_test(test_arm_conv_partial_fast_q15_7_6_8),
		ztest_unit_test(test_arm_conv_partial_opt_q15_3_6_8),
		ztest_unit_test(test_arm_conv_partial_opt_q15_9_6_8),
		ztest_unit_test(test_arm_conv_partial_opt_q15_7_6_8),
		ztest_unit_test(test_arm_conv_partial_fast_opt_q15_3_6_8),
		ztest_unit_test(test_arm_conv_partial_fast_opt_q15_9_6_8),
		ztest_unit_test(test_arm_conv_partial_fast_opt_q15_7_6_8)
		);

	ztest_run_test_suite(filtering_misc_q15);
}
