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

#include "f32.pat"

#define SNR_ERROR_THRESH	((float32_t)120)
#define REL_ERROR_THRESH	(8.0e-5)

void test_arm_linear_interp_f32(void)
{
	arm_linear_interp_instance_f32 inst;
	size_t index;
	size_t length = ARRAY_SIZE(ref_linear);
	const float32_t *input = (const float32_t *)in_linear_x;
	float32_t *output;

	/* Initialise instance */
	inst.nValues = ARRAY_SIZE(in_linear_y);
	inst.x1 = 0.0;
	inst.xSpacing = 1.0;
	inst.pYData = (float32_t *)in_linear_y;

	/* Allocate output buffer */
	output = malloc(length * sizeof(float32_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	for (index = 0; index < length; index++) {
		output[index] = arm_linear_interp_f32(&inst,
						      input[index]);
	}

	/* Validate output */
	zassert_true(
		test_snr_error_f32(length, output, (float32_t *)ref_linear,
			SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_rel_error_f32(length, output, (float32_t *)ref_linear,
			REL_ERROR_THRESH),
		ASSERT_MSG_REL_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

void test_arm_bilinear_interp_f32(void)
{
	arm_bilinear_interp_instance_f32 inst;
	size_t index;
	size_t length = ARRAY_SIZE(ref_bilinear);
	const float32_t *input = (const float32_t *)in_bilinear_x;
	float32_t *output;

	/* Initialise instance */
	inst.numRows = in_bilinear_config[1];
	inst.numCols = in_bilinear_config[0];
	inst.pData = (float32_t *)in_bilinear_y;

	/* Allocate output buffer */
	output = malloc(length * sizeof(float32_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	for (index = 0; index < length; index++) {
		output[index] = arm_bilinear_interp_f32(
					&inst,
					input[2 * index],
					input[2 * index + 1]
					);
	}

	/* Validate output */
	zassert_true(
		test_snr_error_f32(length, output, (float32_t *)ref_bilinear,
			SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_rel_error_f32(length, output, (float32_t *)ref_bilinear,
			REL_ERROR_THRESH),
		ASSERT_MSG_REL_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

static void test_arm_spline(
	const uint32_t *input_x, const uint32_t *input_y,
	const uint32_t *input_xq, const uint32_t *ref, size_t length,
	uint32_t n, arm_spline_type type)
{
	float32_t *output;
	float32_t *scratch;
	float32_t *coeff;
	arm_spline_instance_f32 inst;

	/* Allocate output buffer */
	output = malloc(length * sizeof(float32_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Allocate scratch buffer */
	scratch = malloc(((n * 2) - 1) * sizeof(float32_t));
	zassert_not_null(scratch, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Allocate coefficient buffer */
	coeff = malloc(((n - 1) * 3) * sizeof(float32_t));
	zassert_not_null(coeff, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Initialise spline */
	arm_spline_init_f32(&inst, type,
		(float32_t *)input_x, (float32_t *)input_y, n, coeff, scratch);

	/* Run test function */
	arm_spline_f32(&inst, (float32_t *)input_xq, output, length);

	/* Validate output */
	zassert_true(
		test_snr_error_f32(length, output, (float32_t *)ref,
			SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
	free(scratch);
	free(coeff);
}

DEFINE_TEST_VARIANT7(interpolation_f32, arm_spline, square_20,
	in_spline_squ_x, in_spline_squ_y, in_spline_squ_xq, ref_spline_squ, 20,
	4, ARM_SPLINE_PARABOLIC_RUNOUT);

DEFINE_TEST_VARIANT7(interpolation_f32, arm_spline, sine_33,
	in_spline_sin_x, in_spline_sin_y, in_spline_sin_xq, ref_spline_sin, 33,
	9, ARM_SPLINE_NATURAL);

DEFINE_TEST_VARIANT7(interpolation_f32, arm_spline, ramp_30,
	in_spline_ram_x, in_spline_ram_y, in_spline_ram_xq, ref_spline_ram, 30,
	3, ARM_SPLINE_PARABOLIC_RUNOUT);

ZTEST_SUITE(interpolation_f32, NULL, NULL, NULL, NULL, NULL);
