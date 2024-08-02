/*
 * Copyright (c) 2020 Stephanos Ioannidis <root@stephanos.io>
 * Copyright (C) 2010-2020 ARM Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <stdlib.h>
#include <arm_math.h>
#include "../../common/test_common.h"

#include "q15.pat"

#define SNR_ERROR_THRESH	((float32_t)25)
#define SNR_ERROR_THRESH_HIGH	((float32_t)60)
#define ABS_ERROR_THRESH_Q15	((q15_t)50)
#define ABS_ERROR_THRESH_Q31	((q31_t)(1 << 15))

ZTEST_SUITE(complexmath_q15, NULL, NULL, NULL, NULL, NULL);

static void test_arm_cmplx_conj_q15(
	const q15_t *input1, const q15_t *ref, size_t length)
{
	size_t buf_length;
	q15_t *output;

	/* Complex number buffer length is twice the data length */
	buf_length = 2 * length;

	/* Allocate output buffer */
	output = malloc(buf_length * sizeof(q15_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_cmplx_conj_q15(input1, output, length);

	/* Validate output */
	zassert_true(
		test_snr_error_q15(buf_length, output, ref, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_near_equal_q15(buf_length, output, ref,
			ABS_ERROR_THRESH_Q15),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT3(complexmath_q15, arm_cmplx_conj_q15, 7, in_com1, ref_conj, 7);
DEFINE_TEST_VARIANT3(complexmath_q15, arm_cmplx_conj_q15, 16, in_com1, ref_conj, 16);
DEFINE_TEST_VARIANT3(complexmath_q15, arm_cmplx_conj_q15, 23, in_com1, ref_conj, 23);

static void test_arm_cmplx_dot_prod_q15(
	const q15_t *input1, const q15_t *input2, const q31_t *ref,
	size_t length)
{
	q31_t *output;

	/* Allocate output buffer */
	output = malloc(2 * sizeof(q31_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_cmplx_dot_prod_q15(input1, input2, length, &output[0], &output[1]);

	/* Validate output */
	zassert_true(
		test_snr_error_q31(2, output, ref, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_near_equal_q31(2, output, ref, ABS_ERROR_THRESH_Q31),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT4(complexmath_q15, arm_cmplx_dot_prod_q15, 7, in_com1, in_com2, ref_dot_prod_3,
		     7);
DEFINE_TEST_VARIANT4(complexmath_q15, arm_cmplx_dot_prod_q15, 16, in_com1, in_com2, ref_dot_prod_4n,
		     16);
DEFINE_TEST_VARIANT4(complexmath_q15, arm_cmplx_dot_prod_q15, 23, in_com1, in_com2,
		     ref_dot_prod_4n1, 23);

static void test_arm_cmplx_mag_q15(
	const q15_t *input1, const q15_t *ref, size_t length)
{
	q15_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(q15_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_cmplx_mag_q15(input1, output, length);

	/* Validate output */
	zassert_true(
		test_snr_error_q15(length, output, ref, SNR_ERROR_THRESH_HIGH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_near_equal_q15(length, output, ref, ABS_ERROR_THRESH_Q15),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT3(complexmath_q15, arm_cmplx_mag_q15, 7, in_com1, ref_mag, 7);
DEFINE_TEST_VARIANT3(complexmath_q15, arm_cmplx_mag_q15, 16, in_com1, ref_mag, 16);
DEFINE_TEST_VARIANT3(complexmath_q15, arm_cmplx_mag_q15, 23, in_com1, ref_mag, 23);

static void test_arm_cmplx_mag_squared_q15(
	const q15_t *input1, const q15_t *ref, size_t length)
{
	q15_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(q15_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_cmplx_mag_squared_q15(input1, output, length);

	/* Validate output */
	zassert_true(
		test_snr_error_q15(length, output, ref, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_near_equal_q15(length, output, ref, ABS_ERROR_THRESH_Q15),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT3(complexmath_q15, arm_cmplx_mag_squared_q15, 7, in_com1, ref_mag_squared, 7);
DEFINE_TEST_VARIANT3(complexmath_q15, arm_cmplx_mag_squared_q15, 16, in_com1, ref_mag_squared, 16);
DEFINE_TEST_VARIANT3(complexmath_q15, arm_cmplx_mag_squared_q15, 23, in_com1, ref_mag_squared, 23);

static void test_arm_cmplx_mult_cmplx_q15(
	const q15_t *input1, const q15_t *input2, const q15_t *ref,
	size_t length)
{
	size_t buf_length;
	q15_t *output;

	/* Complex number buffer length is twice the data length */
	buf_length = 2 * length;

	/* Allocate output buffer */
	output = malloc(buf_length * sizeof(q15_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_cmplx_mult_cmplx_q15(input1, input2, output, length);

	/* Validate output */
	zassert_true(
		test_snr_error_q15(buf_length, output, ref, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_near_equal_q15(buf_length, output, ref,
			ABS_ERROR_THRESH_Q15),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT4(complexmath_q15, arm_cmplx_mult_cmplx_q15, 7, in_com1, in_com2, ref_mult_cmplx,
		     7);
DEFINE_TEST_VARIANT4(complexmath_q15, arm_cmplx_mult_cmplx_q15, 16, in_com1, in_com2,
		     ref_mult_cmplx, 16);
DEFINE_TEST_VARIANT4(complexmath_q15, arm_cmplx_mult_cmplx_q15, 23, in_com1, in_com2,
		     ref_mult_cmplx, 23);

static void test_arm_cmplx_mult_real_q15(
	const q15_t *input1, const q15_t *input2, const q15_t *ref,
	size_t length)
{
	size_t buf_length;
	q15_t *output;

	/* Complex number buffer length is twice the data length */
	buf_length = 2 * length;

	/* Allocate output buffer */
	output = malloc(buf_length * sizeof(q15_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	arm_cmplx_mult_real_q15(input1, input2, output, length);

	/* Validate output */
	zassert_true(
		test_snr_error_q15(buf_length, output, ref, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_near_equal_q15(buf_length, output, ref,
			ABS_ERROR_THRESH_Q15),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT4(complexmath_q15, arm_cmplx_mult_real_q15, 7, in_com1, in_com3, ref_mult_real,
		     7);
DEFINE_TEST_VARIANT4(complexmath_q15, arm_cmplx_mult_real_q15, 16, in_com1, in_com3, ref_mult_real,
		     16);
DEFINE_TEST_VARIANT4(complexmath_q15, arm_cmplx_mult_real_q15, 23, in_com1, in_com3, ref_mult_real,
		     23);
