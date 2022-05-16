/*
 * Copyright (c) 2020 Stephanos Ioannidis <root@stephanos.io>
 * Copyright (C) 2010-2020 ARM Limited or its affiliates. All rights reserved.
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
#define REL_ERROR_THRESH	(1.0e-6)
#define ABS_ERROR_THRESH	(1.0e-5)

ZTEST_SUITE(fastmath_f32, NULL, NULL, NULL, NULL, NULL);

ZTEST(fastmath_f32, test_arm_cos_f32)
{
	size_t index;
	size_t length = ARRAY_SIZE(in_angles);
	float32_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(float32_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	for (index = 0; index < length; index++) {
		output[index] = arm_cos_f32(((float32_t *)in_angles)[index]);
	}

	/* Validate output */
	zassert_true(
		test_snr_error_f32(length, output, (float32_t *)ref_cos,
			SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_close_error_f32(length, output, (float32_t *)ref_cos,
			ABS_ERROR_THRESH, REL_ERROR_THRESH),
		ASSERT_MSG_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

ZTEST(fastmath_f32, test_arm_sin_f32)
{
	size_t index;
	size_t length = ARRAY_SIZE(in_angles);
	float32_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(float32_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	for (index = 0; index < length; index++) {
		output[index] = arm_sin_f32(((float32_t *)in_angles)[index]);
	}

	/* Validate output */
	zassert_true(
		test_snr_error_f32(length, output, (float32_t *)ref_sin,
			SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_close_error_f32(length, output, (float32_t *)ref_sin,
			ABS_ERROR_THRESH, REL_ERROR_THRESH),
		ASSERT_MSG_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

ZTEST(fastmath_f32, test_arm_sqrt_f32)
{
	size_t index;
	size_t length = ARRAY_SIZE(in_sqrt);
	arm_status status;
	float32_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(float32_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	for (index = 0; index < length; index++) {
		status = arm_sqrt_f32(
			((float32_t *)in_sqrt)[index], &output[index]);

		/* Validate operation status */
		if (((float32_t *)in_sqrt)[index] < 0.0f) {
			zassert_equal(status, ARM_MATH_ARGUMENT_ERROR,
				"square root did fail with an input value "
				"of '0'");
		} else {
			zassert_equal(status, ARM_MATH_SUCCESS,
				"square root operation did not succeed");
		}
	}

	/* Validate output */
	zassert_true(
		test_snr_error_f32(length, output, (float32_t *)ref_sqrt,
			SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_close_error_f32(length, output, (float32_t *)ref_sqrt,
			ABS_ERROR_THRESH, REL_ERROR_THRESH),
		ASSERT_MSG_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

static void test_arm_vlog_f32(
	const uint32_t *input1, const uint32_t *ref, size_t length)
{
	float32_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(float32_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_vlog_f32((float32_t *)input1, output, length);

	/* Validate output */
	zassert_true(
		test_snr_error_f32(length, output, (float32_t *)ref,
			SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_close_error_f32(length, output, (float32_t *)ref,
			ABS_ERROR_THRESH, REL_ERROR_THRESH),
		ASSERT_MSG_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT3(fastmath_f32, arm_vlog_f32, all, in_log, ref_log, 25);
DEFINE_TEST_VARIANT3(fastmath_f32, arm_vlog_f32, 3, in_log, ref_log, 3);
DEFINE_TEST_VARIANT3(fastmath_f32, arm_vlog_f32, 8, in_log, ref_log, 8);
DEFINE_TEST_VARIANT3(fastmath_f32, arm_vlog_f32, 11, in_log, ref_log, 11);

static void test_arm_vexp_f32(
	const uint32_t *input1, const uint32_t *ref, size_t length)
{
	float32_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(float32_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_vexp_f32((float32_t *)input1, output, length);

	/* Validate output */
	zassert_true(
		test_snr_error_f32(length, output, (float32_t *)ref,
			SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_close_error_f32(length, output, (float32_t *)ref,
			ABS_ERROR_THRESH, REL_ERROR_THRESH),
		ASSERT_MSG_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT3(fastmath_f32, arm_vexp_f32, all, in_exp, ref_exp, 52);
DEFINE_TEST_VARIANT3(fastmath_f32, arm_vexp_f32, 3, in_exp, ref_exp, 3);
DEFINE_TEST_VARIANT3(fastmath_f32, arm_vexp_f32, 8, in_exp, ref_exp, 8);
DEFINE_TEST_VARIANT3(fastmath_f32, arm_vexp_f32, 11, in_exp, ref_exp, 11);
