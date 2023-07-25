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

#include "q15.pat"

#define SNR_ERROR_THRESH	((float32_t)70)
#define ABS_ERROR_THRESH	((q15_t)10)

ZTEST_SUITE(fastmath_q15, NULL, NULL, NULL, NULL, NULL);

ZTEST(fastmath_q15, test_arm_cos_q15)
{
	size_t index;
	size_t length = ARRAY_SIZE(in_angles);
	q15_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(q15_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	for (index = 0; index < length; index++) {
		output[index] = arm_cos_q15(in_angles[index]);
	}

	/* Validate output */
	zassert_true(
		test_snr_error_q15(length, output, ref_cos, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_near_equal_q15(length, output, ref_cos, ABS_ERROR_THRESH),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

ZTEST(fastmath_q15, test_arm_sin_q15)
{
	size_t index;
	size_t length = ARRAY_SIZE(in_angles);
	q15_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(q15_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	for (index = 0; index < length; index++) {
		output[index] = arm_sin_q15(in_angles[index]);
	}

	/* Validate output */
	zassert_true(
		test_snr_error_q15(length, output, ref_sin, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_near_equal_q15(length, output, ref_sin, ABS_ERROR_THRESH),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

ZTEST(fastmath_q15, test_arm_sqrt_q15)
{
	size_t index;
	size_t length = ARRAY_SIZE(in_sqrt);
	arm_status status;
	q15_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(q15_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	for (index = 0; index < length; index++) {
		status = arm_sqrt_q15(in_sqrt[index], &output[index]);

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
		test_snr_error_q15(length, output, ref_sqrt, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_near_equal_q15(length, output, ref_sqrt,
			ABS_ERROR_THRESH),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

ZTEST(fastmath_q15, test_arm_divide_q15)
{
	size_t index;
	size_t length = ARRAY_SIZE(ref_divide);
	arm_status status;
	q15_t *output;
	uint16_t *shift;

	/* Allocate output buffer */
	output = malloc(length * sizeof(q15_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	shift = malloc(length * sizeof(uint16_t));
	zassert_not_null(shift, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	for (index = 0; index < length; index++) {
		status = arm_divide_q15(
				in_divide_num[index], in_divide_den[index],
				&output[index], &shift[index]);
	}

	/* Validate output */
	zassert_true(
		test_snr_error_q15(length, output, ref_divide, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_near_equal_q15(length, output, ref_divide,
			ABS_ERROR_THRESH),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	zassert_true(
		memcmp(shift, ref_divide_shift,
		       length * sizeof(uint16_t)) == 0,
		ASSERT_MSG_INCORRECT_COMP_RESULT);

	/* Free output buffer */
	free(output);
	free(shift);
}
