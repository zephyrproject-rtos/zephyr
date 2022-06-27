/*
 * Copyright (c) 2021 Stephanos Ioannidis <root@stephanos.io>
 * Copyright (C) 2010-2021 ARM Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <zephyr/zephyr.h>
#include <stdlib.h>
#include <arm_math_f16.h>
#include <arm_const_structs.h>
#include "../../common/test_common.h"

#include "rf16.pat"

#define SNR_ERROR_THRESH	((float32_t)58)

static void test_arm_rfft_f16_real_backend(
	bool inverse, const uint16_t *input, const uint16_t *ref,
	size_t length)
{
	arm_rfft_fast_instance_f16 inst;
	float16_t *output, *scratch;

	/* Initialise instance */
	arm_rfft_fast_init_f16(&inst, length);

	/* Allocate output buffer */
	output = malloc(length * sizeof(float16_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	scratch = calloc(length + 2, sizeof(float16_t)); /* see #24701 */
	zassert_not_null(scratch, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Load data in place */
	memcpy(scratch, input, length * sizeof(float16_t));

	/* Run test function */
	arm_rfft_fast_f16(&inst, scratch, output, inverse);

	/* Validate output */
	zassert_true(
		test_snr_error_f16(length, output, (float16_t *)ref,
			SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
	free(scratch);
}

static void test_arm_rfft_f16_real(
	const uint16_t *input, const uint16_t *ref, size_t length)
{
	test_arm_rfft_f16_real_backend(false, input, ref, length);
}

DEFINE_TEST_VARIANT3(arm_rfft_f16_real, noisy_32,
	in_rfft_noisy_32, ref_rfft_noisy_32, 32);

DEFINE_TEST_VARIANT3(arm_rfft_f16_real, noisy_64,
	in_rfft_noisy_64, ref_rfft_noisy_64, 64);

DEFINE_TEST_VARIANT3(arm_rfft_f16_real, noisy_128,
	in_rfft_noisy_128, ref_rfft_noisy_128, 128);

DEFINE_TEST_VARIANT3(arm_rfft_f16_real, noisy_256,
	in_rfft_noisy_256, ref_rfft_noisy_256, 256);

DEFINE_TEST_VARIANT3(arm_rfft_f16_real, noisy_512,
	in_rfft_noisy_512, ref_rfft_noisy_512, 512);

DEFINE_TEST_VARIANT3(arm_rfft_f16_real, noisy_1024,
	in_rfft_noisy_1024, ref_rfft_noisy_1024, 1024);

DEFINE_TEST_VARIANT3(arm_rfft_f16_real, noisy_2048,
	in_rfft_noisy_2048, ref_rfft_noisy_2048, 2048);

DEFINE_TEST_VARIANT3(arm_rfft_f16_real, noisy_4096,
	in_rfft_noisy_4096, ref_rfft_noisy_4096, 4096);

DEFINE_TEST_VARIANT3(arm_rfft_f16_real, step_32,
	in_rfft_step_32, ref_rfft_step_32, 32);

DEFINE_TEST_VARIANT3(arm_rfft_f16_real, step_64,
	in_rfft_step_64, ref_rfft_step_64, 64);

DEFINE_TEST_VARIANT3(arm_rfft_f16_real, step_128,
	in_rfft_step_128, ref_rfft_step_128, 128);

DEFINE_TEST_VARIANT3(arm_rfft_f16_real, step_256,
	in_rfft_step_256, ref_rfft_step_256, 256);

DEFINE_TEST_VARIANT3(arm_rfft_f16_real, step_512,
	in_rfft_step_512, ref_rfft_step_512, 512);

DEFINE_TEST_VARIANT3(arm_rfft_f16_real, step_1024,
	in_rfft_step_1024, ref_rfft_step_1024, 1024);

DEFINE_TEST_VARIANT3(arm_rfft_f16_real, step_2048,
	in_rfft_step_2048, ref_rfft_step_2048, 2048);

DEFINE_TEST_VARIANT3(arm_rfft_f16_real, step_4096,
	in_rfft_step_4096, ref_rfft_step_4096, 4096);

static void test_arm_rifft_f16_real(
	const uint16_t *input, const uint16_t *ref, size_t length)
{
	test_arm_rfft_f16_real_backend(true, input, ref, length);
}

DEFINE_TEST_VARIANT3(arm_rifft_f16_real, noisy_32,
	in_rifft_noisy_32, in_rfft_noisy_32, 32);

DEFINE_TEST_VARIANT3(arm_rifft_f16_real, noisy_64,
	in_rifft_noisy_64, in_rfft_noisy_64, 64);

DEFINE_TEST_VARIANT3(arm_rifft_f16_real, noisy_128,
	in_rifft_noisy_128, in_rfft_noisy_128, 128);

DEFINE_TEST_VARIANT3(arm_rifft_f16_real, noisy_256,
	in_rifft_noisy_256, in_rfft_noisy_256, 256);

DEFINE_TEST_VARIANT3(arm_rifft_f16_real, noisy_512,
	in_rifft_noisy_512, in_rfft_noisy_512, 512);

DEFINE_TEST_VARIANT3(arm_rifft_f16_real, noisy_1024,
	in_rifft_noisy_1024, in_rfft_noisy_1024, 1024);

DEFINE_TEST_VARIANT3(arm_rifft_f16_real, noisy_2048,
	in_rifft_noisy_2048, in_rfft_noisy_2048, 2048);

DEFINE_TEST_VARIANT3(arm_rifft_f16_real, noisy_4096,
	in_rifft_noisy_4096, in_rfft_noisy_4096, 4096);

DEFINE_TEST_VARIANT3(arm_rifft_f16_real, step_32,
	in_rifft_step_32, in_rfft_step_32, 32);

DEFINE_TEST_VARIANT3(arm_rifft_f16_real, step_64,
	in_rifft_step_64, in_rfft_step_64, 64);

DEFINE_TEST_VARIANT3(arm_rifft_f16_real, step_128,
	in_rifft_step_128, in_rfft_step_128, 128);

DEFINE_TEST_VARIANT3(arm_rifft_f16_real, step_256,
	in_rifft_step_256, in_rfft_step_256, 256);

DEFINE_TEST_VARIANT3(arm_rifft_f16_real, step_512,
	in_rifft_step_512, in_rfft_step_512, 512);

DEFINE_TEST_VARIANT3(arm_rifft_f16_real, step_1024,
	in_rifft_step_1024, in_rfft_step_1024, 1024);

DEFINE_TEST_VARIANT3(arm_rifft_f16_real, step_2048,
	in_rifft_step_2048, in_rfft_step_2048, 2048);

DEFINE_TEST_VARIANT3(arm_rifft_f16_real, step_4096,
	in_rifft_step_4096, in_rfft_step_4096, 4096);

void test_transform_rf16(void)
{
	ztest_test_suite(transform_rf16,
		ztest_unit_test(test_arm_rfft_f16_real_noisy_32),
		ztest_unit_test(test_arm_rifft_f16_real_noisy_32),
		ztest_unit_test(test_arm_rfft_f16_real_noisy_64),
		ztest_unit_test(test_arm_rifft_f16_real_noisy_64),
		ztest_unit_test(test_arm_rfft_f16_real_noisy_128),
		ztest_unit_test(test_arm_rifft_f16_real_noisy_128),
		ztest_unit_test(test_arm_rfft_f16_real_noisy_256),
		ztest_unit_test(test_arm_rifft_f16_real_noisy_256),
		ztest_unit_test(test_arm_rfft_f16_real_noisy_512),
		ztest_unit_test(test_arm_rifft_f16_real_noisy_512),
		ztest_unit_test(test_arm_rfft_f16_real_noisy_1024),
		ztest_unit_test(test_arm_rifft_f16_real_noisy_1024),
		ztest_unit_test(test_arm_rfft_f16_real_noisy_2048),
		ztest_unit_test(test_arm_rifft_f16_real_noisy_2048),
		ztest_unit_test(test_arm_rfft_f16_real_noisy_4096),
		ztest_unit_test(test_arm_rifft_f16_real_noisy_4096),
		ztest_unit_test(test_arm_rfft_f16_real_step_32),
		ztest_unit_test(test_arm_rifft_f16_real_step_32),
		ztest_unit_test(test_arm_rfft_f16_real_step_64),
		ztest_unit_test(test_arm_rifft_f16_real_step_64),
		ztest_unit_test(test_arm_rfft_f16_real_step_128),
		ztest_unit_test(test_arm_rifft_f16_real_step_128),
		ztest_unit_test(test_arm_rfft_f16_real_step_256),
		ztest_unit_test(test_arm_rifft_f16_real_step_256),
		ztest_unit_test(test_arm_rfft_f16_real_step_512),
		ztest_unit_test(test_arm_rifft_f16_real_step_512),
		ztest_unit_test(test_arm_rfft_f16_real_step_1024),
		ztest_unit_test(test_arm_rifft_f16_real_step_1024),
		ztest_unit_test(test_arm_rfft_f16_real_step_2048),
		ztest_unit_test(test_arm_rifft_f16_real_step_2048),
		ztest_unit_test(test_arm_rfft_f16_real_step_4096),
		ztest_unit_test(test_arm_rifft_f16_real_step_4096)
		);

	ztest_run_test_suite(transform_rf16);
}
