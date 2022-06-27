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

#include "misc_f32.pat"

#define SNR_ERROR_THRESH	((float32_t)120)
#define REL_ERROR_THRESH	(1.0e-6)
#define ABS_ERROR_THRESH	(1.0e-5)
#define REL_ERROR_THRESH_LD	(1.0e-6)
#define ABS_ERROR_THRESH_LD	(1.0e-6)

static void test_arm_correlate_f32(
	size_t in1_length, size_t in2_length, const uint32_t *ref,
	size_t ref_length)
{
	float32_t *output;

	/* Allocate output buffer */
	output = calloc(ref_length, sizeof(float32_t));

	/* Run test function */
	arm_correlate_f32(
		(float32_t *)in_com1, in1_length,
		(float32_t *)in_com2, in2_length, output);

	/* Validate output */
	zassert_true(
		test_snr_error_f32(ref_length, (float32_t *)ref, output,
			SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_close_error_f32(ref_length, (float32_t *)ref, output,
			ABS_ERROR_THRESH, REL_ERROR_THRESH),
		ASSERT_MSG_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

#define DEFINE_CORRELATE_TEST(a, b) \
	DEFINE_TEST_VARIANT4( \
		arm_correlate_f32, a##_##b, a, b, \
		ref_correlate_##a##_##b, ARRAY_SIZE(ref_correlate_##a##_##b))

DEFINE_CORRELATE_TEST(4, 1);
DEFINE_CORRELATE_TEST(4, 2);
DEFINE_CORRELATE_TEST(4, 3);
DEFINE_CORRELATE_TEST(4, 8);
DEFINE_CORRELATE_TEST(4, 11);
DEFINE_CORRELATE_TEST(5, 1);
DEFINE_CORRELATE_TEST(5, 2);
DEFINE_CORRELATE_TEST(5, 3);
DEFINE_CORRELATE_TEST(5, 8);
DEFINE_CORRELATE_TEST(5, 11);
DEFINE_CORRELATE_TEST(6, 1);
DEFINE_CORRELATE_TEST(6, 2);
DEFINE_CORRELATE_TEST(6, 3);
DEFINE_CORRELATE_TEST(6, 8);
DEFINE_CORRELATE_TEST(6, 11);
DEFINE_CORRELATE_TEST(9, 1);
DEFINE_CORRELATE_TEST(9, 2);
DEFINE_CORRELATE_TEST(9, 3);
DEFINE_CORRELATE_TEST(9, 8);
DEFINE_CORRELATE_TEST(9, 11);
DEFINE_CORRELATE_TEST(10, 1);
DEFINE_CORRELATE_TEST(10, 2);
DEFINE_CORRELATE_TEST(10, 3);
DEFINE_CORRELATE_TEST(10, 8);
DEFINE_CORRELATE_TEST(10, 11);
DEFINE_CORRELATE_TEST(11, 1);
DEFINE_CORRELATE_TEST(11, 2);
DEFINE_CORRELATE_TEST(11, 3);
DEFINE_CORRELATE_TEST(11, 8);
DEFINE_CORRELATE_TEST(11, 11);
DEFINE_CORRELATE_TEST(12, 1);
DEFINE_CORRELATE_TEST(12, 2);
DEFINE_CORRELATE_TEST(12, 3);
DEFINE_CORRELATE_TEST(12, 8);
DEFINE_CORRELATE_TEST(12, 11);
DEFINE_CORRELATE_TEST(13, 1);
DEFINE_CORRELATE_TEST(13, 2);
DEFINE_CORRELATE_TEST(13, 3);
DEFINE_CORRELATE_TEST(13, 8);
DEFINE_CORRELATE_TEST(13, 11);

static void test_arm_conv_f32(
	size_t in1_length, size_t in2_length, const uint32_t *ref,
	size_t ref_length)
{
	float32_t *output;

	/* Allocate output buffer */
	output = calloc(ref_length, sizeof(float32_t));

	/* Run test function */
	arm_conv_f32(
		(float32_t *)in_com1, in1_length,
		(float32_t *)in_com2, in2_length, output);

	/* Validate output */
	zassert_true(
		test_snr_error_f32(ref_length, (float32_t *)ref, output,
			SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_close_error_f32(ref_length, (float32_t *)ref, output,
			ABS_ERROR_THRESH, REL_ERROR_THRESH),
		ASSERT_MSG_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

#define DEFINE_CONV_TEST(a, b) \
	DEFINE_TEST_VARIANT4( \
		arm_conv_f32, a##_##b, a, b, \
		ref_conv_##a##_##b, ARRAY_SIZE(ref_conv_##a##_##b))

DEFINE_CONV_TEST(4, 1);
DEFINE_CONV_TEST(4, 2);
DEFINE_CONV_TEST(4, 3);
DEFINE_CONV_TEST(4, 8);
DEFINE_CONV_TEST(4, 11);
DEFINE_CONV_TEST(5, 1);
DEFINE_CONV_TEST(5, 2);
DEFINE_CONV_TEST(5, 3);
DEFINE_CONV_TEST(5, 8);
DEFINE_CONV_TEST(5, 11);
DEFINE_CONV_TEST(6, 1);
DEFINE_CONV_TEST(6, 2);
DEFINE_CONV_TEST(6, 3);
DEFINE_CONV_TEST(6, 8);
DEFINE_CONV_TEST(6, 11);
DEFINE_CONV_TEST(9, 1);
DEFINE_CONV_TEST(9, 2);
DEFINE_CONV_TEST(9, 3);
DEFINE_CONV_TEST(9, 8);
DEFINE_CONV_TEST(9, 11);
DEFINE_CONV_TEST(10, 1);
DEFINE_CONV_TEST(10, 2);
DEFINE_CONV_TEST(10, 3);
DEFINE_CONV_TEST(10, 8);
DEFINE_CONV_TEST(10, 11);
DEFINE_CONV_TEST(11, 1);
DEFINE_CONV_TEST(11, 2);
DEFINE_CONV_TEST(11, 3);
DEFINE_CONV_TEST(11, 8);
DEFINE_CONV_TEST(11, 11);
DEFINE_CONV_TEST(12, 1);
DEFINE_CONV_TEST(12, 2);
DEFINE_CONV_TEST(12, 3);
DEFINE_CONV_TEST(12, 8);
DEFINE_CONV_TEST(12, 11);
DEFINE_CONV_TEST(13, 1);
DEFINE_CONV_TEST(13, 2);
DEFINE_CONV_TEST(13, 3);
DEFINE_CONV_TEST(13, 8);
DEFINE_CONV_TEST(13, 11);

#ifdef CONFIG_CMSIS_DSP_TEST_FILTERING_MISC_CONV_PARTIAL
static void test_arm_conv_partial_f32(
	size_t first, size_t in1_length, size_t in2_length,
	const uint32_t *ref, size_t ref_length)
{
	float32_t *output;
	float32_t *temp;
	arm_status status;

	/* Allocate output buffer */
	output = calloc(first + ref_length, sizeof(float32_t));
	temp = calloc(ref_length, sizeof(float32_t));

	/* Run test function */
	status = arm_conv_partial_f32(
			(const float32_t *)in_partial1, in1_length,
			(const float32_t *)in_partial2, in2_length,
			output, first, ref_length);

	zassert_equal(status, ARM_MATH_SUCCESS,
		      ASSERT_MSG_INCORRECT_COMP_RESULT);

	memcpy(temp, &output[first], ref_length * sizeof(float32_t));

	/* Validate output */
	zassert_true(
		test_snr_error_f32(ref_length, (const float32_t *)ref, temp,
			SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_close_error_f32(ref_length, (const float32_t *)ref, temp,
			ABS_ERROR_THRESH, REL_ERROR_THRESH),
		ASSERT_MSG_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
	free(temp);
}
#else
static void test_arm_conv_partial_f32(
	size_t first, size_t in1_length, size_t in2_length,
	const uint32_t *ref, size_t ref_length)
{
	ztest_test_skip();
}
#endif /* CONFIG_CMSIS_DSP_TEST_FILTERING_MISC_CONV_PARTIAL */

#define DEFINE_CONV_PARTIAL_TEST(a, b, c) \
	DEFINE_TEST_VARIANT5( \
		arm_conv_partial_f32, a##_##b##_##c, a, b, c, \
		ref_conv_partial_##a##_##b##_##c, \
		ARRAY_SIZE(ref_conv_partial_##a##_##b##_##c))

DEFINE_CONV_PARTIAL_TEST(3, 6, 8);
DEFINE_CONV_PARTIAL_TEST(9, 6, 8);
DEFINE_CONV_PARTIAL_TEST(7, 6, 8);

static void test_arm_levinson_durbin_f32(
	size_t in_length, size_t err_index, const uint32_t *in,
	const uint32_t *ref, size_t ref_length)
{
	float32_t *output;
	float32_t err;

	/* Allocate output buffer */
	output = calloc(ref_length, sizeof(float32_t));

	/* Run test function */
	arm_levinson_durbin_f32((const float32_t *)in, output, &err,
				in_length);

	/* Validate output */
	zassert_true(
		test_snr_error_f32(ref_length, (const float32_t *)ref, output,
			SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_close_error_f32(ref_length, (const float32_t *)ref,
			output, ABS_ERROR_THRESH_LD, REL_ERROR_THRESH_LD),
		ASSERT_MSG_ERROR_LIMIT_EXCEED);

	zassert_true(
		test_close_error_f32(1,
			(const float32_t *)&in_levinson_durbin_err[err_index],
			&(err), ABS_ERROR_THRESH_LD, REL_ERROR_THRESH_LD),
		ASSERT_MSG_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

#define DEFINE_LEVINSON_DURBIN_TEST(a, b) \
	DEFINE_TEST_VARIANT5( \
		arm_levinson_durbin_f32, a##_##b, a, b, \
		in_levinson_durbin_##a##_##b, \
		ref_levinson_durbin_##a##_##b, \
		ARRAY_SIZE(ref_levinson_durbin_##a##_##b))

DEFINE_LEVINSON_DURBIN_TEST(3, 0);
DEFINE_LEVINSON_DURBIN_TEST(8, 1);
DEFINE_LEVINSON_DURBIN_TEST(11, 2);

void test_filtering_misc_f32(void)
{
	ztest_test_suite(filtering_misc_f32,
		ztest_unit_test(test_arm_correlate_f32_4_1),
		ztest_unit_test(test_arm_correlate_f32_4_2),
		ztest_unit_test(test_arm_correlate_f32_4_3),
		ztest_unit_test(test_arm_correlate_f32_4_8),
		ztest_unit_test(test_arm_correlate_f32_4_11),
		ztest_unit_test(test_arm_correlate_f32_5_1),
		ztest_unit_test(test_arm_correlate_f32_5_2),
		ztest_unit_test(test_arm_correlate_f32_5_3),
		ztest_unit_test(test_arm_correlate_f32_5_8),
		ztest_unit_test(test_arm_correlate_f32_5_11),
		ztest_unit_test(test_arm_correlate_f32_6_1),
		ztest_unit_test(test_arm_correlate_f32_6_2),
		ztest_unit_test(test_arm_correlate_f32_6_3),
		ztest_unit_test(test_arm_correlate_f32_6_8),
		ztest_unit_test(test_arm_correlate_f32_6_11),
		ztest_unit_test(test_arm_correlate_f32_9_1),
		ztest_unit_test(test_arm_correlate_f32_9_2),
		ztest_unit_test(test_arm_correlate_f32_9_3),
		ztest_unit_test(test_arm_correlate_f32_9_8),
		ztest_unit_test(test_arm_correlate_f32_9_11),
		ztest_unit_test(test_arm_correlate_f32_10_1),
		ztest_unit_test(test_arm_correlate_f32_10_2),
		ztest_unit_test(test_arm_correlate_f32_10_3),
		ztest_unit_test(test_arm_correlate_f32_10_8),
		ztest_unit_test(test_arm_correlate_f32_10_11),
		ztest_unit_test(test_arm_correlate_f32_11_1),
		ztest_unit_test(test_arm_correlate_f32_11_2),
		ztest_unit_test(test_arm_correlate_f32_11_3),
		ztest_unit_test(test_arm_correlate_f32_11_8),
		ztest_unit_test(test_arm_correlate_f32_11_11),
		ztest_unit_test(test_arm_correlate_f32_12_1),
		ztest_unit_test(test_arm_correlate_f32_12_2),
		ztest_unit_test(test_arm_correlate_f32_12_3),
		ztest_unit_test(test_arm_correlate_f32_12_8),
		ztest_unit_test(test_arm_correlate_f32_12_11),
		ztest_unit_test(test_arm_correlate_f32_13_1),
		ztest_unit_test(test_arm_correlate_f32_13_2),
		ztest_unit_test(test_arm_correlate_f32_13_3),
		ztest_unit_test(test_arm_correlate_f32_13_8),
		ztest_unit_test(test_arm_correlate_f32_13_11),
		ztest_unit_test(test_arm_conv_f32_4_1),
		ztest_unit_test(test_arm_conv_f32_4_2),
		ztest_unit_test(test_arm_conv_f32_4_3),
		ztest_unit_test(test_arm_conv_f32_4_8),
		ztest_unit_test(test_arm_conv_f32_4_11),
		ztest_unit_test(test_arm_conv_f32_5_1),
		ztest_unit_test(test_arm_conv_f32_5_2),
		ztest_unit_test(test_arm_conv_f32_5_3),
		ztest_unit_test(test_arm_conv_f32_5_8),
		ztest_unit_test(test_arm_conv_f32_5_11),
		ztest_unit_test(test_arm_conv_f32_6_1),
		ztest_unit_test(test_arm_conv_f32_6_2),
		ztest_unit_test(test_arm_conv_f32_6_3),
		ztest_unit_test(test_arm_conv_f32_6_8),
		ztest_unit_test(test_arm_conv_f32_6_11),
		ztest_unit_test(test_arm_conv_f32_9_1),
		ztest_unit_test(test_arm_conv_f32_9_2),
		ztest_unit_test(test_arm_conv_f32_9_3),
		ztest_unit_test(test_arm_conv_f32_9_8),
		ztest_unit_test(test_arm_conv_f32_9_11),
		ztest_unit_test(test_arm_conv_f32_10_1),
		ztest_unit_test(test_arm_conv_f32_10_2),
		ztest_unit_test(test_arm_conv_f32_10_3),
		ztest_unit_test(test_arm_conv_f32_10_8),
		ztest_unit_test(test_arm_conv_f32_10_11),
		ztest_unit_test(test_arm_conv_f32_11_1),
		ztest_unit_test(test_arm_conv_f32_11_2),
		ztest_unit_test(test_arm_conv_f32_11_3),
		ztest_unit_test(test_arm_conv_f32_11_8),
		ztest_unit_test(test_arm_conv_f32_11_11),
		ztest_unit_test(test_arm_conv_f32_12_1),
		ztest_unit_test(test_arm_conv_f32_12_2),
		ztest_unit_test(test_arm_conv_f32_12_3),
		ztest_unit_test(test_arm_conv_f32_12_8),
		ztest_unit_test(test_arm_conv_f32_12_11),
		ztest_unit_test(test_arm_conv_f32_13_1),
		ztest_unit_test(test_arm_conv_f32_13_2),
		ztest_unit_test(test_arm_conv_f32_13_3),
		ztest_unit_test(test_arm_conv_f32_13_8),
		ztest_unit_test(test_arm_conv_f32_13_11),
		ztest_unit_test(test_arm_conv_partial_f32_3_6_8),
		ztest_unit_test(test_arm_conv_partial_f32_9_6_8),
		ztest_unit_test(test_arm_conv_partial_f32_7_6_8),
		ztest_unit_test(test_arm_levinson_durbin_f32_3_0),
		ztest_unit_test(test_arm_levinson_durbin_f32_8_1),
		ztest_unit_test(test_arm_levinson_durbin_f32_11_2)
		);

	ztest_run_test_suite(filtering_misc_f32);
}
