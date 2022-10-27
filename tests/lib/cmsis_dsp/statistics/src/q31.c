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

#include "q31.pat"

#define SNR_ERROR_THRESH	((float32_t)100)
#define ABS_ERROR_THRESH_Q15	((q31_t)100)
#define ABS_ERROR_THRESH_Q63	((q63_t)(1 << 18))

static void test_arm_max_q31(
	const q31_t *input1, int ref_index, size_t length)
{
	q31_t val;
	uint32_t index;

	/* Run test function */
	arm_max_q31(input1, length, &val, &index);

	/* Validate output */
	zassert_equal(val, ref_max_val[ref_index],
		ASSERT_MSG_INCORRECT_COMP_RESULT);

	zassert_equal(index, ref_max_idx[ref_index],
		ASSERT_MSG_INCORRECT_COMP_RESULT);
}

DEFINE_TEST_VARIANT3(statistics_q31, arm_max_q31, 3, in_com1, 0, 3);
DEFINE_TEST_VARIANT3(statistics_q31, arm_max_q31, 8, in_com1, 1, 8);
DEFINE_TEST_VARIANT3(statistics_q31, arm_max_q31, 11, in_com1, 2, 11);

static void test_arm_min_q31(
	const q31_t *input1, int ref_index, size_t length)
{
	q31_t val;
	uint32_t index;

	/* Run test function */
	arm_min_q31(input1, length, &val, &index);

	/* Validate output */
	zassert_equal(val, ref_min_val[ref_index],
		ASSERT_MSG_INCORRECT_COMP_RESULT);

	zassert_equal(index, ref_min_idx[ref_index],
		ASSERT_MSG_INCORRECT_COMP_RESULT);
}

DEFINE_TEST_VARIANT3(statistics_q31, arm_min_q31, 3, in_com1, 0, 3);
DEFINE_TEST_VARIANT3(statistics_q31, arm_min_q31, 8, in_com1, 1, 8);
DEFINE_TEST_VARIANT3(statistics_q31, arm_min_q31, 11, in_com1, 2, 11);

static void test_arm_absmax_q31(
	const q31_t *input1, int ref_index, size_t length)
{
	q31_t val;
	uint32_t index;

	/* Run test function */
	arm_absmax_q31(input1, length, &val, &index);

	/* Validate output */
	zassert_equal(val, ref_absmax_val[ref_index],
		ASSERT_MSG_INCORRECT_COMP_RESULT);

	zassert_equal(index, ref_absmax_idx[ref_index],
		ASSERT_MSG_INCORRECT_COMP_RESULT);
}

DEFINE_TEST_VARIANT3(statistics_q31, arm_absmax_q31, 3, in_absminmax, 0, 3);
DEFINE_TEST_VARIANT3(statistics_q31, arm_absmax_q31, 8, in_absminmax, 1, 8);
DEFINE_TEST_VARIANT3(statistics_q31, arm_absmax_q31, 11, in_absminmax, 2, 11);

static void test_arm_absmin_q31(
	const q31_t *input1, int ref_index, size_t length)
{
	q31_t val;
	uint32_t index;

	/* Run test function */
	arm_absmin_q31(input1, length, &val, &index);

	/* Validate output */
	zassert_equal(val, ref_absmin_val[ref_index],
		ASSERT_MSG_INCORRECT_COMP_RESULT);

	zassert_equal(index, ref_absmin_idx[ref_index],
		ASSERT_MSG_INCORRECT_COMP_RESULT);
}

DEFINE_TEST_VARIANT3(statistics_q31, arm_absmin_q31, 3, in_absminmax, 0, 3);
DEFINE_TEST_VARIANT3(statistics_q31, arm_absmin_q31, 8, in_absminmax, 1, 8);
DEFINE_TEST_VARIANT3(statistics_q31, arm_absmin_q31, 11, in_absminmax, 2, 11);

