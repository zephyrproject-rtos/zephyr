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

#include "misc_f16.pat"

#define SNR_ERROR_THRESH	((float32_t)60)
#define REL_ERROR_THRESH	(1.0e-4)
#define ABS_ERROR_THRESH	(1.0e-3)
#define SNR_ERROR_THRESH_LD	((float32_t)52)
#define REL_ERROR_THRESH_LD	(1.0e-3)
#define ABS_ERROR_THRESH_LD	(1.0e-3)

static void test_arm_correlate_f16(
	size_t in1_length, size_t in2_length, const uint16_t *ref,
	size_t ref_length)
{
	float16_t *output;

	/* Allocate output buffer */
	output = calloc(ref_length, sizeof(float16_t));

	/* Run test function */
	arm_correlate_f16(
		(float16_t *)in_com1, in1_length,
		(float16_t *)in_com2, in2_length, output);

	/* Validate output */
	zassert_true(
		test_snr_error_f16(ref_length, (float16_t *)ref, output,
			SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_close_error_f16(ref_length, (float16_t *)ref, output,
			ABS_ERROR_THRESH, REL_ERROR_THRESH),
		ASSERT_MSG_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

#define DEFINE_CORRELATE_TEST(a, b) \
	DEFINE_TEST_VARIANT4(filtering_misc_f16, \
		arm_correlate_f16, a##_##b, a, b, \
		ref_correlate_##a##_##b, ARRAY_SIZE(ref_correlate_##a##_##b))

DEFINE_CORRELATE_TEST(4, 1);
DEFINE_CORRELATE_TEST(4, 2);
DEFINE_CORRELATE_TEST(4, 3);
DEFINE_CORRELATE_TEST(4, 8);
DEFINE_CORRELATE_TEST(4, 11);
DEFINE_CORRELATE_TEST(5, 1);
DEFINE_CORRELATE_TEST(5, 2);
DEFINE_CORRELATE_TEST(5, 3);
DEFINE_CORRELATE_TEST(5, 8);
DEFINE_CORRELATE_TEST(5, 11);
DEFINE_CORRELATE_TEST(6, 1);
DEFINE_CORRELATE_TEST(6, 2);
DEFINE_CORRELATE_TEST(6, 3);
DEFINE_CORRELATE_TEST(6, 8);
DEFINE_CORRELATE_TEST(6, 11);
DEFINE_CORRELATE_TEST(9, 1);
DEFINE_CORRELATE_TEST(9, 2);
DEFINE_CORRELATE_TEST(9, 3);
DEFINE_CORRELATE_TEST(9, 8);
DEFINE_CORRELATE_TEST(9, 11);
DEFINE_CORRELATE_TEST(10, 1);
DEFINE_CORRELATE_TEST(10, 2);
DEFINE_CORRELATE_TEST(10, 3);
DEFINE_CORRELATE_TEST(10, 8);
DEFINE_CORRELATE_TEST(10, 11);
DEFINE_CORRELATE_TEST(11, 1);
DEFINE_CORRELATE_TEST(11, 2);
DEFINE_CORRELATE_TEST(11, 3);
DEFINE_CORRELATE_TEST(11, 8);
DEFINE_CORRELATE_TEST(11, 11);
DEFINE_CORRELATE_TEST(12, 1);
DEFINE_CORRELATE_TEST(12, 2);
DEFINE_CORRELATE_TEST(12, 3);
DEFINE_CORRELATE_TEST(12, 8);
DEFINE_CORRELATE_TEST(12, 11);
DEFINE_CORRELATE_TEST(13, 1);
DEFINE_CORRELATE_TEST(13, 2);
DEFINE_CORRELATE_TEST(13, 3);
DEFINE_CORRELATE_TEST(13, 8);
DEFINE_CORRELATE_TEST(13, 11);

#if 0
/*
 * NOTE: These tests must be enabled once the arm_conv_f16 implementation is
 *       added.
 */
static void test_arm_conv_f16(
	size_t in1_length, size_t in2_length, const uint16_t *ref,
	size_t ref_length)
{
	float16_t *output;

	/* Allocate output buffer */
	output = calloc(ref_length, sizeof(float16_t));

	/* Run test function */
	arm_conv_f16(
		(float16_t *)in_com1, in1_length,
		(float16_t *)in_com2, in2_length, output);

	/* Validate output */
	zassert_true(
		test_snr_error_f16(ref_length, (float16_t *)ref, output,
			SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_close_error_f16(ref_length, (float16_t *)ref, output,
			ABS_ERROR_THRESH, REL_ERROR_THRESH),
		ASSERT_MSG_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

#define DEFINE_CONV_TEST(a, b) \
	DEFINE_TEST_VARIANT4(filtering_misc_f16, \
		arm_conv_f16, a##_##b, a, b, \
		ref_conv_##a##_##b, ARRAY_SIZE(ref_conv_##a##_##b))

DEFINE_CONV_TEST(4, 1);
DEFINE_CONV_TEST(4, 2);
DEFINE_CONV_TEST(4, 3);
DEFINE_CONV_TEST(4, 8);
DEFINE_CONV_TEST(4, 11);
DEFINE_CONV_TEST(5, 1);
DEFINE_CONV_TEST(5, 2);
DEFINE_CONV_TEST(5, 3);
DEFINE_CONV_TEST(5, 8);
DEFINE_CONV_TEST(5, 11);
DEFINE_CONV_TEST(6, 1);
DEFINE_CONV_TEST(6, 2);
DEFINE_CONV_TEST(6, 3);
DEFINE_CONV_TEST(6, 8);
DEFINE_CONV_TEST(6, 11);
DEFINE_CONV_TEST(9, 1);
DEFINE_CONV_TEST(9, 2);
DEFINE_CONV_TEST(9, 3);
DEFINE_CONV_TEST(9, 8);
DEFINE_CONV_TEST(9, 11);
DEFINE_CONV_TEST(10, 1);
DEFINE_CONV_TEST(10, 2);
DEFINE_CONV_TEST(10, 3);
DEFINE_CONV_TEST(10, 8);
DEFINE_CONV_TEST(10, 11);
DEFINE_CONV_TEST(11, 1);
DEFINE_CONV_TEST(11, 2);
DEFINE_CONV_TEST(11, 3);
DEFINE_CONV_TEST(11, 8);
DEFINE_CONV_TEST(11, 11);
DEFINE_CONV_TEST(12, 1);
DEFINE_CONV_TEST(12, 2);
DEFINE_CONV_TEST(12, 3);
DEFINE_CONV_TEST(12, 8);
DEFINE_CONV_TEST(12, 11);
DEFINE_CONV_TEST(13, 1);
DEFINE_CONV_TEST(13, 2);
DEFINE_CONV_TEST(13, 3);
DEFINE_CONV_TEST(13, 8);
DEFINE_CONV_TEST(13, 11);
#endif

static void test_arm_levinson_durbin_f16(
	size_t in_length, size_t err_index, const uint16_t *in,
	const uint16_t *ref, size_t ref_length)
{
	float16_t *output;
	float16_t err;

	/* Allocate output buffer */
	output = calloc(ref_length, sizeof(float16_t));

	/* Run test function */
	arm_levinson_durbin_f16((const float16_t *)in, output, &err,
				in_length);

	/* Validate output */
	zassert_true(
		test_snr_error_f16(ref_length, (const float16_t *)ref, output,
			SNR_ERROR_THRESH_LD),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_close_error_f16(ref_length, (const float16_t *)ref,
			output, ABS_ERROR_THRESH_LD, REL_ERROR_THRESH_LD),
		ASSERT_MSG_ERROR_LIMIT_EXCEED);

	zassert_true(
		test_close_error_f16(1,
			(const float16_t *)&in_levinson_durbin_err[err_index],
			&(err), ABS_ERROR_THRESH_LD, REL_ERROR_THRESH_LD),
		ASSERT_MSG_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

#define DEFINE_LEVINSON_DURBIN_TEST(a, b) \
	DEFINE_TEST_VARIANT5(filtering_misc_f16, \
		arm_levinson_durbin_f16, a##_##b, a, b, \
		in_levinson_durbin_##a##_##b, \
		ref_levinson_durbin_##a##_##b, \
		ARRAY_SIZE(ref_levinson_durbin_##a##_##b))

DEFINE_LEVINSON_DURBIN_TEST(7, 0);
DEFINE_LEVINSON_DURBIN_TEST(16, 1);
DEFINE_LEVINSON_DURBIN_TEST(23, 2);

ZTEST_SUITE(filtering_misc_f16, NULL, NULL, NULL, NULL, NULL);
