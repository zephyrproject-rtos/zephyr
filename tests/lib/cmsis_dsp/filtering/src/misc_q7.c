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
	DEFINE_TEST_VARIANT4( \
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
	DEFINE_TEST_VARIANT4( \
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

void test_filtering_misc_q7(void)
{
	ztest_test_suite(filtering_misc_q7,
		ztest_unit_test(test_arm_correlate_q7_30_31),
		ztest_unit_test(test_arm_correlate_q7_30_32),
		ztest_unit_test(test_arm_correlate_q7_30_33),
		ztest_unit_test(test_arm_correlate_q7_30_34),
		ztest_unit_test(test_arm_correlate_q7_30_49),
		ztest_unit_test(test_arm_correlate_q7_31_31),
		ztest_unit_test(test_arm_correlate_q7_31_32),
		ztest_unit_test(test_arm_correlate_q7_31_33),
		ztest_unit_test(test_arm_correlate_q7_31_34),
		ztest_unit_test(test_arm_correlate_q7_31_49),
		ztest_unit_test(test_arm_correlate_q7_32_31),
		ztest_unit_test(test_arm_correlate_q7_32_32),
		ztest_unit_test(test_arm_correlate_q7_32_33),
		ztest_unit_test(test_arm_correlate_q7_32_34),
		ztest_unit_test(test_arm_correlate_q7_32_49),
		ztest_unit_test(test_arm_correlate_q7_33_31),
		ztest_unit_test(test_arm_correlate_q7_33_32),
		ztest_unit_test(test_arm_correlate_q7_33_33),
		ztest_unit_test(test_arm_correlate_q7_33_34),
		ztest_unit_test(test_arm_correlate_q7_33_49),
		ztest_unit_test(test_arm_correlate_q7_48_31),
		ztest_unit_test(test_arm_correlate_q7_48_32),
		ztest_unit_test(test_arm_correlate_q7_48_33),
		ztest_unit_test(test_arm_correlate_q7_48_34),
		ztest_unit_test(test_arm_correlate_q7_48_49),
		ztest_unit_test(test_arm_conv_q7_30_31),
		ztest_unit_test(test_arm_conv_q7_30_32),
		ztest_unit_test(test_arm_conv_q7_30_33),
		ztest_unit_test(test_arm_conv_q7_30_34),
		ztest_unit_test(test_arm_conv_q7_30_49),
		ztest_unit_test(test_arm_conv_q7_31_31),
		ztest_unit_test(test_arm_conv_q7_31_32),
		ztest_unit_test(test_arm_conv_q7_31_33),
		ztest_unit_test(test_arm_conv_q7_31_34),
		ztest_unit_test(test_arm_conv_q7_31_49),
		ztest_unit_test(test_arm_conv_q7_32_31),
		ztest_unit_test(test_arm_conv_q7_32_32),
		ztest_unit_test(test_arm_conv_q7_32_33),
		ztest_unit_test(test_arm_conv_q7_32_34),
		ztest_unit_test(test_arm_conv_q7_32_49),
		ztest_unit_test(test_arm_conv_q7_33_31),
		ztest_unit_test(test_arm_conv_q7_33_32),
		ztest_unit_test(test_arm_conv_q7_33_33),
		ztest_unit_test(test_arm_conv_q7_33_34),
		ztest_unit_test(test_arm_conv_q7_33_49),
		ztest_unit_test(test_arm_conv_q7_48_31),
		ztest_unit_test(test_arm_conv_q7_48_32),
		ztest_unit_test(test_arm_conv_q7_48_33),
		ztest_unit_test(test_arm_conv_q7_48_34),
		ztest_unit_test(test_arm_conv_q7_48_49)
		);

	ztest_run_test_suite(filtering_misc_q7);
}
