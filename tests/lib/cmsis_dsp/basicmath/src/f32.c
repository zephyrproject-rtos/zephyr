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

#include "f32.pat"

#define SNR_ERROR_THRESH	((float32_t)120)
#define REL_ERROR_THRESH	(5.0e-5)

static void test_arm_add_f32(
	const uint32_t *input1, const uint32_t *input2, const uint32_t *ref,
	size_t length)
{
	float32_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(float32_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_add_f32((float32_t *)input1, (float32_t *)input2, output, length);

	/* Validate output */
	zassert_true(
		test_snr_error_f32(length, output, (float32_t *)ref,
			SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_rel_error_f32(length, output, (float32_t *)ref,
			REL_ERROR_THRESH),
		ASSERT_MSG_REL_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT4(arm_add_f32, 3, in_com1, in_com2, ref_add, 3);
DEFINE_TEST_VARIANT4(arm_add_f32, 8, in_com1, in_com2, ref_add, 8);
DEFINE_TEST_VARIANT4(arm_add_f32, 11, in_com1, in_com2, ref_add, 11);

static void test_arm_sub_f32(
	const uint32_t *input1, const uint32_t *input2, const uint32_t *ref,
	size_t length)
{
	float32_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(float32_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_sub_f32((float32_t *)input1, (float32_t *)input2, output, length);

	/* Validate output */
	zassert_true(
		test_snr_error_f32(length, output, (float32_t *)ref,
			SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_rel_error_f32(length, output, (float32_t *)ref,
			REL_ERROR_THRESH),
		ASSERT_MSG_REL_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT4(arm_sub_f32, 3, in_com1, in_com2, ref_sub, 3);
DEFINE_TEST_VARIANT4(arm_sub_f32, 8, in_com1, in_com2, ref_sub, 8);
DEFINE_TEST_VARIANT4(arm_sub_f32, 11, in_com1, in_com2, ref_sub, 11);

static void test_arm_mult_f32(
	const uint32_t *input1, const uint32_t *input2, const uint32_t *ref,
	size_t length)
{
	float32_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(float32_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_mult_f32((float32_t *)input1, (float32_t *)input2, output, length);

	/* Validate output */
	zassert_true(
		test_snr_error_f32(length, output, (float32_t *)ref,
			SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_rel_error_f32(length, output, (float32_t *)ref,
			REL_ERROR_THRESH),
		ASSERT_MSG_REL_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT4(arm_mult_f32, 3, in_com1, in_com2, ref_mult, 3);
DEFINE_TEST_VARIANT4(arm_mult_f32, 8, in_com1, in_com2, ref_mult, 8);
DEFINE_TEST_VARIANT4(arm_mult_f32, 11, in_com1, in_com2, ref_mult, 11);

static void test_arm_negate_f32(
	const uint32_t *input1, const uint32_t *ref, size_t length)
{
	float32_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(float32_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_negate_f32((float32_t *)input1, output, length);

	/* Validate output */
	zassert_true(
		test_snr_error_f32(length, output, (float32_t *)ref,
			SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_rel_error_f32(length, output, (float32_t *)ref,
			REL_ERROR_THRESH),
		ASSERT_MSG_REL_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT3(arm_negate_f32, 3, in_com1, ref_negate, 3);
DEFINE_TEST_VARIANT3(arm_negate_f32, 8, in_com1, ref_negate, 8);
DEFINE_TEST_VARIANT3(arm_negate_f32, 11, in_com1, ref_negate, 11);

static void test_arm_offset_f32(
	const uint32_t *input1, float32_t scalar, const uint32_t *ref,
	size_t length)
{
	float32_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(float32_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_offset_f32((float32_t *)input1, scalar, output, length);

	/* Validate output */
	zassert_true(
		test_snr_error_f32(length, output, (float32_t *)ref,
			SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_rel_error_f32(length, output, (float32_t *)ref,
			REL_ERROR_THRESH),
		ASSERT_MSG_REL_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT4(arm_offset_f32, 0p5_3, in_com1, 0.5f, ref_offset, 3);
DEFINE_TEST_VARIANT4(arm_offset_f32, 0p5_8, in_com1, 0.5f, ref_offset, 8);
DEFINE_TEST_VARIANT4(arm_offset_f32, 0p5_11, in_com1, 0.5f, ref_offset, 11);

static void test_arm_scale_f32(
	const uint32_t *input1, float32_t scalar, const uint32_t *ref,
	size_t length)
{
	float32_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(float32_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_scale_f32((float32_t *)input1, scalar, output, length);

	/* Validate output */
	zassert_true(
		test_snr_error_f32(length, output, (float32_t *)ref,
			SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_rel_error_f32(length, output, (float32_t *)ref,
			REL_ERROR_THRESH),
		ASSERT_MSG_REL_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT4(arm_scale_f32, 0p5_3, in_com1, 0.5f, ref_scale, 3);
DEFINE_TEST_VARIANT4(arm_scale_f32, 0p5_8, in_com1, 0.5f, ref_scale, 8);
DEFINE_TEST_VARIANT4(arm_scale_f32, 0p5_11, in_com1, 0.5f, ref_scale, 11);

static void test_arm_dot_prod_f32(
	const uint32_t *input1, const uint32_t *input2, const uint32_t *ref,
	size_t length)
{
	float32_t *output;

	/* Allocate output buffer */
	output = malloc(1 * sizeof(float32_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_dot_prod_f32(
		(float32_t *)input1, (float32_t *)input2, length, &output[0]);

	/* Validate output */
	zassert_true(
		test_snr_error_f32(1, output, (float32_t *)ref,
			SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_rel_error_f32(1, output, (float32_t *)ref,
			REL_ERROR_THRESH),
		ASSERT_MSG_REL_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT4(arm_dot_prod_f32, 3, in_com1, in_com2, ref_dot_prod_3, 3);
DEFINE_TEST_VARIANT4(arm_dot_prod_f32, 8, in_com1, in_com2, ref_dot_prod_4, 8);
DEFINE_TEST_VARIANT4(arm_dot_prod_f32, 11, in_com1, in_com2, ref_dot_prod_4n1, 11);

static void test_arm_abs_f32(
	const uint32_t *input1, const uint32_t *ref, size_t length)
{
	float32_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(float32_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_abs_f32((float32_t *)input1, output, length);

	/* Validate output */
	zassert_true(
		test_snr_error_f32(length, output, (float32_t *)ref,
			SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_rel_error_f32(length, output, (float32_t *)ref,
			REL_ERROR_THRESH),
		"incorrect computation result");

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT3(arm_abs_f32, 3, in_com1, ref_abs, 3);
DEFINE_TEST_VARIANT3(arm_abs_f32, 8, in_com1, ref_abs, 8);
DEFINE_TEST_VARIANT3(arm_abs_f32, 11, in_com1, ref_abs, 11);

void test_basicmath_f32(void)
{
	ztest_test_suite(basicmath_f32,
		ztest_unit_test(test_arm_add_f32_3),
		ztest_unit_test(test_arm_add_f32_8),
		ztest_unit_test(test_arm_add_f32_11),
		ztest_unit_test(test_arm_sub_f32_3),
		ztest_unit_test(test_arm_sub_f32_8),
		ztest_unit_test(test_arm_sub_f32_11),
		ztest_unit_test(test_arm_mult_f32_3),
		ztest_unit_test(test_arm_mult_f32_8),
		ztest_unit_test(test_arm_mult_f32_11),
		ztest_unit_test(test_arm_negate_f32_3),
		ztest_unit_test(test_arm_negate_f32_8),
		ztest_unit_test(test_arm_negate_f32_11),
		ztest_unit_test(test_arm_offset_f32_0p5_3),
		ztest_unit_test(test_arm_offset_f32_0p5_8),
		ztest_unit_test(test_arm_offset_f32_0p5_11),
		ztest_unit_test(test_arm_scale_f32_0p5_3),
		ztest_unit_test(test_arm_scale_f32_0p5_8),
		ztest_unit_test(test_arm_scale_f32_0p5_11),
		ztest_unit_test(test_arm_dot_prod_f32_3),
		ztest_unit_test(test_arm_dot_prod_f32_8),
		ztest_unit_test(test_arm_dot_prod_f32_11),
		ztest_unit_test(test_arm_abs_f32_3),
		ztest_unit_test(test_arm_abs_f32_8),
		ztest_unit_test(test_arm_abs_f32_11)
		);

	ztest_run_test_suite(basicmath_f32);
}
