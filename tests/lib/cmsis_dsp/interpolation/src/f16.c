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

#define SNR_ERROR_THRESH	((float16_t)55)
#define REL_ERROR_THRESH	(5.0e-3)
#define ABS_ERROR_THRESH	(5.0e-3)

ZTEST(interpolation_f16, test_arm_linear_interp_f16)
{
	arm_linear_interp_instance_f16 inst;
	size_t index;
	size_t length = ARRAY_SIZE(ref_linear);
	const float16_t *input = (const float16_t *)in_linear_x;
	float16_t *output;

	/* Initialise instance */
	inst.nValues = ARRAY_SIZE(in_linear_y);
	inst.x1 = 0.0;
	inst.xSpacing = 1.0;
	inst.pYData = (float16_t *)in_linear_y;

	/* Allocate output buffer */
	output = malloc(length * sizeof(float16_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	for (index = 0; index < length; index++) {
		output[index] = arm_linear_interp_f16(&inst,
						      input[index]);
	}

	/* Validate output */
	zassert_true(
		test_snr_error_f16(length, output, (float16_t *)ref_linear,
			SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_close_error_f16(length, output, (float16_t *)ref_linear,
			ABS_ERROR_THRESH, REL_ERROR_THRESH),
		ASSERT_MSG_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

ZTEST(interpolation_f16, test_arm_bilinear_interp_f16)
{
	arm_bilinear_interp_instance_f16 inst;
	size_t index;
	size_t length = ARRAY_SIZE(ref_bilinear);
	const float16_t *input = (const float16_t *)in_bilinear_x;
	float16_t *output;

	/* Initialise instance */
	inst.numRows = in_bilinear_config[1];
	inst.numCols = in_bilinear_config[0];
	inst.pData = (float16_t *)in_bilinear_y;

	/* Allocate output buffer */
	output = malloc(length * sizeof(float16_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	for (index = 0; index < length; index++) {
		output[index] = arm_bilinear_interp_f16(
					&inst,
					input[2 * index],
					input[2 * index + 1]
					);
	}

	/* Validate output */
	zassert_true(
		test_snr_error_f16(length, output, (float16_t *)ref_bilinear,
			SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_close_error_f16(length, output, (float16_t *)ref_bilinear,
			ABS_ERROR_THRESH, REL_ERROR_THRESH),
		ASSERT_MSG_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

ZTEST_SUITE(interpolation_f16, NULL, NULL, NULL, NULL, NULL);
