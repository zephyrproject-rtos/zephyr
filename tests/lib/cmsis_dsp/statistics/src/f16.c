/*
 * Copyright (c) 2021 Stephanos Ioannidis <root@stephanos.io>
 * Copyright (C) 2010-2021 ARM Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <stdlib.h>
#include <arm_math_f16.h>
#include "../../common/test_common.h"

#include "f16.pat"

#define SNR_ERROR_THRESH	((float32_t)48)
#define REL_ERROR_THRESH	(6.0e-3)

#define SNR_ERROR_THRESH_KB	((float32_t)40)
#define REL_ERROR_THRESH_KB	(5.0e-3)
#define ABS_ERROR_THRESH_KB	(5.0e-3)

#ifdef CONFIG_ARMV8_1_M_MVEF
/*
 * NOTE: The MVE vector version of the statistics functions are slightly less
 *       accurate than the scalar version.
 */
#undef REL_ERROR_THRESH
#define REL_ERROR_THRESH	(10.0e-3)

#undef SNR_ERROR_THRESH_KB
#define SNR_ERROR_THRESH_KB	((float32_t)39)
#endif

static void test_arm_max_f16(
	const uint16_t *input1, int ref_index, size_t length)
{
	float16_t val;
	uint32_t index;

	/* Run test function */
	arm_max_f16((float16_t *)input1, length, &val, &index);

	/* Validate output */
	zassert_equal(val, ((float16_t *)ref_max_val)[ref_index],
		ASSERT_MSG_INCORRECT_COMP_RESULT);

	zassert_equal(index, ref_max_idx[ref_index],
		ASSERT_MSG_INCORRECT_COMP_RESULT);
}

DEFINE_TEST_VARIANT3(statistics_f16, arm_max_f16, 7, in_com1, 0, 7);
DEFINE_TEST_VARIANT3(statistics_f16, arm_max_f16, 16, in_com1, 1, 16);
DEFINE_TEST_VARIANT3(statistics_f16, arm_max_f16, 23, in_com1, 2, 23);

static void test_arm_max_no_idx_f16(
	const uint16_t *input1, int ref_index, size_t length)
{
	float16_t val;

	/* Run test function */
	arm_max_no_idx_f16((float16_t *)input1, length, &val);

	/* Validate output */
	zassert_equal(val, ((float16_t *)ref_max_val)[ref_index],
		ASSERT_MSG_INCORRECT_COMP_RESULT);
}

DEFINE_TEST_VARIANT3(statistics_f16, arm_max_no_idx_f16, 7, in_com1, 0, 7);
DEFINE_TEST_VARIANT3(statistics_f16, arm_max_no_idx_f16, 16, in_com1, 1, 16);
DEFINE_TEST_VARIANT3(statistics_f16, arm_max_no_idx_f16, 23, in_com1, 2, 23);

static void test_arm_min_f16(
	const uint16_t *input1, int ref_index, size_t length)
{
	float16_t val;
	uint32_t index;

	/* Run test function */
	arm_min_f16((float16_t *)input1, length, &val, &index);

	/* Validate output */
	zassert_equal(val, ((float16_t *)ref_min_val)[ref_index],
		ASSERT_MSG_INCORRECT_COMP_RESULT);

	zassert_equal(index, ref_min_idx[ref_index],
		ASSERT_MSG_INCORRECT_COMP_RESULT);
}

DEFINE_TEST_VARIANT3(statistics_f16, arm_min_f16, 7, in_com1, 0, 7);
DEFINE_TEST_VARIANT3(statistics_f16, arm_min_f16, 16, in_com1, 1, 16);
DEFINE_TEST_VARIANT3(statistics_f16, arm_min_f16, 23, in_com1, 2, 23);

static void test_arm_absmax_f16(
	const uint16_t *input1, int ref_index, size_t length)
{
	float16_t val;
	uint32_t index;

	/* Run test function */
	arm_absmax_f16((float16_t *)input1, length, &val, &index);

	/* Validate output */
	zassert_equal(val, ((float16_t *)ref_absmax_val)[ref_index],
		ASSERT_MSG_INCORRECT_COMP_RESULT);

	zassert_equal(index, ref_absmax_idx[ref_index],
		ASSERT_MSG_INCORRECT_COMP_RESULT);
}

DEFINE_TEST_VARIANT3(statistics_f16, arm_absmax_f16, 7, in_absminmax, 0, 7);
DEFINE_TEST_VARIANT3(statistics_f16, arm_absmax_f16, 16, in_absminmax, 1, 16);
DEFINE_TEST_VARIANT3(statistics_f16, arm_absmax_f16, 23, in_absminmax, 2, 23);

