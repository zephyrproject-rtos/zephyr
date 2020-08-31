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
#include <arm_const_structs.h>
#include "../../common/test_common.h"

#include "cf64.pat"

#define SNR_ERROR_THRESH	((float64_t)250)

static void test_arm_cfft_f64_cmplx_backend(
	const arm_cfft_instance_f64 * inst, bool inverse,
	const uint64_t *input, const uint64_t *ref, size_t length)
{
	float64_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(float64_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Load data in place */
	memcpy(output, input, length * sizeof(float64_t));

	/* Run test function */
	arm_cfft_f64(inst, output, inverse, true);

	/* Validate output */
	zassert_true(
		test_snr_error_f64(length, output, (float64_t *)ref,
			SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

static void test_arm_cfft_f64_cmplx(
	const arm_cfft_instance_f64 * inst,
	const uint64_t *input, const uint64_t *ref, size_t length)
{
	test_arm_cfft_f64_cmplx_backend(inst, false, input, ref, length);
}

DEFINE_TEST_VARIANT4(arm_cfft_f64_cmplx, noisy_16, &arm_cfft_sR_f64_len16,
	in_cfft_noisy_16, ref_cfft_noisy_16, 32);

DEFINE_TEST_VARIANT4(arm_cfft_f64_cmplx, noisy_32, &arm_cfft_sR_f64_len32,
	in_cfft_noisy_32, ref_cfft_noisy_32, 64);

DEFINE_TEST_VARIANT4(arm_cfft_f64_cmplx, noisy_64, &arm_cfft_sR_f64_len64,
	in_cfft_noisy_64, ref_cfft_noisy_64, 128);

DEFINE_TEST_VARIANT4(arm_cfft_f64_cmplx, noisy_128, &arm_cfft_sR_f64_len128,
	in_cfft_noisy_128, ref_cfft_noisy_128, 256);

DEFINE_TEST_VARIANT4(arm_cfft_f64_cmplx, noisy_256, &arm_cfft_sR_f64_len256,
	in_cfft_noisy_256, ref_cfft_noisy_256, 512);

DEFINE_TEST_VARIANT4(arm_cfft_f64_cmplx, noisy_512, &arm_cfft_sR_f64_len512,
	in_cfft_noisy_512, ref_cfft_noisy_512, 1024);

DEFINE_TEST_VARIANT4(arm_cfft_f64_cmplx, noisy_1024, &arm_cfft_sR_f64_len1024,
	in_cfft_noisy_1024, ref_cfft_noisy_1024, 2048);

DEFINE_TEST_VARIANT4(arm_cfft_f64_cmplx, noisy_2048, &arm_cfft_sR_f64_len2048,
	in_cfft_noisy_2048, ref_cfft_noisy_2048, 4096);

DEFINE_TEST_VARIANT4(arm_cfft_f64_cmplx, noisy_4096, &arm_cfft_sR_f64_len4096,
	in_cfft_noisy_4096, ref_cfft_noisy_4096, 8192);

DEFINE_TEST_VARIANT4(arm_cfft_f64_cmplx, step_16, &arm_cfft_sR_f64_len16,
	in_cfft_step_16, ref_cfft_step_16, 32);

DEFINE_TEST_VARIANT4(arm_cfft_f64_cmplx, step_32, &arm_cfft_sR_f64_len32,
	in_cfft_step_32, ref_cfft_step_32, 64);

DEFINE_TEST_VARIANT4(arm_cfft_f64_cmplx, step_64, &arm_cfft_sR_f64_len64,
	in_cfft_step_64, ref_cfft_step_64, 128);

DEFINE_TEST_VARIANT4(arm_cfft_f64_cmplx, step_128, &arm_cfft_sR_f64_len128,
	in_cfft_step_128, ref_cfft_step_128, 256);

DEFINE_TEST_VARIANT4(arm_cfft_f64_cmplx, step_256, &arm_cfft_sR_f64_len256,
	in_cfft_step_256, ref_cfft_step_256, 512);

DEFINE_TEST_VARIANT4(arm_cfft_f64_cmplx, step_512, &arm_cfft_sR_f64_len512,
	in_cfft_step_512, ref_cfft_step_512, 1024);

DEFINE_TEST_VARIANT4(arm_cfft_f64_cmplx, step_1024, &arm_cfft_sR_f64_len1024,
	in_cfft_step_1024, ref_cfft_step_1024, 2048);

DEFINE_TEST_VARIANT4(arm_cfft_f64_cmplx, step_2048, &arm_cfft_sR_f64_len2048,
	in_cfft_step_2048, ref_cfft_step_2048, 4096);

DEFINE_TEST_VARIANT4(arm_cfft_f64_cmplx, step_4096, &arm_cfft_sR_f64_len4096,
	in_cfft_step_4096, ref_cfft_step_4096, 8192);

static void test_arm_cifft_f64_cmplx(
	const arm_cfft_instance_f64 * inst,
	const uint64_t *input, const uint64_t *ref, size_t length)
{
	test_arm_cfft_f64_cmplx_backend(inst, true, input, ref, length);
}

DEFINE_TEST_VARIANT4(arm_cifft_f64_cmplx, noisy_16, &arm_cfft_sR_f64_len16,
	in_cifft_noisy_16, in_cfft_noisy_16, 32);

DEFINE_TEST_VARIANT4(arm_cifft_f64_cmplx, noisy_32, &arm_cfft_sR_f64_len32,
	in_cifft_noisy_32, in_cfft_noisy_32, 64);

DEFINE_TEST_VARIANT4(arm_cifft_f64_cmplx, noisy_64, &arm_cfft_sR_f64_len64,
	in_cifft_noisy_64, in_cfft_noisy_64, 128);

DEFINE_TEST_VARIANT4(arm_cifft_f64_cmplx, noisy_128, &arm_cfft_sR_f64_len128,
	in_cifft_noisy_128, in_cfft_noisy_128, 256);

DEFINE_TEST_VARIANT4(arm_cifft_f64_cmplx, noisy_256, &arm_cfft_sR_f64_len256,
	in_cifft_noisy_256, in_cfft_noisy_256, 512);

DEFINE_TEST_VARIANT4(arm_cifft_f64_cmplx, noisy_512, &arm_cfft_sR_f64_len512,
	in_cifft_noisy_512, in_cfft_noisy_512, 1024);

DEFINE_TEST_VARIANT4(arm_cifft_f64_cmplx, noisy_1024, &arm_cfft_sR_f64_len1024,
	in_cifft_noisy_1024, in_cfft_noisy_1024, 2048);

DEFINE_TEST_VARIANT4(arm_cifft_f64_cmplx, noisy_2048, &arm_cfft_sR_f64_len2048,
	in_cifft_noisy_2048, in_cfft_noisy_2048, 4096);

DEFINE_TEST_VARIANT4(arm_cifft_f64_cmplx, noisy_4096, &arm_cfft_sR_f64_len4096,
	in_cifft_noisy_4096, in_cfft_noisy_4096, 8192);

DEFINE_TEST_VARIANT4(arm_cifft_f64_cmplx, step_16, &arm_cfft_sR_f64_len16,
	in_cifft_step_16, in_cfft_step_16, 32);

DEFINE_TEST_VARIANT4(arm_cifft_f64_cmplx, step_32, &arm_cfft_sR_f64_len32,
	in_cifft_step_32, in_cfft_step_32, 64);

DEFINE_TEST_VARIANT4(arm_cifft_f64_cmplx, step_64, &arm_cfft_sR_f64_len64,
	in_cifft_step_64, in_cfft_step_64, 128);

DEFINE_TEST_VARIANT4(arm_cifft_f64_cmplx, step_128, &arm_cfft_sR_f64_len128,
	in_cifft_step_128, in_cfft_step_128, 256);

DEFINE_TEST_VARIANT4(arm_cifft_f64_cmplx, step_256, &arm_cfft_sR_f64_len256,
	in_cifft_step_256, in_cfft_step_256, 512);

DEFINE_TEST_VARIANT4(arm_cifft_f64_cmplx, step_512, &arm_cfft_sR_f64_len512,
	in_cifft_step_512, in_cfft_step_512, 1024);

DEFINE_TEST_VARIANT4(arm_cifft_f64_cmplx, step_1024, &arm_cfft_sR_f64_len1024,
	in_cifft_step_1024, in_cfft_step_1024, 2048);

DEFINE_TEST_VARIANT4(arm_cifft_f64_cmplx, step_2048, &arm_cfft_sR_f64_len2048,
	in_cifft_step_2048, in_cfft_step_2048, 4096);

DEFINE_TEST_VARIANT4(arm_cifft_f64_cmplx, step_4096, &arm_cfft_sR_f64_len4096,
	in_cifft_step_4096, in_cfft_step_4096, 8192);

void test_transform_cf64(void)
{
	ztest_test_suite(transform_cf64,
		ztest_unit_test(test_arm_cfft_f64_cmplx_noisy_4096),
		ztest_unit_test(test_arm_cifft_f64_cmplx_noisy_4096),
		ztest_unit_test(test_arm_cfft_f64_cmplx_noisy_2048),
		ztest_unit_test(test_arm_cifft_f64_cmplx_noisy_2048),
		ztest_unit_test(test_arm_cfft_f64_cmplx_noisy_1024),
		ztest_unit_test(test_arm_cifft_f64_cmplx_noisy_1024),
		ztest_unit_test(test_arm_cfft_f64_cmplx_noisy_512),
		ztest_unit_test(test_arm_cifft_f64_cmplx_noisy_512),
		ztest_unit_test(test_arm_cfft_f64_cmplx_noisy_256),
		ztest_unit_test(test_arm_cifft_f64_cmplx_noisy_256),
		ztest_unit_test(test_arm_cfft_f64_cmplx_noisy_128),
		ztest_unit_test(test_arm_cifft_f64_cmplx_noisy_128),
		ztest_unit_test(test_arm_cfft_f64_cmplx_noisy_64),
		ztest_unit_test(test_arm_cifft_f64_cmplx_noisy_64),
		ztest_unit_test(test_arm_cfft_f64_cmplx_noisy_32),
		ztest_unit_test(test_arm_cifft_f64_cmplx_noisy_32),
		ztest_unit_test(test_arm_cfft_f64_cmplx_noisy_16),
		ztest_unit_test(test_arm_cifft_f64_cmplx_noisy_16),
		ztest_unit_test(test_arm_cfft_f64_cmplx_step_4096),
		ztest_unit_test(test_arm_cifft_f64_cmplx_step_4096),
		ztest_unit_test(test_arm_cfft_f64_cmplx_step_2048),
		ztest_unit_test(test_arm_cifft_f64_cmplx_step_2048),
		ztest_unit_test(test_arm_cfft_f64_cmplx_step_1024),
		ztest_unit_test(test_arm_cifft_f64_cmplx_step_1024),
		ztest_unit_test(test_arm_cfft_f64_cmplx_step_512),
		ztest_unit_test(test_arm_cifft_f64_cmplx_step_512),
		ztest_unit_test(test_arm_cfft_f64_cmplx_step_256),
		ztest_unit_test(test_arm_cifft_f64_cmplx_step_256),
		ztest_unit_test(test_arm_cfft_f64_cmplx_step_128),
		ztest_unit_test(test_arm_cifft_f64_cmplx_step_128),
		ztest_unit_test(test_arm_cfft_f64_cmplx_step_64),
		ztest_unit_test(test_arm_cifft_f64_cmplx_step_64),
		ztest_unit_test(test_arm_cfft_f64_cmplx_step_32),
		ztest_unit_test(test_arm_cifft_f64_cmplx_step_32),
		ztest_unit_test(test_arm_cfft_f64_cmplx_step_16),
		ztest_unit_test(test_arm_cifft_f64_cmplx_step_16)
		);

	ztest_run_test_suite(transform_cf64);
}
