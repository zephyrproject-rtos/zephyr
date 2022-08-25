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

#define SNR_ERROR_THRESH	((float32_t)39)
#define REL_ERROR_THRESH	(6.0e-2)

ZTEST_SUITE(complexmath_f16, NULL, NULL, NULL, NULL, NULL);

static void test_arm_cmplx_conj_f16(
	const uint16_t *input1, const uint16_t *ref, size_t length)
{
	size_t buf_length;
	float16_t *output;

	/* Complex number buffer length is twice the data length */
	buf_length = 2 * length;

	/* Allocate output buffer */
	output = malloc(buf_length * sizeof(float16_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_cmplx_conj_f16((float16_t *)input1, output, length);

	/* Validate output */
	zassert_true(
		test_snr_error_f16(buf_length, output, (float16_t *)ref,
			SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_rel_error_f16(buf_length, output, (float16_t *)ref,
			REL_ERROR_THRESH),
		ASSERT_MSG_REL_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT3(complexmath_f16, arm_cmplx_conj_f16, 7, in_com1, ref_conj, 7);
DEFINE_TEST_VARIANT3(complexmath_f16, arm_cmplx_conj_f16, 16, in_com1, ref_conj, 16);
DEFINE_TEST_VARIANT3(complexmath_f16, arm_cmplx_conj_f16, 23, in_com1, ref_conj, 23);

static void test_arm_cmplx_dot_prod_f16(
	const uint16_t *input1, const uint16_t *input2, const uint16_t *ref,
	size_t length)
{
	float16_t *output;

	/* Allocate output buffer */
	output = malloc(2 * sizeof(float16_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_cmplx_dot_prod_f16(
		(float16_t *)input1, (float16_t *)input2, length,
		&output[0], &output[1]);

	/* Validate output */
	zassert_true(
		test_snr_error_f16(2, output, (float16_t *)ref,
			SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_rel_error_f16(2, output, (float16_t *)ref,
			REL_ERROR_THRESH),
		ASSERT_MSG_REL_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT4(complexmath_f16, arm_cmplx_dot_prod_f16, 7, in_com1, in_com2, ref_dot_prod_3,
		     7);
DEFINE_TEST_VARIANT4(complexmath_f16, arm_cmplx_dot_prod_f16, 16, in_com1, in_com2, ref_dot_prod_4n,
		     16);
DEFINE_TEST_VARIANT4(complexmath_f16, arm_cmplx_dot_prod_f16, 23, in_com1, in_com2,
		     ref_dot_prod_4n1, 23);

static void test_arm_cmplx_mag_f16(
	const uint16_t *input1, const uint16_t *ref, size_t length)
{
	float16_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(float16_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_cmplx_mag_f16((float16_t *)input1, output, length);

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

DEFINE_TEST_VARIANT3(complexmath_f16, arm_cmplx_mag_f16, 7, in_com1, ref_mag, 7);
DEFINE_TEST_VARIANT3(complexmath_f16, arm_cmplx_mag_f16, 16, in_com1, ref_mag, 16);
DEFINE_TEST_VARIANT3(complexmath_f16, arm_cmplx_mag_f16, 23, in_com1, ref_mag, 23);

static void test_arm_cmplx_mag_squared_f16(
	const uint16_t *input1, const uint16_t *ref, size_t length)
{
	float16_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(float16_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_cmplx_mag_squared_f16((float16_t *)input1, output, length);

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

DEFINE_TEST_VARIANT3(complexmath_f16, arm_cmplx_mag_squared_f16, 7, in_com1, ref_mag_squared, 7);
DEFINE_TEST_VARIANT3(complexmath_f16, arm_cmplx_mag_squared_f16, 16, in_com1, ref_mag_squared, 16);
DEFINE_TEST_VARIANT3(complexmath_f16, arm_cmplx_mag_squared_f16, 23, in_com1, ref_mag_squared, 23);

static void test_arm_cmplx_mult_cmplx_f16(
	const uint16_t *input1, const uint16_t *input2, const uint16_t *ref,
	size_t length)
{
	size_t buf_length;
	float16_t *output;

	/* Complex number buffer length is twice the data length */
	buf_length = 2 * length;

	/* Allocate output buffer */
	output = malloc(buf_length * sizeof(float16_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_cmplx_mult_cmplx_f16(
		(float16_t *)input1, (float16_t *)input2, output, length);

	/* Validate output */
	zassert_true(
		test_snr_error_f16(buf_length, output, (float16_t *)ref,
			SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_rel_error_f16(buf_length, output, (float16_t *)ref,
			REL_ERROR_THRESH),
		ASSERT_MSG_REL_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT4(complexmath_f16, arm_cmplx_mult_cmplx_f16, 7, in_com1, in_com2, ref_mult_cmplx,
		     7);
DEFINE_TEST_VARIANT4(complexmath_f16, arm_cmplx_mult_cmplx_f16, 16, in_com1, in_com2,
		     ref_mult_cmplx, 16);
DEFINE_TEST_VARIANT4(complexmath_f16, arm_cmplx_mult_cmplx_f16, 23, in_com1, in_com2,
		     ref_mult_cmplx, 23);

static void test_arm_cmplx_mult_real_f16(
	const uint16_t *input1, const uint16_t *input2, const uint16_t *ref,
	size_t length)
{
	size_t buf_length;
	float16_t *output;

	/* Complex number buffer length is twice the data length */
	buf_length = 2 * length;

	/* Allocate output buffer */
	output = malloc(buf_length * sizeof(float16_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_cmplx_mult_real_f16(
		(float16_t *)input1, (float16_t *)input2, output, length);

	/* Validate output */
	zassert_true(
		test_snr_error_f16(
			buf_length, output, (float16_t *)ref,
			SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_rel_error_f16(
			buf_length, output, (float16_t *)ref,
			REL_ERROR_THRESH),
		ASSERT_MSG_REL_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT4(complexmath_f16, arm_cmplx_mult_real_f16, 7, in_com1, in_com3, ref_mult_real,
		     7);
DEFINE_TEST_VARIANT4(complexmath_f16, arm_cmplx_mult_real_f16, 16, in_com1, in_com3, ref_mult_real,
		     16);
DEFINE_TEST_VARIANT4(complexmath_f16, arm_cmplx_mult_real_f16, 23, in_com1, in_com3, ref_mult_real,
		     23);