static void test_arm_absmin_f16(
	const uint16_t *input1, int ref_index, size_t length)
{
	float16_t val;
	uint32_t index;

	/* Run test function */
	arm_absmin_f16((float16_t *)input1, length, &val, &index);

	/* Validate output */
	zassert_equal(val, ((float16_t *)ref_absmin_val)[ref_index],
		ASSERT_MSG_INCORRECT_COMP_RESULT);

	zassert_equal(index, ref_absmin_idx[ref_index],
		ASSERT_MSG_INCORRECT_COMP_RESULT);
}

DEFINE_TEST_VARIANT3(statistics_f16, arm_absmin_f16, 7, in_absminmax, 0, 7);
DEFINE_TEST_VARIANT3(statistics_f16, arm_absmin_f16, 16, in_absminmax, 1, 16);
DEFINE_TEST_VARIANT3(statistics_f16, arm_absmin_f16, 23, in_absminmax, 2, 23);

static void test_arm_mean_f16(
	const uint16_t *input1, int ref_index, size_t length)
{
	float16_t ref[1];
	float16_t *output;

	/* Load reference */
	ref[0] = ((float16_t *)ref_mean)[ref_index];

	/* Allocate output buffer */
	output = malloc(1 * sizeof(float16_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_mean_f16((float16_t *)input1, length, &output[0]);

	/* Validate output */
	zassert_true(
		test_snr_error_f16(1, output, ref, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_rel_error_f16(1, output, ref, REL_ERROR_THRESH),
		ASSERT_MSG_REL_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT3(statistics_f16, arm_mean_f16, 7, in_com2, 0, 7);
DEFINE_TEST_VARIANT3(statistics_f16, arm_mean_f16, 16, in_com2, 1, 16);
DEFINE_TEST_VARIANT3(statistics_f16, arm_mean_f16, 23, in_com2, 2, 23);

static void test_arm_power_f16(
	const uint16_t *input1, int ref_index, size_t length)
{
	float16_t ref[1];
	float16_t *output;

	/* Load reference */
	ref[0] = ((float16_t *)ref_power)[ref_index];

	/* Allocate output buffer */
	output = malloc(1 * sizeof(float16_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_power_f16((float16_t *)input1, length, &output[0]);

	/* Validate output */
	zassert_true(
		test_snr_error_f16(1, output, ref, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_rel_error_f16(1, output, ref, REL_ERROR_THRESH),
		ASSERT_MSG_REL_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT3(statistics_f16, arm_power_f16, 7, in_com1, 0, 7);
DEFINE_TEST_VARIANT3(statistics_f16, arm_power_f16, 16, in_com1, 1, 16);
DEFINE_TEST_VARIANT3(statistics_f16, arm_power_f16, 23, in_com1, 2, 23);

static void test_arm_rms_f16(
	const uint16_t *input1, int ref_index, size_t length)
{
	float16_t ref[1];
	float16_t *output;

	/* Load reference */
	ref[0] = ((float16_t *)ref_rms)[ref_index];

	/* Allocate output buffer */
	output = malloc(1 * sizeof(float16_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_rms_f16((float16_t *)input1, length, &output[0]);

	/* Validate output */
	zassert_true(
		test_snr_error_f16(1, output, ref, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_rel_error_f16(1, output, ref, REL_ERROR_THRESH),
		ASSERT_MSG_REL_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT3(statistics_f16, arm_rms_f16, 7, in_com1, 0, 7);
DEFINE_TEST_VARIANT3(statistics_f16, arm_rms_f16, 16, in_com1, 1, 16);
DEFINE_TEST_VARIANT3(statistics_f16, arm_rms_f16, 23, in_com1, 2, 23);

static void test_arm_std_f16(
	const uint16_t *input1, int ref_index, size_t length)
{
	float16_t ref[1];
	float16_t *output;

	/* Load reference */
	ref[0] = ((float16_t *)ref_std)[ref_index];

	/* Allocate output buffer */
	output = malloc(1 * sizeof(float16_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_std_f16((float16_t *)input1, length, &output[0]);

	/* Validate output */
	zassert_true(
		test_snr_error_f16(1, output, ref, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_rel_error_f16(1, output, ref, REL_ERROR_THRESH),
		ASSERT_MSG_REL_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT3(statistics_f16, arm_std_f16, 7, in_com1, 0, 7);
DEFINE_TEST_VARIANT3(statistics_f16, arm_std_f16, 16, in_com1, 1, 16);
DEFINE_TEST_VARIANT3(statistics_f16, arm_std_f16, 23, in_com1, 2, 23);

static void test_arm_var_f16(
	const uint16_t *input1, int ref_index, size_t length)
{
	float16_t ref[1];
	float16_t *output;

	/* Load reference */
	ref[0] = ((float16_t *)ref_var)[ref_index];

	/* Allocate output buffer */
	output = malloc(1 * sizeof(float16_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_var_f16((float16_t *)input1, length, &output[0]);

	/* Validate output */
	zassert_true(
		test_snr_error_f16(1, output, ref, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_rel_error_f16(1, output, ref, REL_ERROR_THRESH),
		ASSERT_MSG_REL_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT3(statistics_f16, arm_var_f16, 7, in_com1, 0, 7);
DEFINE_TEST_VARIANT3(statistics_f16, arm_var_f16, 16, in_com1, 1, 16);
DEFINE_TEST_VARIANT3(statistics_f16, arm_var_f16, 23, in_com1, 2, 23);

ZTEST(statistics_f16, test_arm_entropy_f16)
{
	size_t index;
	size_t length = in_entropy_dim[0];
	const float16_t *ref = (float16_t *)ref_entropy;
	const float16_t *input = (float16_t *)in_entropy;
	float16_t *output;

	__ASSERT_NO_MSG(ARRAY_SIZE(in_entropy_dim) > length);
	__ASSERT_NO_MSG(ARRAY_SIZE(ref_entropy) >= length);

	/* Allocate output buffer */
	output = malloc(length * sizeof(float16_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	for (index = 0; index < length; index++) {
		output[index] =
			arm_entropy_f16(input, in_entropy_dim[index + 1]);
		input += in_entropy_dim[index + 1];
	}

	/* Validate output */
	zassert_true(
		test_snr_error_f16(length, ref, output, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_near_equal_f16(length, ref, output, REL_ERROR_THRESH),
		ASSERT_MSG_REL_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

ZTEST(statistics_f16, test_arm_logsumexp_f16)
{
	size_t index;
	size_t length = in_logsumexp_dim[0];
	const float16_t *ref = (float16_t *)ref_logsumexp;
	const float16_t *input = (float16_t *)in_logsumexp;
	float16_t *output;

	__ASSERT_NO_MSG(ARRAY_SIZE(in_logsumexp_dim) > length);
	__ASSERT_NO_MSG(ARRAY_SIZE(ref_logsumexp) >= length);

	/* Allocate output buffer */
	output = malloc(length * sizeof(float16_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	for (index = 0; index < length; index++) {
		output[index] =
			arm_logsumexp_f16(input, in_logsumexp_dim[index + 1]);
		input += in_logsumexp_dim[index + 1];
	}

	/* Validate output */
	zassert_true(
		test_snr_error_f16(length, ref, output, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_near_equal_f16(length, ref, output, REL_ERROR_THRESH),
		ASSERT_MSG_REL_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

ZTEST(statistics_f16, test_arm_kullback_leibler_f16)
{
	size_t index;
	size_t length = in_kl_dim[0];
	const float16_t *ref = (float16_t *)ref_kl;
	const float16_t *input1 = (float16_t *)in_kl1;
	const float16_t *input2 = (float16_t *)in_kl2;
	float16_t *output;

	__ASSERT_NO_MSG(ARRAY_SIZE(in_kl_dim) > length);
	__ASSERT_NO_MSG(ARRAY_SIZE(ref_kl) >= length);

	/* Allocate output buffer */
	output = malloc(length * sizeof(float16_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	for (index = 0; index < length; index++) {
		output[index] =
			arm_kullback_leibler_f16(
				input1, input2, in_kl_dim[index + 1]);

		input1 += in_kl_dim[index + 1];
		input2 += in_kl_dim[index + 1];
	}

	/* Validate output */
	zassert_true(
		test_snr_error_f16(length, ref, output, SNR_ERROR_THRESH_KB),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_close_error_f16(length, ref, output,
			ABS_ERROR_THRESH_KB, REL_ERROR_THRESH_KB),
		ASSERT_MSG_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

ZTEST(statistics_f16, test_arm_logsumexp_dot_prod_f16)
{
	size_t index;
	size_t length = in_logsumexp_dp_dim[0];
	const float16_t *ref = (float16_t *)ref_logsumexp_dp;
	const float16_t *input1 = (float16_t *)in_logsumexp_dp1;
	const float16_t *input2 = (float16_t *)in_logsumexp_dp2;
	float16_t *output;
	float16_t *tmp;

	__ASSERT_NO_MSG(ARRAY_SIZE(in_logsumexp_dp_dim) > length);
	__ASSERT_NO_MSG(ARRAY_SIZE(ref_logsumexp_dp) >= length);

	/* Allocate buffers */
	output = malloc(length * sizeof(float16_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	tmp = malloc(12 * sizeof(float16_t));
	zassert_not_null(tmp, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	for (index = 0; index < length; index++) {
		output[index] =
			arm_logsumexp_dot_prod_f16(
				input1, input2,
				in_logsumexp_dp_dim[index + 1], tmp);

		input1 += in_logsumexp_dp_dim[index + 1];
		input2 += in_logsumexp_dp_dim[index + 1];
	}

	/* Validate output */
	zassert_true(
		test_snr_error_f16(length, ref, output, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_near_equal_f16(length, ref, output, REL_ERROR_THRESH),
		ASSERT_MSG_REL_ERROR_LIMIT_EXCEED);

	/* Free buffers */
	free(output);
	free(tmp);
}

ZTEST_SUITE(statistics_f16, NULL, NULL, NULL, NULL, NULL);
