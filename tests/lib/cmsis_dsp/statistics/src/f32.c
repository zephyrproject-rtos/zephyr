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

#include "f32.pat"

#define SNR_ERROR_THRESH	((float32_t)120)
#define REL_ERROR_THRESH	(1.0e-5)

static void test_arm_max_f32(
	const uint32_t *input1, int ref_index, size_t length)
{
	float32_t val;
	uint32_t index;

	/* Run test function */
	arm_max_f32((float32_t *)input1, length, &val, &index);

	/* Validate output */
	zassert_equal(val, ((float32_t *)ref_max_val)[ref_index],
		ASSERT_MSG_INCORRECT_COMP_RESULT);

	zassert_equal(index, ref_max_idx[ref_index],
		ASSERT_MSG_INCORRECT_COMP_RESULT);
}

DEFINE_TEST_VARIANT3(arm_max_f32, 3, in_com1, 0, 3);
DEFINE_TEST_VARIANT3(arm_max_f32, 8, in_com1, 1, 8);
DEFINE_TEST_VARIANT3(arm_max_f32, 11, in_com1, 2, 11);

static void test_arm_max_no_idx_f32(
	const uint32_t *input1, int ref_index, size_t length)
{
	float32_t val;

	/* Run test function */
	arm_max_no_idx_f32((float32_t *)input1, length, &val);

	/* Validate output */
	zassert_equal(val, ((float32_t *)ref_max_val)[ref_index],
		ASSERT_MSG_INCORRECT_COMP_RESULT);
}

DEFINE_TEST_VARIANT3(arm_max_no_idx_f32, 3, in_com1, 0, 3);
DEFINE_TEST_VARIANT3(arm_max_no_idx_f32, 8, in_com1, 1, 8);
DEFINE_TEST_VARIANT3(arm_max_no_idx_f32, 11, in_com1, 2, 11);

static void test_arm_min_f32(
	const uint32_t *input1, int ref_index, size_t length)
{
	float32_t val;
	uint32_t index;

	/* Run test function */
	arm_min_f32((float32_t *)input1, length, &val, &index);

	/* Validate output */
	zassert_equal(val, ((float32_t *)ref_min_val)[ref_index],
		ASSERT_MSG_INCORRECT_COMP_RESULT);

	zassert_equal(index, ref_min_idx[ref_index],
		ASSERT_MSG_INCORRECT_COMP_RESULT);
}

DEFINE_TEST_VARIANT3(arm_min_f32, 3, in_com1, 0, 3);
DEFINE_TEST_VARIANT3(arm_min_f32, 8, in_com1, 1, 8);
DEFINE_TEST_VARIANT3(arm_min_f32, 11, in_com1, 2, 11);

static void test_arm_absmax_f32(
	const uint32_t *input1, int ref_index, size_t length)
{
	float32_t val;
	uint32_t index;

	/* Run test function */
	arm_absmax_f32((float32_t *)input1, length, &val, &index);

	/* Validate output */
	zassert_equal(val, ((float32_t *)ref_absmax_val)[ref_index],
		ASSERT_MSG_INCORRECT_COMP_RESULT);

	zassert_equal(index, ref_absmax_idx[ref_index],
		ASSERT_MSG_INCORRECT_COMP_RESULT);
}

DEFINE_TEST_VARIANT3(arm_absmax_f32, 3, in_absminmax, 0, 3);
DEFINE_TEST_VARIANT3(arm_absmax_f32, 8, in_absminmax, 1, 8);
DEFINE_TEST_VARIANT3(arm_absmax_f32, 11, in_absminmax, 2, 11);

static void test_arm_absmin_f32(
	const uint32_t *input1, int ref_index, size_t length)
{
	float32_t val;
	uint32_t index;

	/* Run test function */
	arm_absmin_f32((float32_t *)input1, length, &val, &index);

	/* Validate output */
	zassert_equal(val, ((float32_t *)ref_absmin_val)[ref_index],
		ASSERT_MSG_INCORRECT_COMP_RESULT);

	zassert_equal(index, ref_absmin_idx[ref_index],
		ASSERT_MSG_INCORRECT_COMP_RESULT);
}

DEFINE_TEST_VARIANT3(arm_absmin_f32, 3, in_absminmax, 0, 3);
DEFINE_TEST_VARIANT3(arm_absmin_f32, 8, in_absminmax, 1, 8);
DEFINE_TEST_VARIANT3(arm_absmin_f32, 11, in_absminmax, 2, 11);