static void test_arm_mean_q31(
	const q31_t *input1, int ref_index, size_t length)
{
	q31_t ref[1];
	q31_t *output;

	/* Load reference */
	ref[0] = ref_mean[ref_index];

	/* Allocate output buffer */
	output = malloc(1 * sizeof(q31_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_mean_q31(input1, length, &output[0]);

	/* Validate output */
	zassert_true(
		test_snr_error_q31(1, output, ref, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_near_equal_q31(1, output, ref, ABS_ERROR_THRESH_Q15),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT3(statistics_q31, arm_mean_q31, 3, in_com2, 0, 3);
DEFINE_TEST_VARIANT3(statistics_q31, arm_mean_q31, 8, in_com2, 1, 8);
DEFINE_TEST_VARIANT3(statistics_q31, arm_mean_q31, 11, in_com2, 2, 11);

static void test_arm_power_q31(
	const q31_t *input1, int ref_index, size_t length)
{
	q63_t ref[1];
	q63_t *output;

	/* Load reference */
	ref[0] = ref_power[ref_index];

	/* Allocate output buffer */
	output = malloc(1 * sizeof(q63_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_power_q31(input1, length, &output[0]);

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

DEFINE_TEST_VARIANT3(statistics_q31, arm_power_q31, 3, in_com1, 0, 3);
DEFINE_TEST_VARIANT3(statistics_q31, arm_power_q31, 8, in_com1, 1, 8);
DEFINE_TEST_VARIANT3(statistics_q31, arm_power_q31, 11, in_com1, 2, 11);

static void test_arm_rms_q31(
	const q31_t *input1, int ref_index, size_t length)
{
	q31_t ref[1];
	q31_t *output;

	/* Load reference */
	ref[0] = ref_rms[ref_index];

	/* Allocate output buffer */
	output = malloc(1 * sizeof(q31_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_rms_q31(input1, length, &output[0]);

	/* Validate output */
	zassert_true(
		test_snr_error_q31(1, output, ref, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_near_equal_q31(1, output, ref, ABS_ERROR_THRESH_Q15),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT3(statistics_q31, arm_rms_q31, 3, in_com1, 0, 3);
DEFINE_TEST_VARIANT3(statistics_q31, arm_rms_q31, 8, in_com1, 1, 8);
DEFINE_TEST_VARIANT3(statistics_q31, arm_rms_q31, 11, in_com1, 2, 11);

static void test_arm_std_q31(
	const q31_t *input1, int ref_index, size_t length)
{
	q31_t ref[1];
	q31_t *output;

	/* Load reference */
	ref[0] = ref_std[ref_index];

	/* Allocate output buffer */
	output = malloc(1 * sizeof(q31_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_std_q31(input1, length, &output[0]);

	/* Validate output */
	zassert_true(
		test_snr_error_q31(1, output, ref, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_near_equal_q31(1, output, ref, ABS_ERROR_THRESH_Q15),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT3(statistics_q31, arm_std_q31, 3, in_com1, 0, 3);
DEFINE_TEST_VARIANT3(statistics_q31, arm_std_q31, 8, in_com1, 1, 8);
DEFINE_TEST_VARIANT3(statistics_q31, arm_std_q31, 11, in_com1, 2, 11);

static void test_arm_var_q31(
	const q31_t *input1, int ref_index, size_t length)
{
	q31_t ref[1];
	q31_t *output;

	/* Load reference */
	ref[0] = ref_var[ref_index];

	/* Allocate output buffer */
	output = malloc(1 * sizeof(q31_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_var_q31(input1, length, &output[0]);

	/* Validate output */
	zassert_true(
		test_snr_error_q31(1, output, ref, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_near_equal_q31(1, output, ref, ABS_ERROR_THRESH_Q15),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT3(statistics_q31, arm_var_q31, 3, in_com1, 0, 3);
DEFINE_TEST_VARIANT3(statistics_q31, arm_var_q31, 8, in_com1, 1, 8);
DEFINE_TEST_VARIANT3(statistics_q31, arm_var_q31, 11, in_com1, 2, 11);

ZTEST_SUITE(statistics_q31, NULL, NULL, NULL, NULL, NULL);
