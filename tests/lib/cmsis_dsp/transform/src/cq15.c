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
#include <arm_const_structs.h>
#include "../../common/test_common.h"

#include "cq15.pat"

#define SNR_ERROR_THRESH_FFT	((float32_t)30)
#define SNR_ERROR_THRESH_IFFT	((float32_t)5)

static void test_arm_cfft_q15(
	const q15_t *input, const q15_t *ref, size_t length)
{
	arm_cfft_instance_q15 inst;
	q15_t *output;
	arm_status status;

	/* Initialise instance */
	status = arm_cfft_init_q15(&inst, length / 2);

	zassert_equal(status, ARM_MATH_SUCCESS,
		      ASSERT_MSG_INCORRECT_COMP_RESULT);

	/* Allocate output buffer */
	output = malloc(length * sizeof(q15_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Load data in place */
	memcpy(output, input, length * sizeof(q15_t));

	/* Run test function */
	arm_cfft_q15(&inst, output, false, true);

	/* Validate output */
	zassert_true(
		test_snr_error_q15(length, output, ref, SNR_ERROR_THRESH_FFT),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT3(transform_cq15,
	arm_cfft_q15, noisy_16,
	in_cfft_noisy_16, ref_cfft_noisy_16, 32);

DEFINE_TEST_VARIANT3(transform_cq15,
	arm_cfft_q15, noisy_32,
	in_cfft_noisy_32, ref_cfft_noisy_32, 64);

DEFINE_TEST_VARIANT3(transform_cq15,
	arm_cfft_q15, noisy_64,
	in_cfft_noisy_64, ref_cfft_noisy_64, 128);

DEFINE_TEST_VARIANT3(transform_cq15,
	arm_cfft_q15, noisy_128,
	in_cfft_noisy_128, ref_cfft_noisy_128, 256);

DEFINE_TEST_VARIANT3(transform_cq15,
	arm_cfft_q15, noisy_256,
	in_cfft_noisy_256, ref_cfft_noisy_256, 512);

DEFINE_TEST_VARIANT3(transform_cq15,
	arm_cfft_q15, noisy_512,
	in_cfft_noisy_512, ref_cfft_noisy_512, 1024);

DEFINE_TEST_VARIANT3(transform_cq15,
	arm_cfft_q15, noisy_1024,
	in_cfft_noisy_1024, ref_cfft_noisy_1024, 2048);

DEFINE_TEST_VARIANT3(transform_cq15,
	arm_cfft_q15, noisy_2048,
	in_cfft_noisy_2048, ref_cfft_noisy_2048, 4096);

DEFINE_TEST_VARIANT3(transform_cq15,
	arm_cfft_q15, noisy_4096,
	in_cfft_noisy_4096, ref_cfft_noisy_4096, 8192);

DEFINE_TEST_VARIANT3(transform_cq15,
	arm_cfft_q15, step_16,
	in_cfft_step_16, ref_cfft_step_16, 32);

DEFINE_TEST_VARIANT3(transform_cq15,
	arm_cfft_q15, step_32,
	in_cfft_step_32, ref_cfft_step_32, 64);

DEFINE_TEST_VARIANT3(transform_cq15,
	arm_cfft_q15, step_64,
	in_cfft_step_64, ref_cfft_step_64, 128);

DEFINE_TEST_VARIANT3(transform_cq15,
	arm_cfft_q15, step_128,
	in_cfft_step_128, ref_cfft_step_128, 256);

DEFINE_TEST_VARIANT3(transform_cq15,
	arm_cfft_q15, step_256,
	in_cfft_step_256, ref_cfft_step_256, 512);

DEFINE_TEST_VARIANT3(transform_cq15,
	arm_cfft_q15, step_512,
	in_cfft_step_512, ref_cfft_step_512, 1024);

DEFINE_TEST_VARIANT3(transform_cq15,
	arm_cfft_q15, step_1024,
	in_cfft_step_1024, ref_cfft_step_1024, 2048);

DEFINE_TEST_VARIANT3(transform_cq15,
	arm_cfft_q15, step_2048,
	in_cfft_step_2048, ref_cfft_step_2048, 4096);

DEFINE_TEST_VARIANT3(transform_cq15,
	arm_cfft_q15, step_4096,
	in_cfft_step_4096, ref_cfft_step_4096, 8192);

static void test_arm_cifft_q15(
	int scale_factor, const q15_t *input, const q15_t *ref, size_t length)
{
	arm_cfft_instance_q15 inst;
	size_t index;
	q15_t *output, *scaled_ref;
	arm_status status;

	/* Initialise instance */
	status = arm_cfft_init_q15(&inst, length / 2);

	zassert_equal(status, ARM_MATH_SUCCESS,
		      ASSERT_MSG_INCORRECT_COMP_RESULT);

	/* Allocate buffers */
	output = malloc(length * sizeof(q15_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	scaled_ref = malloc(length * sizeof(q15_t));
	zassert_not_null(scaled_ref, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Load data in place */
	memcpy(output, input, length * sizeof(q15_t));

	/* Run test function */
	arm_cfft_q15(&inst, output, true, true);

	/* Scale reference data */
	for (index = 0; index < length; index++) {
		scaled_ref[index] = ref[index] >> scale_factor;
	}

	/* Validate output */
	zassert_true(
		test_snr_error_q15(length, output, scaled_ref,
			SNR_ERROR_THRESH_IFFT),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
	free(scaled_ref);
}

DEFINE_TEST_VARIANT4(transform_cq15,
	arm_cifft_q15, noisy_16, 4,
	in_cifft_noisy_16, in_cfft_noisy_16, 32);

DEFINE_TEST_VARIANT4(transform_cq15,
	arm_cifft_q15, noisy_32, 5,
	in_cifft_noisy_32, in_cfft_noisy_32, 64);

DEFINE_TEST_VARIANT4(transform_cq15,
	arm_cifft_q15, noisy_64, 6,
	in_cifft_noisy_64, in_cfft_noisy_64, 128);

DEFINE_TEST_VARIANT4(transform_cq15,
	arm_cifft_q15, noisy_128, 7,
	in_cifft_noisy_128, in_cfft_noisy_128, 256);

DEFINE_TEST_VARIANT4(transform_cq15,
	arm_cifft_q15, noisy_256, 8,
	in_cifft_noisy_256, in_cfft_noisy_256, 512);

DEFINE_TEST_VARIANT4(transform_cq15,
	arm_cifft_q15, noisy_512, 9,
	in_cifft_noisy_512, in_cfft_noisy_512, 1024);

DEFINE_TEST_VARIANT4(transform_cq15,
	arm_cifft_q15, noisy_1024, 10,
	in_cifft_noisy_1024, in_cfft_noisy_1024, 2048);

DEFINE_TEST_VARIANT4(transform_cq15,
	arm_cifft_q15, noisy_2048, 11,
	in_cifft_noisy_2048, in_cfft_noisy_2048, 4096);

DEFINE_TEST_VARIANT4(transform_cq15,
	arm_cifft_q15, noisy_4096, 12,
	in_cifft_noisy_4096, in_cfft_noisy_4096, 8192);

DEFINE_TEST_VARIANT4(transform_cq15,
	arm_cifft_q15, step_16, 4,
	in_cifft_step_16, in_cfft_step_16, 32);

DEFINE_TEST_VARIANT4(transform_cq15,
	arm_cifft_q15, step_32, 5,
	in_cifft_step_32, in_cfft_step_32, 64);

DEFINE_TEST_VARIANT4(transform_cq15,
	arm_cifft_q15, step_64, 6,
	in_cifft_step_64, in_cfft_step_64, 128);

DEFINE_TEST_VARIANT4(transform_cq15,
	arm_cifft_q15, step_128, 7,
	in_cifft_step_128, in_cfft_step_128, 256);

DEFINE_TEST_VARIANT4(transform_cq15,
	arm_cifft_q15, step_256, 8,
	in_cifft_step_256, in_cfft_step_256, 512);

DEFINE_TEST_VARIANT4(transform_cq15,
	arm_cifft_q15, step_512, 9,
	in_cifft_step_512, in_cfft_step_512, 1024);

DEFINE_TEST_VARIANT4(transform_cq15,
	arm_cifft_q15, step_1024, 10,
	in_cifft_step_1024, in_cfft_step_1024, 2048);

DEFINE_TEST_VARIANT4(transform_cq15,
	arm_cifft_q15, step_2048, 11,
	in_cifft_step_2048, in_cfft_step_2048, 4096);

DEFINE_TEST_VARIANT4(transform_cq15,
	arm_cifft_q15, step_4096, 12,
	in_cifft_step_4096, in_cfft_step_4096, 8192);

ZTEST_SUITE(transform_cq15, NULL, NULL, NULL, NULL, NULL);
