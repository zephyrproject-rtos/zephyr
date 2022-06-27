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

#include "q7.pat"

#define SNR_ERROR_THRESH	((float32_t)20)
#define ABS_ERROR_THRESH	((q7_t)2)

void test_arm_linear_interp_q7(void)
{
	size_t index;
	size_t length = ARRAY_SIZE(ref_linear);
	q7_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(q7_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	for (index = 0; index < length; index++) {
		/*
		 * FIXME: Remove the cast below once the upstream CMSIS is
		 *        fixed to specify the const qualifier.
		 */
		output[index] = arm_linear_interp_q7((q7_t *)in_linear_y,
						     in_linear_x[index],
						     ARRAY_SIZE(in_linear_y));
	}

	/* Validate output */
	zassert_true(
		test_snr_error_q7(length, output, ref_linear,
			SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_near_equal_q7(length, output, ref_linear,
			ABS_ERROR_THRESH),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

void test_arm_bilinear_interp_q7(void)
{
	arm_bilinear_interp_instance_q7 inst;
	size_t index;
	size_t length = ARRAY_SIZE(ref_bilinear);
	q7_t *output;

	/* Initialise instance */
	inst.numRows = in_bilinear_config[1];
	inst.numCols = in_bilinear_config[0];
	inst.pData = (q7_t *)in_bilinear_y;

	/* Allocate output buffer */
	output = malloc(length * sizeof(q7_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	for (index = 0; index < length; index++) {
		output[index] = arm_bilinear_interp_q7(
					&inst,
					in_bilinear_x[2 * index],
					in_bilinear_x[2 * index + 1]
					);
	}

	/* Validate output */
	zassert_true(
		test_snr_error_q7(length, output, ref_bilinear,
			SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_near_equal_q7(length, output, ref_bilinear,
			ABS_ERROR_THRESH),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

void test_interpolation_q7(void)
{
	ztest_test_suite(interpolation_q7,
		ztest_unit_test(test_arm_linear_interp_q7),
		ztest_unit_test(test_arm_bilinear_interp_q7)
		);

	ztest_run_test_suite(interpolation_q7);
}
