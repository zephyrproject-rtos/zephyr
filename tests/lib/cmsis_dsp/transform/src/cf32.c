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
#include <arm_const_structs.h>
#include "../../common/test_common.h"

#include "cf32.pat"

#define SNR_ERROR_THRESH	((float32_t)120)

static void test_arm_cfft_f32_cmplx_backend(
	bool inverse, const uint32_t *input, const uint32_t *ref, size_t length)
{
	arm_cfft_instance_f32 inst;
	float32_t *output;
	arm_status status;

	/* Initialise instance */
	status = arm_cfft_init_f32(&inst, length / 2);

	zassert_equal(status, ARM_MATH_SUCCESS,
		      ASSERT_MSG_INCORRECT_COMP_RESULT);

	/* Allocate output buffer */
	output = malloc(length * sizeof(float32_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Load data in place */
	memcpy(output, input, length * sizeof(float32_t));

	/* Run test function */
	arm_cfft_f32(&inst, output, inverse, true);

	/* Validate output */
	zassert_true(
		test_snr_error_f32(length, output, (float32_t *)ref,
			SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

static void test_arm_cfft_f32_cmplx(
	const uint32_t *input, const uint32_t *ref, size_t length)
{
	test_arm_cfft_f32_cmplx_backend(false, input, ref, length);
}

DEFINE_TEST_VARIANT3(arm_cfft_f32_cmplx, noisy_16,
	in_cfft_noisy_16, ref_cfft_noisy_16, 32);

DEFINE_TEST_VARIANT3(arm_cfft_f32_cmplx, noisy_32,
	in_cfft_noisy_32, ref_cfft_noisy_32, 64);

DEFINE_TEST_VARIANT3(arm_cfft_f32_cmplx, noisy_64,
	in_cfft_noisy_64, ref_cfft_noisy_64, 128);

DEFINE_TEST_VARIANT3(arm_cfft_f32_cmplx, noisy_128,
	in_cfft_noisy_128, ref_cfft_noisy_128, 256);

DEFINE_TEST_VARIANT3(arm_cfft_f32_cmplx, noisy_256,
	in_cfft_noisy_256, ref_cfft_noisy_256, 512);

DEFINE_TEST_VARIANT3(arm_cfft_f32_cmplx, noisy_512,
	in_cfft_noisy_512, ref_cfft_noisy_512, 1024);

DEFINE_TEST_VARIANT3(arm_cfft_f32_cmplx, noisy_1024,
	in_cfft_noisy_1024, ref_cfft_noisy_1024, 2048);

DEFINE_TEST_VARIANT3(arm_cfft_f32_cmplx, noisy_2048,
	in_cfft_noisy_2048, ref_cfft_noisy_2048, 4096);

DEFINE_TEST_VARIANT3(arm_cfft_f32_cmplx, noisy_4096,
	in_cfft_noisy_4096, ref_cfft_noisy_4096, 8192);

DEFINE_TEST_VARIANT3(arm_cfft_f32_cmplx, step_16,
	in_cfft_step_16, ref_cfft_step_16, 32);

DEFINE_TEST_VARIANT3(arm_cfft_f32_cmplx, step_32,
	in_cfft_step_32, ref_cfft_step_32, 64);

DEFINE_TEST_VARIANT3(arm_cfft_f32_cmplx, step_64,
	in_cfft_step_64, ref_cfft_step_64, 128);

DEFINE_TEST_VARIANT3(arm_cfft_f32_cmplx, step_128,
	in_cfft_step_128, ref_cfft_step_128, 256);

DEFINE_TEST_VARIANT3(arm_cfft_f32_cmplx, step_256,
	in_cfft_step_256, ref_cfft_step_256, 512);

DEFINE_TEST_VARIANT3(arm_cfft_f32_cmplx, step_512,
	in_cfft_step_512, ref_cfft_step_512, 1024);

DEFINE_TEST_VARIANT3(arm_cfft_f32_cmplx, step_1024,
	in_cfft_step_1024, ref_cfft_step_1024, 2048);

DEFINE_TEST_VARIANT3(arm_cfft_f32_cmplx, step_2048,
	in_cfft_step_2048, ref_cfft_step_2048, 4096);

DEFINE_TEST_VARIANT3(arm_cfft_f32_cmplx, step_4096,
	in_cfft_step_4096, ref_cfft_step_4096, 8192);

static void test_arm_cifft_f32_cmplx(
	const uint32_t *input, const uint32_t *ref, size_t length)
{
	test_arm_cfft_f32_cmplx_backend(true, input, ref, length);
}

DEFINE_TEST_VARIANT3(arm_cifft_f32_cmplx, noisy_16,
	in_cifft_noisy_16, in_cfft_noisy_16, 32);

DEFINE_TEST_VARIANT3(arm_cifft_f32_cmplx, noisy_32,
	in_cifft_noisy_32, in_cfft_noisy_32, 64);

DEFINE_TEST_VARIANT3(arm_cifft_f32_cmplx, noisy_64,
	in_cifft_noisy_64, in_cfft_noisy_64, 128);

DEFINE_TEST_VARIANT3(arm_cifft_f32_cmplx, noisy_128,
	in_cifft_noisy_128, in_cfft_noisy_128, 256);

DEFINE_TEST_VARIANT3(arm_cifft_f32_cmplx, noisy_256,
	in_cifft_noisy_256, in_cfft_noisy_256, 512);

DEFINE_TEST_VARIANT3(arm_cifft_f32_cmplx, noisy_512,
	in_cifft_noisy_512, in_cfft_noisy_512, 1024);

DEFINE_TEST_VARIANT3(arm_cifft_f32_cmplx, noisy_1024,
	in_cifft_noisy_1024, in_cfft_noisy_1024, 2048);

DEFINE_TEST_VARIANT3(arm_cifft_f32_cmplx, noisy_2048,
	in_cifft_noisy_2048, in_cfft_noisy_2048, 4096);

DEFINE_TEST_VARIANT3(arm_cifft_f32_cmplx, noisy_4096,
	in_cifft_noisy_4096, in_cfft_noisy_4096, 8192);

DEFINE_TEST_VARIANT3(arm_cifft_f32_cmplx, step_16,
	in_cifft_step_16, in_cfft_step_16, 32);

DEFINE_TEST_VARIANT3(arm_cifft_f32_cmplx, step_32,
	in_cifft_step_32, in_cfft_step_32, 64);

DEFINE_TEST_VARIANT3(arm_cifft_f32_cmplx, step_64,
	in_cifft_step_64, in_cfft_step_64, 128);

DEFINE_TEST_VARIANT3(arm_cifft_f32_cmplx, step_128,
	in_cifft_step_128, in_cfft_step_128, 256);

DEFINE_TEST_VARIANT3(arm_cifft_f32_cmplx, step_256,
	in_cifft_step_256, in_cfft_step_256, 512);

DEFINE_TEST_VARIANT3(arm_cifft_f32_cmplx, step_512,
	in_cifft_step_512, in_cfft_step_512, 1024);

DEFINE_TEST_VARIANT3(arm_cifft_f32_cmplx, step_1024,
	in_cifft_step_1024, in_cfft_step_1024, 2048);

DEFINE_TEST_VARIANT3(arm_cifft_f32_cmplx, step_2048,
	in_cifft_step_2048, in_cfft_step_2048, 4096);

DEFINE_TEST_VARIANT3(arm_cifft_f32_cmplx, step_4096,
	in_cifft_step_4096, in_cfft_step_4096, 8192);

void test_transform_cf32(void)
{
	ztest_test_suite(transform_cf32,
		ztest_unit_test(test_arm_cfft_f32_cmplx_noisy_16),
		ztest_unit_test(test_arm_cifft_f32_cmplx_noisy_16),
		ztest_unit_test(test_arm_cfft_f32_cmplx_noisy_32),
		ztest_unit_test(test_arm_cifft_f32_cmplx_noisy_32),
		ztest_unit_test(test_arm_cfft_f32_cmplx_noisy_64),
		ztest_unit_test(test_arm_cifft_f32_cmplx_noisy_64),
		ztest_unit_test(test_arm_cfft_f32_cmplx_noisy_128),
		ztest_unit_test(test_arm_cifft_f32_cmplx_noisy_128),
		ztest_unit_test(test_arm_cfft_f32_cmplx_noisy_256),
		ztest_unit_test(test_arm_cifft_f32_cmplx_noisy_256),
		ztest_unit_test(test_arm_cfft_f32_cmplx_noisy_512),
		ztest_unit_test(test_arm_cifft_f32_cmplx_noisy_512),
		ztest_unit_test(test_arm_cfft_f32_cmplx_noisy_1024),
		ztest_unit_test(test_arm_cifft_f32_cmplx_noisy_1024),
		ztest_unit_test(test_arm_cfft_f32_cmplx_noisy_2048),
		ztest_unit_test(test_arm_cifft_f32_cmplx_noisy_2048),
		ztest_unit_test(test_arm_cfft_f32_cmplx_noisy_4096),
		ztest_unit_test(test_arm_cifft_f32_cmplx_noisy_4096),
		ztest_unit_test(test_arm_cfft_f32_cmplx_step_16),
		ztest_unit_test(test_arm_cifft_f32_cmplx_step_16),
		ztest_unit_test(test_arm_cfft_f32_cmplx_step_32),
		ztest_unit_test(test_arm_cifft_f32_cmplx_step_32),
		ztest_unit_test(test_arm_cfft_f32_cmplx_step_64),
		ztest_unit_test(test_arm_cifft_f32_cmplx_step_64),
		ztest_unit_test(test_arm_cfft_f32_cmplx_step_128),
		ztest_unit_test(test_arm_cifft_f32_cmplx_step_128),
		ztest_unit_test(test_arm_cfft_f32_cmplx_step_256),
		ztest_unit_test(test_arm_cifft_f32_cmplx_step_256),
		ztest_unit_test(test_arm_cfft_f32_cmplx_step_512),
		ztest_unit_test(test_arm_cifft_f32_cmplx_step_512),
		ztest_unit_test(test_arm_cfft_f32_cmplx_step_1024),
		ztest_unit_test(test_arm_cifft_f32_cmplx_step_1024),
		ztest_unit_test(test_arm_cfft_f32_cmplx_step_2048),
		ztest_unit_test(test_arm_cifft_f32_cmplx_step_2048),
		ztest_unit_test(test_arm_cfft_f32_cmplx_step_4096),
		ztest_unit_test(test_arm_cifft_f32_cmplx_step_4096)
		);

	ztest_run_test_suite(transform_cf32);
}
