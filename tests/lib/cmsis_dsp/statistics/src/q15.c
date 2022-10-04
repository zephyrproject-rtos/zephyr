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

#include "q15.pat"

#define SNR_ERROR_THRESH	((float32_t)50)
#define ABS_ERROR_THRESH_Q15	((q15_t)100)
#define ABS_ERROR_THRESH_Q63	((q63_t)(1 << 17))

static void test_arm_max_q15(
	const q15_t *input1, int ref_index, size_t length)
{
	q15_t val;
	uint32_t index;

	/* Run test function */
	arm_max_q15(input1, length, &val, &index);

	/* Validate output */
	zassert_equal(val, ref_max_val[ref_index],
		ASSERT_MSG_INCORRECT_COMP_RESULT);

	zassert_equal(index, ref_max_idx[ref_index],
		ASSERT_MSG_INCORRECT_COMP_RESULT);
}

DEFINE_TEST_VARIANT3(arm_max_q15, 7, in_com1, 0, 7);
DEFINE_TEST_VARIANT3(arm_max_q15, 16, in_com1, 1, 16);
DEFINE_TEST_VARIANT3(arm_max_q15, 23, in_com1, 2, 23);

static void test_arm_min_q15(
	const q15_t *input1, int ref_index, size_t length)
{
	q15_t val;
	uint32_t index;

	/* Run test function */
	arm_min_q15(input1, length, &val, &index);

	/* Validate output */
	zassert_equal(val, ref_min_val[ref_index],
		ASSERT_MSG_INCORRECT_COMP_RESULT);

	zassert_equal(index, ref_min_idx[ref_index],
		ASSERT_MSG_INCORRECT_COMP_RESULT);
}

DEFINE_TEST_VARIANT3(arm_min_q15, 7, in_com1, 0, 7);
DEFINE_TEST_VARIANT3(arm_min_q15, 16, in_com1, 1, 16);
DEFINE_TEST_VARIANT3(arm_min_q15, 23, in_com1, 2, 23);

static void test_arm_absmax_q15(
	const q15_t *input1, int ref_index, size_t length)
{
	q15_t val;
	uint32_t index;

	/* Run test function */
	arm_absmax_q15(input1, length, &val, &index);

	/* Validate output */
	zassert_equal(val, ref_absmax_val[ref_index],
		ASSERT_MSG_INCORRECT_COMP_RESULT);

	zassert_equal(index, ref_absmax_idx[ref_index],
		ASSERT_MSG_INCORRECT_COMP_RESULT);
}

DEFINE_TEST_VARIANT3(arm_absmax_q15, 7, in_absminmax, 0, 7);
DEFINE_TEST_VARIANT3(arm_absmax_q15, 16, in_absminmax, 1, 16);
DEFINE_TEST_VARIANT3(arm_absmax_q15, 23, in_absminmax, 2, 23);

static void test_arm_absmin_q15(
	const q15_t *input1, int ref_index, size_t length)
{
	q15_t val;
	uint32_t index;

	/* Run test function */
	arm_absmin_q15(input1, length, &val, &index);

	/* Validate output */
	zassert_equal(val, ref_absmin_val[ref_index],
		ASSERT_MSG_INCORRECT_COMP_RESULT);

	zassert_equal(index, ref_absmin_idx[ref_index],
		ASSERT_MSG_INCORRECT_COMP_RESULT);
}

DEFINE_TEST_VARIANT3(arm_absmin_q15, 7, in_absminmax, 0, 7);
DEFINE_TEST_VARIANT3(arm_absmin_q15, 16, in_absminmax, 1, 16);
DEFINE_TEST_VARIANT3(arm_absmin_q15, 23, in_absminmax, 2, 23);

