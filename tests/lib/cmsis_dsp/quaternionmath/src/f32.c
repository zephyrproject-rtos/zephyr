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
#define REL_ERROR_THRESH	(1.0e-6)
#define ABS_ERROR_THRESH	(1.0e-7)

ZTEST(quaternionmath_f32, test_arm_quaternion_norm_f32)
{
	size_t length = ARRAY_SIZE(ref_norm);
	const float32_t *input = (const float32_t *)in_com1;
	const float32_t *ref = (const float32_t *)ref_norm;
	float32_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(float32_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_quaternion_norm_f32(input, output, length);

	/* Validate output */
	zassert_true(
		test_snr_error_f32(length, output, ref, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_close_error_f32(length, output, ref,
			ABS_ERROR_THRESH, REL_ERROR_THRESH),
		ASSERT_MSG_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

ZTEST(quaternionmath_f32, test_arm_quaternion_inverse_f32)
{
	size_t length = ARRAY_SIZE(ref_inv);
	const float32_t *input = (const float32_t *)in_com1;
	const float32_t *ref = (const float32_t *)ref_inv;
	float32_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(float32_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_quaternion_inverse_f32(input, output, length / 4);

	/* Validate output */
	zassert_true(
		test_snr_error_f32(length, output, ref, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_close_error_f32(length, output, ref,
			ABS_ERROR_THRESH, REL_ERROR_THRESH),
		ASSERT_MSG_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

ZTEST(quaternionmath_f32, test_arm_quaternion_conjugate_f32)
{
	size_t length = ARRAY_SIZE(ref_conj);
	const float32_t *input = (const float32_t *)in_com1;
	const float32_t *ref = (const float32_t *)ref_conj;
	float32_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(float32_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_quaternion_conjugate_f32(input, output, length / 4);

	/* Validate output */
	zassert_true(
		test_snr_error_f32(length, output, ref, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_close_error_f32(length, output, ref,
			ABS_ERROR_THRESH, REL_ERROR_THRESH),
		ASSERT_MSG_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

ZTEST(quaternionmath_f32, test_arm_quaternion_normalize_f32)
{
	size_t length = ARRAY_SIZE(ref_normalize);
	const float32_t *input = (const float32_t *)in_com1;
	const float32_t *ref = (const float32_t *)ref_normalize;
	float32_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(float32_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_quaternion_normalize_f32(input, output, length / 4);

	/* Validate output */
	zassert_true(
		test_snr_error_f32(length, output, ref, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_close_error_f32(length, output, ref,
			ABS_ERROR_THRESH, REL_ERROR_THRESH),
		ASSERT_MSG_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

ZTEST(quaternionmath_f32, test_arm_quaternion_product_single_f32)
{
	size_t index;
	size_t length = ARRAY_SIZE(ref_mult);
	const float32_t *input1 = (const float32_t *)in_com1;
	const float32_t *input2 = (const float32_t *)in_com2;
	const float32_t *ref = (const float32_t *)ref_mult;
	float32_t *output, *output_buf;

	/* Allocate output buffer */
	output_buf = malloc(length * sizeof(float32_t));
	zassert_not_null(output_buf, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	output = output_buf;

	/* Run test function */
	for (index = 0; index < (length / 4); index++) {
		arm_quaternion_product_single_f32(input1, input2, output);

		output += 4;
		input1 += 4;
		input2 += 4;
	}

	/* Validate output */
	zassert_true(
		test_snr_error_f32(length, output_buf, ref, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_close_error_f32(length, output_buf, ref,
			ABS_ERROR_THRESH, REL_ERROR_THRESH),
		ASSERT_MSG_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output_buf);
}

ZTEST(quaternionmath_f32, test_arm_quaternion_product_f32)
{
	size_t length = ARRAY_SIZE(ref_mult);
	const float32_t *input1 = (const float32_t *)in_com1;
	const float32_t *input2 = (const float32_t *)in_com2;
	const float32_t *ref = (const float32_t *)ref_mult;
	float32_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(float32_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_quaternion_product_f32(input1, input2, output, length / 4);

	/* Validate output */
	zassert_true(
		test_snr_error_f32(length, output, ref, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_close_error_f32(length, output, ref,
			ABS_ERROR_THRESH, REL_ERROR_THRESH),
		ASSERT_MSG_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

ZTEST(quaternionmath_f32, test_arm_quaternion2rotation_f32)
{
	size_t length = ARRAY_SIZE(ref_quat2rot);
	const float32_t *input = (const float32_t *)in_com1;
	const float32_t *ref = (const float32_t *)ref_quat2rot;
	float32_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(float32_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_quaternion2rotation_f32(input, output, ARRAY_SIZE(in_com1) / 4);

	/* Validate output */
	zassert_true(
		test_snr_error_f32(length, output, ref, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_close_error_f32(length, output, ref,
			ABS_ERROR_THRESH, REL_ERROR_THRESH),
		ASSERT_MSG_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

ZTEST(quaternionmath_f32, test_arm_rotation2quaternion_f32)
{
	size_t length = ARRAY_SIZE(ref_rot2quat);
	const float32_t *input = (const float32_t *)in_rot;
	const float32_t *ref = (const float32_t *)ref_rot2quat;
	float32_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(float32_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_rotation2quaternion_f32(input, output, ARRAY_SIZE(in_com1) / 4);

	/* Remove ambiguity */
	for (float32_t *ptr = output; ptr < (output + length); ptr += 4) {
		if (ptr[0] < 0.0f) {
			ptr[0] = -ptr[0];
			ptr[1] = -ptr[1];
			ptr[2] = -ptr[2];
			ptr[3] = -ptr[3];
		}
	}

	/* Validate output */
	zassert_true(
		test_snr_error_f32(length, output, ref, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_close_error_f32(length, output, ref,
			ABS_ERROR_THRESH, REL_ERROR_THRESH),
		ASSERT_MSG_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

ZTEST_SUITE(quaternionmath_f32, NULL, NULL, NULL, NULL, NULL);