static void test_arm_mean_f32(
	const uint32_t *input1, int ref_index, size_t length)
{
	float32_t ref[1];
	float32_t *output;

	/* Load reference */
	ref[0] = ((float32_t *)ref_mean)[ref_index];

	/* Allocate output buffer */
	output = malloc(1 * sizeof(float32_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_mean_f32((float32_t *)input1, length, &output[0]);

	/* Validate output */
	zassert_true(
		test_snr_error_f32(1, output, ref, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_rel_error_f32(1, output, ref, REL_ERROR_THRESH),
		ASSERT_MSG_REL_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT3(arm_mean_f32, 3, in_com2, 0, 3);
DEFINE_TEST_VARIANT3(arm_mean_f32, 8, in_com2, 1, 8);
DEFINE_TEST_VARIANT3(arm_mean_f32, 11, in_com2, 2, 11);

static void test_arm_power_f32(
	const uint32_t *input1, int ref_index, size_t length)
{
	float32_t ref[1];
	float32_t *output;

	/* Load reference */
	ref[0] = ((float32_t *)ref_power)[ref_index];

	/* Allocate output buffer */
	output = malloc(1 * sizeof(float32_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_power_f32((float32_t *)input1, length, &output[0]);

	/* Validate output */
	zassert_true(
		test_snr_error_f32(1, output, ref, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_rel_error_f32(1, output, ref, REL_ERROR_THRESH),
		ASSERT_MSG_REL_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT3(arm_power_f32, 3, in_com1, 0, 3);
DEFINE_TEST_VARIANT3(arm_power_f32, 8, in_com1, 1, 8);
DEFINE_TEST_VARIANT3(arm_power_f32, 11, in_com1, 2, 11);

static void test_arm_rms_f32(
	const uint32_t *input1, int ref_index, size_t length)
{
	float32_t ref[1];
	float32_t *output;

	/* Load reference */
	ref[0] = ((float32_t *)ref_rms)[ref_index];

	/* Allocate output buffer */
	output = malloc(1 * sizeof(float32_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_rms_f32((float32_t *)input1, length, &output[0]);

	/* Validate output */
	zassert_true(
		test_snr_error_f32(1, output, ref, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_rel_error_f32(1, output, ref, REL_ERROR_THRESH),
		ASSERT_MSG_REL_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT3(arm_rms_f32, 3, in_com1, 0, 3);
DEFINE_TEST_VARIANT3(arm_rms_f32, 8, in_com1, 1, 8);
DEFINE_TEST_VARIANT3(arm_rms_f32, 11, in_com1, 2, 11);

static void test_arm_std_f32(
	const uint32_t *input1, int ref_index, size_t length)
{
	float32_t ref[1];
	float32_t *output;

	/* Load reference */
	ref[0] = ((float32_t *)ref_std)[ref_index];

	/* Allocate output buffer */
	output = malloc(1 * sizeof(float32_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_std_f32((float32_t *)input1, length, &output[0]);

	/* Validate output */
	zassert_true(
		test_snr_error_f32(1, output, ref, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_rel_error_f32(1, output, ref, REL_ERROR_THRESH),
		ASSERT_MSG_REL_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT3(arm_std_f32, 3, in_com1, 0, 3);
DEFINE_TEST_VARIANT3(arm_std_f32, 8, in_com1, 1, 8);
DEFINE_TEST_VARIANT3(arm_std_f32, 11, in_com1, 2, 11);

static void test_arm_var_f32(
	const uint32_t *input1, int ref_index, size_t length)
{
	float32_t ref[1];
	float32_t *output;

	/* Load reference */
	ref[0] = ((float32_t *)ref_var)[ref_index];

	/* Allocate output buffer */
	output = malloc(1 * sizeof(float32_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_var_f32((float32_t *)input1, length, &output[0]);

	/* Validate output */
	zassert_true(
		test_snr_error_f32(1, output, ref, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_rel_error_f32(1, output, ref, REL_ERROR_THRESH),
		ASSERT_MSG_REL_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT3(arm_var_f32, 3, in_com1, 0, 3);
DEFINE_TEST_VARIANT3(arm_var_f32, 8, in_com1, 1, 8);
DEFINE_TEST_VARIANT3(arm_var_f32, 11, in_com1, 2, 11);

static void test_arm_entropy_f32(void)
{
	size_t index;
	size_t length = in_entropy_dim[0];
	const float32_t *ref = (float32_t *)ref_entropy;
	const float32_t *input = (float32_t *)in_entropy;
	float32_t *output;

	__ASSERT_NO_MSG(ARRAY_SIZE(in_entropy_dim) > length);
	__ASSERT_NO_MSG(ARRAY_SIZE(ref_entropy) >= length);

	/* Allocate output buffer */
	output = malloc(length * sizeof(float32_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	for (index = 0; index < length; index++) {
		output[index] =
			arm_entropy_f32(input, in_entropy_dim[index + 1]);
		input += in_entropy_dim[index + 1];
	}

	/* Validate output */
	zassert_true(
		test_snr_error_f32(length, ref, output, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_near_equal_f32(length, ref, output, REL_ERROR_THRESH),
		ASSERT_MSG_REL_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

static void test_arm_logsumexp_f32(void)
{
	size_t index;
	size_t length = in_logsumexp_dim[0];
	const float32_t *ref = (float32_t *)ref_logsumexp;
	const float32_t *input = (float32_t *)in_logsumexp;
	float32_t *output;

	__ASSERT_NO_MSG(ARRAY_SIZE(in_logsumexp_dim) > length);
	__ASSERT_NO_MSG(ARRAY_SIZE(ref_logsumexp) >= length);

	/* Allocate output buffer */
	output = malloc(length * sizeof(float32_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	for (index = 0; index < length; index++) {
		output[index] =
			arm_logsumexp_f32(input, in_logsumexp_dim[index + 1]);
		input += in_logsumexp_dim[index + 1];
	}

	/* Validate output */
	zassert_true(
		test_snr_error_f32(length, ref, output, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_near_equal_f32(length, ref, output, REL_ERROR_THRESH),
		ASSERT_MSG_REL_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

static void test_arm_kullback_leibler_f32(void)
{
	size_t index;
	size_t length = in_kl_dim[0];
	const float32_t *ref = (float32_t *)ref_kl;
	const float32_t *input1 = (float32_t *)in_kl1;
	const float32_t *input2 = (float32_t *)in_kl2;
	float32_t *output;

	__ASSERT_NO_MSG(ARRAY_SIZE(in_kl_dim) > length);
	__ASSERT_NO_MSG(ARRAY_SIZE(ref_kl) >= length);

	/* Allocate output buffer */
	output = malloc(length * sizeof(float32_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	for (index = 0; index < length; index++) {
		output[index] =
			arm_kullback_leibler_f32(
				input1, input2, in_kl_dim[index + 1]);

		input1 += in_kl_dim[index + 1];
		input2 += in_kl_dim[index + 1];
	}

	/* Validate output */
	zassert_true(
		test_snr_error_f32(length, ref, output, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_near_equal_f32(length, ref, output, REL_ERROR_THRESH),
		ASSERT_MSG_REL_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

static void test_arm_logsumexp_dot_prod_f32(void)
{
	size_t index;
	size_t length = in_logsumexp_dp_dim[0];
	const float32_t *ref = (float32_t *)ref_logsumexp_dp;
	const float32_t *input1 = (float32_t *)in_logsumexp_dp1;
	const float32_t *input2 = (float32_t *)in_logsumexp_dp2;
	float32_t *output;
	float32_t *tmp;

	__ASSERT_NO_MSG(ARRAY_SIZE(in_logsumexp_dp_dim) > length);
	__ASSERT_NO_MSG(ARRAY_SIZE(ref_logsumexp_dp) >= length);

	/* Allocate buffers */
	output = malloc(length * sizeof(float32_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	tmp = malloc(12 * sizeof(float32_t));
	zassert_not_null(tmp, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	for (index = 0; index < length; index++) {
		output[index] =
			arm_logsumexp_dot_prod_f32(
				input1, input2,
				in_logsumexp_dp_dim[index + 1], tmp);

		input1 += in_logsumexp_dp_dim[index + 1];
		input2 += in_logsumexp_dp_dim[index + 1];
	}

	/* Validate output */
	zassert_true(
		test_snr_error_f32(length, ref, output, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_near_equal_f32(length, ref, output, REL_ERROR_THRESH),
		ASSERT_MSG_REL_ERROR_LIMIT_EXCEED);

	/* Free buffers */
	free(output);
	free(tmp);
}

void test_statistics_f32(void)
{
	ztest_test_suite(statistics_f32,
		ztest_unit_test(test_arm_max_f32_3),
		ztest_unit_test(test_arm_max_f32_8),
		ztest_unit_test(test_arm_max_f32_11),
		ztest_unit_test(test_arm_min_f32_3),
		ztest_unit_test(test_arm_min_f32_8),
		ztest_unit_test(test_arm_min_f32_11),
		ztest_unit_test(test_arm_absmax_f32_3),
		ztest_unit_test(test_arm_absmax_f32_8),
		ztest_unit_test(test_arm_absmax_f32_11),
		ztest_unit_test(test_arm_absmin_f32_3),
		ztest_unit_test(test_arm_absmin_f32_8),
		ztest_unit_test(test_arm_absmin_f32_11),
		ztest_unit_test(test_arm_mean_f32_3),
		ztest_unit_test(test_arm_mean_f32_8),
		ztest_unit_test(test_arm_mean_f32_11),
		ztest_unit_test(test_arm_power_f32_3),
		ztest_unit_test(test_arm_power_f32_8),
		ztest_unit_test(test_arm_power_f32_11),
		ztest_unit_test(test_arm_rms_f32_3),
		ztest_unit_test(test_arm_rms_f32_8),
		ztest_unit_test(test_arm_rms_f32_11),
		ztest_unit_test(test_arm_std_f32_3),
		ztest_unit_test(test_arm_std_f32_8),
		ztest_unit_test(test_arm_std_f32_11),
		ztest_unit_test(test_arm_var_f32_3),
		ztest_unit_test(test_arm_var_f32_8),
		ztest_unit_test(test_arm_var_f32_11),
		ztest_unit_test(test_arm_entropy_f32),
		ztest_unit_test(test_arm_logsumexp_f32),
		ztest_unit_test(test_arm_kullback_leibler_f32),
		ztest_unit_test(test_arm_logsumexp_dot_prod_f32),
		ztest_unit_test(test_arm_max_no_idx_f32_3),
		ztest_unit_test(test_arm_max_no_idx_f32_8),
		ztest_unit_test(test_arm_max_no_idx_f32_11)
		);

	ztest_run_test_suite(statistics_f32);
}