static void test_arm_mean_q15(
	const q15_t *input1, int ref_index, size_t length)
{
	q15_t ref[1];
	q15_t *output;

	/* Load reference */
	ref[0] = ref_mean[ref_index];

	/* Allocate output buffer */
	output = malloc(1 * sizeof(q15_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_mean_q15(input1, length, &output[0]);

	/* Validate output */
	zassert_true(
		test_snr_error_q15(1, output, ref, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_near_equal_q15(1, output, ref, ABS_ERROR_THRESH_Q15),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT3(arm_mean_q15, 7, in_com2, 0, 7);
DEFINE_TEST_VARIANT3(arm_mean_q15, 16, in_com2, 1, 16);
DEFINE_TEST_VARIANT3(arm_mean_q15, 23, in_com2, 2, 23);

static void test_arm_power_q15(
	const q15_t *input1, int ref_index, size_t length)
{
	q63_t ref[1];
	q63_t *output;

	/* Load reference */
	ref[0] = ref_power[ref_index];

	/* Allocate output buffer */
	output = malloc(1 * sizeof(q63_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_power_q15(input1, length, &output[0]);

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

DEFINE_TEST_VARIANT3(arm_power_q15, 7, in_com1, 0, 7);
DEFINE_TEST_VARIANT3(arm_power_q15, 16, in_com1, 1, 16);
DEFINE_TEST_VARIANT3(arm_power_q15, 23, in_com1, 2, 23);

static void test_arm_rms_q15(
	const q15_t *input1, int ref_index, size_t length)
{
	q15_t ref[1];
	q15_t *output;

	/* Load reference */
	ref[0] = ref_rms[ref_index];

	/* Allocate output buffer */
	output = malloc(1 * sizeof(q15_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_rms_q15(input1, length, &output[0]);

	/* Validate output */
	zassert_true(
		test_snr_error_q15(1, output, ref, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_near_equal_q15(1, output, ref, ABS_ERROR_THRESH_Q15),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT3(arm_rms_q15, 7, in_com1, 0, 7);
DEFINE_TEST_VARIANT3(arm_rms_q15, 16, in_com1, 1, 16);
DEFINE_TEST_VARIANT3(arm_rms_q15, 23, in_com1, 2, 23);

static void test_arm_std_q15(
	const q15_t *input1, int ref_index, size_t length)
{
	q15_t ref[1];
	q15_t *output;

	/* Load reference */
	ref[0] = ref_std[ref_index];

	/* Allocate output buffer */
	output = malloc(1 * sizeof(q15_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_std_q15(input1, length, &output[0]);

	/* Validate output */
	zassert_true(
		test_snr_error_q15(1, output, ref, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_near_equal_q15(1, output, ref, ABS_ERROR_THRESH_Q15),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT3(arm_std_q15, 7, in_com1, 0, 7);
DEFINE_TEST_VARIANT3(arm_std_q15, 16, in_com1, 1, 16);
DEFINE_TEST_VARIANT3(arm_std_q15, 23, in_com1, 2, 23);

static void test_arm_var_q15(
	const q15_t *input1, int ref_index, size_t length)
{
	q15_t ref[1];
	q15_t *output;

	/* Load reference */
	ref[0] = ref_var[ref_index];

	/* Allocate output buffer */
	output = malloc(1 * sizeof(q15_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_var_q15(input1, length, &output[0]);

	/* Validate output */
	zassert_true(
		test_snr_error_q15(1, output, ref, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_near_equal_q15(1, output, ref, ABS_ERROR_THRESH_Q15),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT3(arm_var_q15, 7, in_com1, 0, 7);
DEFINE_TEST_VARIANT3(arm_var_q15, 16, in_com1, 1, 16);
DEFINE_TEST_VARIANT3(arm_var_q15, 23, in_com1, 2, 23);

void test_statistics_q15(void)
{
	ztest_test_suite(statistics_q15,
		ztest_unit_test(test_arm_max_q15_7),
		ztest_unit_test(test_arm_max_q15_16),
		ztest_unit_test(test_arm_max_q15_23),
		ztest_unit_test(test_arm_min_q15_7),
		ztest_unit_test(test_arm_min_q15_16),
		ztest_unit_test(test_arm_min_q15_23),
		ztest_unit_test(test_arm_absmax_q15_7),
		ztest_unit_test(test_arm_absmax_q15_16),
		ztest_unit_test(test_arm_absmax_q15_23),
		ztest_unit_test(test_arm_absmin_q15_7),
		ztest_unit_test(test_arm_absmin_q15_16),
		ztest_unit_test(test_arm_absmin_q15_23),
		ztest_unit_test(test_arm_mean_q15_7),
		ztest_unit_test(test_arm_mean_q15_16),
		ztest_unit_test(test_arm_mean_q15_23),
		ztest_unit_test(test_arm_power_q15_7),
		ztest_unit_test(test_arm_power_q15_16),
		ztest_unit_test(test_arm_power_q15_23),
		ztest_unit_test(test_arm_rms_q15_7),
		ztest_unit_test(test_arm_rms_q15_16),
		ztest_unit_test(test_arm_rms_q15_23),
		ztest_unit_test(test_arm_std_q15_7),
		ztest_unit_test(test_arm_std_q15_16),
		ztest_unit_test(test_arm_std_q15_23),
		ztest_unit_test(test_arm_var_q15_7),
		ztest_unit_test(test_arm_var_q15_16),
		ztest_unit_test(test_arm_var_q15_23)
		);

	ztest_run_test_suite(statistics_q15);
}
