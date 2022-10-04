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

#include "f64.pat"

#define SNR_ERROR_THRESH	((float64_t)300)
#define REL_ERROR_THRESH	(1.0e-14)

static void test_arm_entropy_f64(void)
{
	size_t index;
	size_t length = in_entropy_dim[0];
	const float64_t *ref = (float64_t *)ref_entropy;
	const float64_t *input = (float64_t *)in_entropy;
	float64_t *output;

	__ASSERT_NO_MSG(ARRAY_SIZE(in_entropy_dim) > length);
	__ASSERT_NO_MSG(ARRAY_SIZE(ref_entropy) >= length);

	/* Allocate output buffer */
	output = malloc(length * sizeof(float64_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	for (index = 0; index < length; index++) {
		output[index] =
			arm_entropy_f64(input, in_entropy_dim[index + 1]);
		input += in_entropy_dim[index + 1];
	}

	/* Validate output */
	zassert_true(
		test_snr_error_f64(length, ref, output, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_near_equal_f64(length, ref, output, REL_ERROR_THRESH),
		ASSERT_MSG_REL_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

static void test_arm_kullback_leibler_f64(void)
{
	size_t index;
	size_t length = in_kl_dim[0];
	const float64_t *ref = (float64_t *)ref_kl;
	const float64_t *input1 = (float64_t *)in_kl1;
	const float64_t *input2 = (float64_t *)in_kl2;
	float64_t *output;

	__ASSERT_NO_MSG(ARRAY_SIZE(in_kl_dim) > length);
	__ASSERT_NO_MSG(ARRAY_SIZE(ref_kl) >= length);

	/* Allocate output buffer */
	output = malloc(length * sizeof(float64_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Run test function */
	for (index = 0; index < length; index++) {
		output[index] =
			arm_kullback_leibler_f64(
				input1, input2, in_kl_dim[index + 1]);

		input1 += in_kl_dim[index + 1];
		input2 += in_kl_dim[index + 1];
	}

	/* Validate output */
	zassert_true(
		test_snr_error_f64(length, ref, output, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_near_equal_f64(length, ref, output, REL_ERROR_THRESH),
		ASSERT_MSG_REL_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

void test_statistics_f64(void)
{
	ztest_test_suite(statistics_f64,
		ztest_unit_test(test_arm_entropy_f64),
		ztest_unit_test(test_arm_kullback_leibler_f64)
		);

	ztest_run_test_suite(statistics_f64);
}
