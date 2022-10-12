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

#include "f16.pat"

#define SNR_ERROR_THRESH	((float32_t)62)
#define SNR_DOTPROD_THRESH	((float32_t)40)
#define REL_ERROR_THRESH	(4.0e-2)

static void test_zdsp_add_f16(
	const uint16_t *input1, const uint16_t *input2, const uint16_t *ref,
	size_t length)
{
	float16_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(float16_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	zdsp_add_f16((float16_t *)input1, (float16_t *)input2, output, length);

	/* Validate output */
	zassert_true(
		test_snr_error_f16(length, output, (float16_t *)ref,
			SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_rel_error_f16(length, output, (float16_t *)ref,
			REL_ERROR_THRESH),
		ASSERT_MSG_REL_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT4(basic_math_f16, zdsp_add_f16, 7, in_com1, in_com2, ref_add, 7);
DEFINE_TEST_VARIANT4(basic_math_f16, zdsp_add_f16, 16, in_com1, in_com2, ref_add, 16);
DEFINE_TEST_VARIANT4(basic_math_f16, zdsp_add_f16, 23, in_com1, in_com2, ref_add, 23);
DEFINE_TEST_VARIANT4(basic_math_f16, zdsp_add_f16, long, in_com1, in_com2, ref_add,
		     ARRAY_SIZE(in_com1));

static void test_zdsp_sub_f16(
	const uint16_t *input1, const uint16_t *input2, const uint16_t *ref,
	size_t length)
{
	float16_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(float16_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	zdsp_sub_f16((float16_t *)input1, (float16_t *)input2, output, length);

	/* Validate output */
	zassert_true(
		test_snr_error_f16(length, output, (float16_t *)ref,
			SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_rel_error_f16(length, output, (float16_t *)ref,
			REL_ERROR_THRESH),
		ASSERT_MSG_REL_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT4(basic_math_f16, zdsp_sub_f16, 7, in_com1, in_com2, ref_sub, 7);
DEFINE_TEST_VARIANT4(basic_math_f16, zdsp_sub_f16, 16, in_com1, in_com2, ref_sub, 16);
DEFINE_TEST_VARIANT4(basic_math_f16, zdsp_sub_f16, 23, in_com1, in_com2, ref_sub, 23);
DEFINE_TEST_VARIANT4(basic_math_f16, zdsp_sub_f16, long, in_com1, in_com2, ref_sub,
		     ARRAY_SIZE(in_com1));

static void test_zdsp_mult_f16(
	const uint16_t *input1, const uint16_t *input2, const uint16_t *ref,
	size_t length)
{
	float16_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(float16_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	zdsp_mult_f16((float16_t *)input1, (float16_t *)input2, output, length);

	/* Validate output */
	zassert_true(
		test_snr_error_f16(length, output, (float16_t *)ref,
			SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_rel_error_f16(length, output, (float16_t *)ref,
			REL_ERROR_THRESH),
		ASSERT_MSG_REL_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT4(basic_math_f16, zdsp_mult_f16, 7, in_com1, in_com2, ref_mult, 7);
DEFINE_TEST_VARIANT4(basic_math_f16, zdsp_mult_f16, 16, in_com1, in_com2, ref_mult, 16);
DEFINE_TEST_VARIANT4(basic_math_f16, zdsp_mult_f16, 23, in_com1, in_com2, ref_mult, 23);
DEFINE_TEST_VARIANT4(basic_math_f16, zdsp_mult_f16, long, in_com1, in_com2, ref_mult,
		     ARRAY_SIZE(in_com1));

static void test_zdsp_negate_f16(
	const uint16_t *input1, const uint16_t *ref, size_t length)
{
	float16_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(float16_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	zdsp_negate_f16((float16_t *)input1, output, length);

	/* Validate output */
	zassert_true(
		test_snr_error_f16(length, output, (float16_t *)ref,
			SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_rel_error_f16(length, output, (float16_t *)ref,
			REL_ERROR_THRESH),
		ASSERT_MSG_REL_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT3(basic_math_f16, zdsp_negate_f16, 7, in_com1, ref_negate, 7);
DEFINE_TEST_VARIANT3(basic_math_f16, zdsp_negate_f16, 16, in_com1, ref_negate, 16);
DEFINE_TEST_VARIANT3(basic_math_f16, zdsp_negate_f16, 23, in_com1, ref_negate, 23);
DEFINE_TEST_VARIANT3(basic_math_f16, zdsp_negate_f16, long, in_com1, ref_negate,
		     ARRAY_SIZE(in_com1));

static void test_zdsp_offset_f16(
	const uint16_t *input1, float16_t scalar, const uint16_t *ref,
	size_t length)
{
	float16_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(float16_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	zdsp_offset_f16((float16_t *)input1, scalar, output, length);

	/* Validate output */
	zassert_true(
		test_snr_error_f16(length, output, (float16_t *)ref,
			SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_rel_error_f16(length, output, (float16_t *)ref,
			REL_ERROR_THRESH),
		ASSERT_MSG_REL_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT4(basic_math_f16, zdsp_offset_f16, 0p5_7, in_com1, 0.5f, ref_offset, 7);
DEFINE_TEST_VARIANT4(basic_math_f16, zdsp_offset_f16, 0p5_16, in_com1, 0.5f, ref_offset, 16);
DEFINE_TEST_VARIANT4(basic_math_f16, zdsp_offset_f16, 0p5_23, in_com1, 0.5f, ref_offset, 23);
DEFINE_TEST_VARIANT4(basic_math_f16, zdsp_offset_f16, long, in_com1, 0.5f, ref_offset,
		     ARRAY_SIZE(in_com1));

static void test_zdsp_scale_f16(
	const uint16_t *input1, float16_t scalar, const uint16_t *ref,
	size_t length)
{
	float16_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(float16_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	zdsp_scale_f16((float16_t *)input1, scalar, output, length);

	/* Validate output */
	zassert_true(
		test_snr_error_f16(length, output, (float16_t *)ref,
			SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_rel_error_f16(length, output, (float16_t *)ref,
			REL_ERROR_THRESH),
		ASSERT_MSG_REL_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT4(basic_math_f16, zdsp_scale_f16, 0p5_7, in_com1, 0.5f, ref_scale, 7);
DEFINE_TEST_VARIANT4(basic_math_f16, zdsp_scale_f16, 0p5_16, in_com1, 0.5f, ref_scale, 16);
DEFINE_TEST_VARIANT4(basic_math_f16, zdsp_scale_f16, 0p5_23, in_com1, 0.5f, ref_scale, 23);
DEFINE_TEST_VARIANT4(basic_math_f16, zdsp_scale_f16, long, in_com1, 0.5f, ref_scale,
		     ARRAY_SIZE(in_com1));

static void test_zdsp_dot_prod_f16(
	const uint16_t *input1, const uint16_t *input2, const uint16_t *ref,
	size_t length)
{
	float16_t *output;

	/* Allocate output buffer */
	output = malloc(1 * sizeof(float16_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	zdsp_dot_prod_f16(
		(float16_t *)input1, (float16_t *)input2, length, &output[0]);

	/* Validate output */
	zassert_true(
		test_snr_error_f16(1, output, (float16_t *)ref,
			SNR_DOTPROD_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_rel_error_f16(1, output, (float16_t *)ref,
			REL_ERROR_THRESH),
		ASSERT_MSG_REL_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT4(basic_math_f16, zdsp_dot_prod_f16, 7, in_com1, in_com2, ref_dot_prod_3, 7);
DEFINE_TEST_VARIANT4(basic_math_f16, zdsp_dot_prod_f16, 16, in_com1, in_com2, ref_dot_prod_4, 16);
DEFINE_TEST_VARIANT4(basic_math_f16, zdsp_dot_prod_f16, 23, in_com1, in_com2, ref_dot_prod_4n1, 23);
DEFINE_TEST_VARIANT4(basic_math_f16, zdsp_dot_prod_f16, long, in_com1, in_com2, ref_dot_prod_long,
		     ARRAY_SIZE(in_com1));

static void test_zdsp_abs_f16(
	const uint16_t *input1, const uint16_t *ref, size_t length)
{
	float16_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(float16_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	zdsp_abs_f16((float16_t *)input1, output, length);

	/* Validate output */
	zassert_true(
		test_snr_error_f16(length, output, (float16_t *)ref,
			SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_rel_error_f16(length, output, (float16_t *)ref,
			REL_ERROR_THRESH),
		"incorrect computation result");

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT3(basic_math_f16, zdsp_abs_f16, 7, in_com1, ref_abs, 7);
DEFINE_TEST_VARIANT3(basic_math_f16, zdsp_abs_f16, 16, in_com1, ref_abs, 16);
DEFINE_TEST_VARIANT3(basic_math_f16, zdsp_abs_f16, 23, in_com1, ref_abs, 23);
DEFINE_TEST_VARIANT3(basic_math_f16, zdsp_abs_f16, long, in_com1, ref_abs, ARRAY_SIZE(in_com1));

static void test_zdsp_clip_f16(
	const uint16_t *input, const uint16_t *ref, float16_t min, float16_t max, size_t length)
{
	float16_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(float16_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	zdsp_clip_f16((float16_t *)input, output, min, max, length);

	/* Validate output */
	zassert_true(
		test_snr_error_f16(length, output, (float16_t *)ref,
			SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_rel_error_f16(length, output, (float16_t *)ref,
			REL_ERROR_THRESH),
		"incorrect computation result");

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT5(basic_math_f16, zdsp_clip_f16, m0p5_m0p1, in_clip, ref_clip1,
		     -0.5f, -0.1f, ARRAY_SIZE(ref_clip1));
DEFINE_TEST_VARIANT5(basic_math_f16, zdsp_clip_f16, m0p5_0p5, in_clip, ref_clip2,
		     -0.5f, 0.5f, ARRAY_SIZE(ref_clip2));
DEFINE_TEST_VARIANT5(basic_math_f16, zdsp_clip_f16, 0p1_0p5, in_clip, ref_clip3,
		     0.1f, 0.5f, ARRAY_SIZE(ref_clip3));

ZTEST_SUITE(basic_math_f16, NULL, NULL, NULL, NULL, NULL);
