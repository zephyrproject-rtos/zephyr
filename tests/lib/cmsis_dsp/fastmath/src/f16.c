/*
 * Copyright (c) 2021 Stephanos Ioannidis <root@stephanos.io>
 * Copyright (C) 2010-2021 ARM Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <zephyr/zephyr.h>
#include <stdlib.h>
#include <arm_math_f16.h>
#include "../../common/test_common.h"

#include "f16.pat"

#define SNR_ERROR_THRESH	((float32_t)60)
#define SNR_LOG_ERROR_THRESH	((float32_t)40)
#define REL_ERROR_THRESH	(1.0e-3)
#define REL_LOG_ERROR_THRESH	(3.0e-2)
#define ABS_ERROR_THRESH	(1.0e-3)
#define ABS_LOG_ERROR_THRESH	(3.0e-2)

#ifdef CONFIG_ARMV8_1_M_MVEF
/*
 * NOTE: The MVE vector version of the `vinverse` function is slightly less
 *       accurate than the scalar version.
 */
#undef REL_ERROR_THRESH
#define REL_ERROR_THRESH	(1.1e-3)
#endif

#if 0
/*
 * NOTE: These tests must be enabled once the F16 sine and cosine function
 *       implementations are added.
 */
static void test_arm_cos_f16(void)
{
	size_t index;
	size_t length = ARRAY_SIZE(in_angles);
	float16_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(float16_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	for (index = 0; index < length; index++) {
		output[index] = arm_cos_f16(((float16_t *)in_angles)[index]);
	}

	/* Validate output */
	zassert_true(
		test_snr_error_f16(length, output, (float16_t *)ref_cos,
			SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_close_error_f16(length, output, (float16_t *)ref_cos,
			ABS_ERROR_THRESH, REL_ERROR_THRESH),
		ASSERT_MSG_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

static void test_arm_sin_f16(void)
{
	size_t index;
	size_t length = ARRAY_SIZE(in_angles);
	float16_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(float16_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	for (index = 0; index < length; index++) {
		output[index] = arm_sin_f16(((float16_t *)in_angles)[index]);
	}

	/* Validate output */
	zassert_true(
		test_snr_error_f16(length, output, (float16_t *)ref_sin,
			SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_close_error_f16(length, output, (float16_t *)ref_sin,
			ABS_ERROR_THRESH, REL_ERROR_THRESH),
		ASSERT_MSG_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}
#endif

ZTEST_SUITE(fastmath_f16, NULL, NULL, NULL, NULL, NULL);

ZTEST(fastmath_f16, test_arm_sqrt_f16)
{
	size_t index;
	size_t length = ARRAY_SIZE(in_sqrt);
	arm_status status;
	float16_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(float16_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	for (index = 0; index < length; index++) {
		status = arm_sqrt_f16(
			((float16_t *)in_sqrt)[index], &output[index]);

		/* Validate operation status */
		if (((float16_t *)in_sqrt)[index] < 0.0f) {
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
		test_snr_error_f16(length, output, (float16_t *)ref_sqrt,
			SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_close_error_f16(length, output, (float16_t *)ref_sqrt,
			ABS_ERROR_THRESH, REL_ERROR_THRESH),
		ASSERT_MSG_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

static void test_arm_vlog_f16(
	const uint16_t *input1, const uint16_t *ref, size_t length)
{
	float16_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(float16_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_vlog_f16((float16_t *)input1, output, length);

	/* Validate output */
	zassert_true(
		test_snr_error_f16(length, output, (float16_t *)ref,
			SNR_LOG_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_close_error_f16(length, output, (float16_t *)ref,
			ABS_LOG_ERROR_THRESH, REL_LOG_ERROR_THRESH),
		ASSERT_MSG_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT3(fastmath_f16, arm_vlog_f16, all, in_log, ref_log, 25);
DEFINE_TEST_VARIANT3(fastmath_f16, arm_vlog_f16, 3, in_log, ref_log, 3);
DEFINE_TEST_VARIANT3(fastmath_f16, arm_vlog_f16, 8, in_log, ref_log, 8);
DEFINE_TEST_VARIANT3(fastmath_f16, arm_vlog_f16, 11, in_log, ref_log, 11);

static void test_arm_vexp_f16(
	const uint16_t *input1, const uint16_t *ref, size_t length)
{
	float16_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(float16_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_vexp_f16((float16_t *)input1, output, length);

	/* Validate output */
	zassert_true(
		test_snr_error_f16(length, output, (float16_t *)ref,
			SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_close_error_f16(length, output, (float16_t *)ref,
			ABS_ERROR_THRESH, REL_ERROR_THRESH),
		ASSERT_MSG_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT3(fastmath_f16, arm_vexp_f16, all, in_exp, ref_exp, 52);
DEFINE_TEST_VARIANT3(fastmath_f16, arm_vexp_f16, 3, in_exp, ref_exp, 3);
DEFINE_TEST_VARIANT3(fastmath_f16, arm_vexp_f16, 8, in_exp, ref_exp, 8);
DEFINE_TEST_VARIANT3(fastmath_f16, arm_vexp_f16, 11, in_exp, ref_exp, 11);

ZTEST(fastmath_f16, test_arm_vinverse_f16)
{
	size_t length = ARRAY_SIZE(ref_vinverse);
	float16_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(float16_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_vinverse_f16((float16_t *)in_vinverse, output, length);

	/* Validate output */
	zassert_true(
		test_snr_error_f16(length, output, (float16_t *)ref_vinverse,
			SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_close_error_f16(length, output, (float16_t *)ref_vinverse,
			ABS_ERROR_THRESH, REL_ERROR_THRESH),
		ASSERT_MSG_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

/* TODO: Add inverse test */
