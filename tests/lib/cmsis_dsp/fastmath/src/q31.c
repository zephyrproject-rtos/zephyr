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

#include "q31.pat"

#define SNR_ERROR_THRESH	((float32_t)100)
#define ABS_ERROR_THRESH	((q31_t)2200)

static void test_arm_cos_q31(void)
{
	size_t index;
	size_t length = ARRAY_SIZE(in_angles);
	q31_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(q31_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	for (index = 0; index < length; index++) {
		output[index] = arm_cos_q31(in_angles[index]);
	}

	/* Validate output */
	zassert_true(
		test_snr_error_q31(length, output, ref_cos, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_near_equal_q31(length, output, ref_cos, ABS_ERROR_THRESH),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

static void test_arm_sin_q31(void)
{
	size_t index;
	size_t length = ARRAY_SIZE(in_angles);
	q31_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(q31_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	for (index = 0; index < length; index++) {
		output[index] = arm_sin_q31(in_angles[index]);
	}

	/* Validate output */
	zassert_true(
		test_snr_error_q31(length, output, ref_sin, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_near_equal_q31(length, output, ref_sin, ABS_ERROR_THRESH),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

static void test_arm_sqrt_q31(void)
{
	size_t index;
	size_t length = ARRAY_SIZE(in_sqrt);
	arm_status status;
	q31_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(q31_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	for (index = 0; index < length; index++) {
		status = arm_sqrt_q31(in_sqrt[index], &output[index]);

		/* Validate operation status */
		if (in_sqrt[index] <= 0) {
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
		test_snr_error_q31(length, output, ref_sqrt, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_near_equal_q31(length, output, ref_sqrt,
			ABS_ERROR_THRESH),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

void test_fastmath_q31(void)
{
	ztest_test_suite(fastmath_q31,
		ztest_unit_test(test_arm_cos_q31),
		ztest_unit_test(test_arm_sin_q31),
		ztest_unit_test(test_arm_sqrt_q31)
		);

	ztest_run_test_suite(fastmath_q31);
}
