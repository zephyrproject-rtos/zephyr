/*
 * Copyright (c) 2020 Stephanos Ioannidis <root@stephanos.io>
 * Copyright (C) 2010-2020 ARM Limited or its affiliates. All rights reserved.
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
#define REL_ERROR_THRESH	(7.0e-6)

ZTEST_SUITE(complexmath_f32, NULL, NULL, NULL, NULL, NULL);

static void test_arm_cmplx_conj_f32(
	const uint32_t *input1, const uint32_t *ref, size_t length)
{
	size_t buf_length;
	float32_t *output;

	/* Complex number buffer length is twice the data length */
	buf_length = 2 * length;

	/* Allocate output buffer */
	output = malloc(buf_length * sizeof(float32_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_cmplx_conj_f32((float32_t *)input1, output, length);

	/* Validate output */
	zassert_true(
		test_snr_error_f32(buf_length, output, (float32_t *)ref,
			SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_rel_error_f32(buf_length, output, (float32_t *)ref,
			REL_ERROR_THRESH),
		ASSERT_MSG_REL_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT3(complexmath_f32, arm_cmplx_conj_f32, 3, in_com1, ref_conj, 3);
DEFINE_TEST_VARIANT3(complexmath_f32, arm_cmplx_conj_f32, 8, in_com1, ref_conj, 8);
DEFINE_TEST_VARIANT3(complexmath_f32, arm_cmplx_conj_f32, 11, in_com1, ref_conj, 11);

static void test_arm_cmplx_dot_prod_f32(
	const uint32_t *input1, const uint32_t *input2, const uint32_t *ref,
	size_t length)
{
	float32_t *output;

	/* Allocate output buffer */
	output = malloc(2 * sizeof(float32_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_cmplx_dot_prod_f32(
		(float32_t *)input1, (float32_t *)input2, length,
		&output[0], &output[1]);

	/* Validate output */
	zassert_true(
		test_snr_error_f32(2, output, (float32_t *)ref,
			SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_rel_error_f32(2, output, (float32_t *)ref,
			REL_ERROR_THRESH),
		ASSERT_MSG_REL_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT4(complexmath_f32, arm_cmplx_dot_prod_f32, 3, in_com1, in_com2, ref_dot_prod_3,
		     3);
DEFINE_TEST_VARIANT4(complexmath_f32, arm_cmplx_dot_prod_f32, 8, in_com1, in_com2, ref_dot_prod_4n,
		     8);
DEFINE_TEST_VARIANT4(complexmath_f32, arm_cmplx_dot_prod_f32, 11, in_com1, in_com2,
		     ref_dot_prod_4n1, 11);

static void test_arm_cmplx_mag_f32(
	const uint32_t *input1, const uint32_t *ref, size_t length)
{
	float32_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(float32_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_cmplx_mag_f32((float32_t *)input1, output, length);

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

DEFINE_TEST_VARIANT3(complexmath_f32, arm_cmplx_mag_f32, 3, in_com1, ref_mag, 3);
DEFINE_TEST_VARIANT3(complexmath_f32, arm_cmplx_mag_f32, 8, in_com1, ref_mag, 8);
DEFINE_TEST_VARIANT3(complexmath_f32, arm_cmplx_mag_f32, 11, in_com1, ref_mag, 11);

static void test_arm_cmplx_mag_squared_f32(
	const uint32_t *input1, const uint32_t *ref, size_t length)
{
	float32_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(float32_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_cmplx_mag_squared_f32((float32_t *)input1, output, length);

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

DEFINE_TEST_VARIANT3(complexmath_f32, arm_cmplx_mag_squared_f32, 3, in_com1, ref_mag_squared, 3);
DEFINE_TEST_VARIANT3(complexmath_f32, arm_cmplx_mag_squared_f32, 8, in_com1, ref_mag_squared, 8);
DEFINE_TEST_VARIANT3(complexmath_f32, arm_cmplx_mag_squared_f32, 11, in_com1, ref_mag_squared, 11);

static void test_arm_cmplx_mult_cmplx_f32(
	const uint32_t *input1, const uint32_t *input2, const uint32_t *ref,
	size_t length)
{
	size_t buf_length;
	float32_t *output;

	/* Complex number buffer length is twice the data length */
	buf_length = 2 * length;

	/* Allocate output buffer */
	output = malloc(buf_length * sizeof(float32_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_cmplx_mult_cmplx_f32(
		(float32_t *)input1, (float32_t *)input2, output, length);

	/* Validate output */
	zassert_true(
		test_snr_error_f32(buf_length, output, (float32_t *)ref,
			SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_rel_error_f32(buf_length, output, (float32_t *)ref,
			REL_ERROR_THRESH),
		ASSERT_MSG_REL_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT4(complexmath_f32, arm_cmplx_mult_cmplx_f32, 3, in_com1, in_com2, ref_mult_cmplx,
		     3);
DEFINE_TEST_VARIANT4(complexmath_f32, arm_cmplx_mult_cmplx_f32, 8, in_com1, in_com2, ref_mult_cmplx,
		     8);
DEFINE_TEST_VARIANT4(complexmath_f32, arm_cmplx_mult_cmplx_f32, 11, in_com1, in_com2,
		     ref_mult_cmplx, 11);

static void test_arm_cmplx_mult_real_f32(
	const uint32_t *input1, const uint32_t *input2, const uint32_t *ref,
	size_t length)
{
	size_t buf_length;
	float32_t *output;

	/* Complex number buffer length is twice the data length */
	buf_length = 2 * length;

	/* Allocate output buffer */
	output = malloc(buf_length * sizeof(float32_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_cmplx_mult_real_f32(
		(float32_t *)input1, (float32_t *)input2, output, length);

	/* Validate output */
	zassert_true(
		test_snr_error_f32(
			buf_length, output, (float32_t *)ref,
			SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_rel_error_f32(
			buf_length, output, (float32_t *)ref,
			REL_ERROR_THRESH),
		ASSERT_MSG_REL_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT4(complexmath_f32, arm_cmplx_mult_real_f32, 3, in_com1, in_com3, ref_mult_real,
		     3);
DEFINE_TEST_VARIANT4(complexmath_f32, arm_cmplx_mult_real_f32, 8, in_com1, in_com3, ref_mult_real,
		     8);
DEFINE_TEST_VARIANT4(complexmath_f32, arm_cmplx_mult_real_f32, 11, in_com1, in_com3, ref_mult_real,
		     11);
