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

#include "q7.pat"

#define SNR_ERROR_THRESH	((float32_t)20)
#define ABS_ERROR_THRESH_Q7	((q7_t)20)
#define ABS_ERROR_THRESH_Q31	((q31_t)(1 << 15))

static void test_arm_max_q7(
	const q7_t *input1, int ref_index, size_t length)
{
	q7_t val;
	uint32_t index;

	/* Run test function */
	arm_max_q7(input1, length, &val, &index);

	/* Validate output */
	zassert_equal(val, ref_max_val[ref_index],
		ASSERT_MSG_INCORRECT_COMP_RESULT);

	zassert_equal(index, ref_max_idx[ref_index],
		ASSERT_MSG_INCORRECT_COMP_RESULT);
}

DEFINE_TEST_VARIANT3(arm_max_q7, 15, in_com1, 0, 15);
DEFINE_TEST_VARIANT3(arm_max_q7, 32, in_com1, 1, 32);
DEFINE_TEST_VARIANT3(arm_max_q7, 47, in_com1, 2, 47);
DEFINE_TEST_VARIANT3(arm_max_q7, max, in_max_maxidx, 3, 280);

static void test_arm_min_q7(
	const q7_t *input1, int ref_index, size_t length)
{
	q7_t val;
	uint32_t index;

	/* Run test function */
	arm_min_q7(input1, length, &val, &index);

	/* Validate output */
	zassert_equal(val, ref_min_val[ref_index],
		ASSERT_MSG_INCORRECT_COMP_RESULT);

	zassert_equal(index, ref_min_idx[ref_index],
		ASSERT_MSG_INCORRECT_COMP_RESULT);
}

DEFINE_TEST_VARIANT3(arm_min_q7, 15, in_com1, 0, 15);
DEFINE_TEST_VARIANT3(arm_min_q7, 32, in_com1, 1, 32);
DEFINE_TEST_VARIANT3(arm_min_q7, 47, in_com1, 2, 47);
DEFINE_TEST_VARIANT3(arm_min_q7, max, in_min_maxidx, 3, 280);

static void test_arm_mean_q7(
	const q7_t *input1, int ref_index, size_t length)
{
	q7_t ref[1];
	q7_t *output;

	/* Load reference */
	ref[0] = ref_mean[ref_index];

	/* Allocate output buffer */
	output = malloc(1 * sizeof(q7_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_mean_q7(input1, length, &output[0]);

	/* Validate output */
	zassert_true(
		test_snr_error_q7(1, output, ref, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_near_equal_q7(1, output, ref, ABS_ERROR_THRESH_Q7),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT3(arm_mean_q7, 15, in_com2, 0, 15);
DEFINE_TEST_VARIANT3(arm_mean_q7, 32, in_com2, 1, 32);
DEFINE_TEST_VARIANT3(arm_mean_q7, 47, in_com2, 2, 47);

static void test_arm_power_q7(
	const q7_t *input1, int ref_index, size_t length)
{
	q31_t ref[1];
	q31_t *output;

	/* Load reference */
	ref[0] = ref_power[ref_index];

	/* Allocate output buffer */
	output = malloc(1 * sizeof(q31_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_power_q7(input1, length, &output[0]);

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

DEFINE_TEST_VARIANT3(arm_power_q7, 15, in_com1, 0, 15);
DEFINE_TEST_VARIANT3(arm_power_q7, 32, in_com1, 1, 32);
DEFINE_TEST_VARIANT3(arm_power_q7, 47, in_com1, 2, 47);

void test_statistics_q7(void)
{
	ztest_test_suite(statistics_q7,
		ztest_unit_test(test_arm_max_q7_15),
		ztest_unit_test(test_arm_max_q7_32),
		ztest_unit_test(test_arm_max_q7_47),
		ztest_unit_test(test_arm_max_q7_max),
		ztest_unit_test(test_arm_min_q7_15),
		ztest_unit_test(test_arm_min_q7_32),
		ztest_unit_test(test_arm_min_q7_47),
		ztest_unit_test(test_arm_min_q7_max),
		ztest_unit_test(test_arm_mean_q7_15),
		ztest_unit_test(test_arm_mean_q7_32),
		ztest_unit_test(test_arm_mean_q7_47),
		ztest_unit_test(test_arm_power_q7_15),
		ztest_unit_test(test_arm_power_q7_32),
		ztest_unit_test(test_arm_power_q7_47)
		);

	ztest_run_test_suite(statistics_q7);
}
