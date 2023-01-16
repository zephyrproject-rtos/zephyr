/*
 * Copyright (c) 2021 Stephanos Ioannidis <root@stephanos.io>
 * Copyright (C) 2010-2021 ARM Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/dsp/dsp.h>
#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <stdlib.h>
#include "common/test_common.h"

#include "f32.pat"

#define SNR_ERROR_THRESH	((float32_t)120)
#define REL_ERROR_THRESH	(5.0e-5)

static void test_zdsp_add_f32(
	const uint32_t *input1, const uint32_t *input2, const uint32_t *ref,
	size_t length)
{
	float32_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(float32_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	zdsp_add_f32((float32_t *)input1, (float32_t *)input2, output, length);

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

DEFINE_TEST_VARIANT4(basic_math_f32, zdsp_add_f32, 3, in_com1, in_com2, ref_add, 3);
DEFINE_TEST_VARIANT4(basic_math_f32, zdsp_add_f32, 8, in_com1, in_com2, ref_add, 8);
DEFINE_TEST_VARIANT4(basic_math_f32, zdsp_add_f32, 11, in_com1, in_com2, ref_add, 11);
DEFINE_TEST_VARIANT4(basic_math_f32, zdsp_add_f32, long, in_com1, in_com2, ref_add,
		     ARRAY_SIZE(in_com1));

static void test_zdsp_sub_f32(
	const uint32_t *input1, const uint32_t *input2, const uint32_t *ref,
	size_t length)
{
	float32_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(float32_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	zdsp_sub_f32((float32_t *)input1, (float32_t *)input2, output, length);

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

DEFINE_TEST_VARIANT4(basic_math_f32, zdsp_sub_f32, 3, in_com1, in_com2, ref_sub, 3);
DEFINE_TEST_VARIANT4(basic_math_f32, zdsp_sub_f32, 8, in_com1, in_com2, ref_sub, 8);
DEFINE_TEST_VARIANT4(basic_math_f32, zdsp_sub_f32, 11, in_com1, in_com2, ref_sub, 11);
DEFINE_TEST_VARIANT4(basic_math_f32, zdsp_sub_f32, long, in_com1, in_com2, ref_sub,
		     ARRAY_SIZE(in_com1));

static void test_zdsp_mult_f32(
	const uint32_t *input1, const uint32_t *input2, const uint32_t *ref,
	size_t length)
{
	float32_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(float32_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	zdsp_mult_f32((float32_t *)input1, (float32_t *)input2, output, length);

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

DEFINE_TEST_VARIANT4(basic_math_f32, zdsp_mult_f32, 3, in_com1, in_com2, ref_mult, 3);
DEFINE_TEST_VARIANT4(basic_math_f32, zdsp_mult_f32, 8, in_com1, in_com2, ref_mult, 8);
DEFINE_TEST_VARIANT4(basic_math_f32, zdsp_mult_f32, 11, in_com1, in_com2, ref_mult, 11);
DEFINE_TEST_VARIANT4(basic_math_f32, zdsp_mult_f32, long, in_com1, in_com2, ref_mult,
		     ARRAY_SIZE(in_com1));

static void test_zdsp_negate_f32(
	const uint32_t *input1, const uint32_t *ref, size_t length)
{
	float32_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(float32_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	zdsp_negate_f32((float32_t *)input1, output, length);

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

DEFINE_TEST_VARIANT3(basic_math_f32, zdsp_negate_f32, 3, in_com1, ref_negate, 3);
DEFINE_TEST_VARIANT3(basic_math_f32, zdsp_negate_f32, 8, in_com1, ref_negate, 8);
DEFINE_TEST_VARIANT3(basic_math_f32, zdsp_negate_f32, 11, in_com1, ref_negate, 11);
DEFINE_TEST_VARIANT3(basic_math_f32, zdsp_negate_f32, long, in_com1, ref_negate,
		     ARRAY_SIZE(in_com1));

static void test_zdsp_offset_f32(
	const uint32_t *input1, float32_t scalar, const uint32_t *ref,
	size_t length)
{
	float32_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(float32_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	zdsp_offset_f32((float32_t *)input1, scalar, output, length);

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

DEFINE_TEST_VARIANT4(basic_math_f32, zdsp_offset_f32, 0p5_3, in_com1, 0.5f, ref_offset, 3);
DEFINE_TEST_VARIANT4(basic_math_f32, zdsp_offset_f32, 0p5_8, in_com1, 0.5f, ref_offset, 8);
DEFINE_TEST_VARIANT4(basic_math_f32, zdsp_offset_f32, 0p5_11, in_com1, 0.5f, ref_offset, 11);
DEFINE_TEST_VARIANT4(basic_math_f32, zdsp_offset_f32, long, in_com1, 0.5f, ref_offset,
		     ARRAY_SIZE(in_com1));

static void test_zdsp_scale_f32(
	const uint32_t *input1, float32_t scalar, const uint32_t *ref,
	size_t length)
{
	float32_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(float32_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	zdsp_scale_f32((float32_t *)input1, scalar, output, length);

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

DEFINE_TEST_VARIANT4(basic_math_f32, zdsp_scale_f32, 0p5_3, in_com1, 0.5f, ref_scale, 3);
DEFINE_TEST_VARIANT4(basic_math_f32, zdsp_scale_f32, 0p5_8, in_com1, 0.5f, ref_scale, 8);
DEFINE_TEST_VARIANT4(basic_math_f32, zdsp_scale_f32, 0p5_11, in_com1, 0.5f, ref_scale, 11);
DEFINE_TEST_VARIANT4(basic_math_f32, zdsp_scale_f32, long, in_com1, 0.5f, ref_scale,
		     ARRAY_SIZE(in_com1));

static void test_zdsp_dot_prod_f32(
	const uint32_t *input1, const uint32_t *input2, const uint32_t *ref,
	size_t length)
{
	float32_t *output;

	/* Allocate output buffer */
	output = malloc(1 * sizeof(float32_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	zdsp_dot_prod_f32(
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

DEFINE_TEST_VARIANT4(basic_math_f32, zdsp_dot_prod_f32, 3, in_com1, in_com2, ref_dot_prod_3, 3);
DEFINE_TEST_VARIANT4(basic_math_f32, zdsp_dot_prod_f32, 8, in_com1, in_com2, ref_dot_prod_4, 8);
DEFINE_TEST_VARIANT4(basic_math_f32, zdsp_dot_prod_f32, 11, in_com1, in_com2, ref_dot_prod_4n1, 11);
DEFINE_TEST_VARIANT4(basic_math_f32, zdsp_dot_prod_f32, long, in_com1, in_com2, ref_dot_prod_long,
		     ARRAY_SIZE(in_com1));

static void test_zdsp_abs_f32(
	const uint32_t *input1, const uint32_t *ref, size_t length)
{
	float32_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(float32_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	zdsp_abs_f32((float32_t *)input1, output, length);

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

DEFINE_TEST_VARIANT3(basic_math_f32, zdsp_abs_f32, 3, in_com1, ref_abs, 3);
DEFINE_TEST_VARIANT3(basic_math_f32, zdsp_abs_f32, 8, in_com1, ref_abs, 8);
DEFINE_TEST_VARIANT3(basic_math_f32, zdsp_abs_f32, 11, in_com1, ref_abs, 11);
DEFINE_TEST_VARIANT3(basic_math_f32, zdsp_abs_f32, long, in_com1, ref_abs, ARRAY_SIZE(in_com1));

static void test_zdsp_clip_f32(
	const uint32_t *input, const uint32_t *ref, float32_t min, float32_t max, size_t length)
{
	float32_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(float32_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	zdsp_clip_f32((float32_t *)input, output, min, max, length);

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

DEFINE_TEST_VARIANT5(basic_math_f32, zdsp_clip_f32, m0p5_m0p1, in_clip, ref_clip1,
		     -0.5f, -0.1f, ARRAY_SIZE(ref_clip1));
DEFINE_TEST_VARIANT5(basic_math_f32, zdsp_clip_f32, m0p5_0p5, in_clip, ref_clip2,
		     -0.5f, 0.5f, ARRAY_SIZE(ref_clip2));
DEFINE_TEST_VARIANT5(basic_math_f32, zdsp_clip_f32, 0p1_0p5, in_clip, ref_clip3,
		     0.1f, 0.5f, ARRAY_SIZE(ref_clip3));

ZTEST_SUITE(basic_math_f32, NULL, NULL, NULL, NULL, NULL);
