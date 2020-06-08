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

#include "misc_q15.pat"

#define SNR_ERROR_THRESH	((float32_t)70)
#define ABS_ERROR_THRESH_Q15	((q15_t)10)

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
		ztest_unit_test(test_arm_conv_q15_32_33)
		);

	ztest_run_test_suite(filtering_misc_q15);
}
