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

#include "misc_f32.pat"

#define SNR_ERROR_THRESH	((float32_t)120)
#define REL_ERROR_THRESH	(1.0e-6)
#define ABS_ERROR_THRESH	(1.0e-5)

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
		ztest_unit_test(test_arm_conv_f32_13_11)
		);

	ztest_run_test_suite(filtering_misc_f32);
}
