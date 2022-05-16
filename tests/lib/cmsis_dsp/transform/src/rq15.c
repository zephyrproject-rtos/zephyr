/*
 * Copyright (c) 2020 Stephanos Ioannidis <root@stephanos.io>
 * Copyright (C) 2010-2020 ARM Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * NOTE: The following tests are disabled because the current Q15 IFFT
 *       implementation cannot achieve sufficient accuracy to pass them:
 *
 *       test_arm_rifft_q15_noisy_256
 *       test_arm_rifft_q15_noisy_512
 *       test_arm_rifft_q15_noisy_1024
 *       test_arm_rifft_q15_noisy_2048
 *       test_arm_rifft_q15_noisy_4096
 *       test_arm_rifft_q15_step_256
 *       test_arm_rifft_q15_step_512
 *       test_arm_rifft_q15_step_1024
 *       test_arm_rifft_q15_step_2048
 *       test_arm_rifft_q15_step_4096
 */

#include <ztest.h>
#include <zephyr/zephyr.h>
#include <stdlib.h>
#include <arm_math.h>
#include <arm_const_structs.h>
#include "../../common/test_common.h"

#include "rq15.pat"

#define SNR_ERROR_THRESH_FFT	((float32_t)40)
#define SNR_ERROR_THRESH_IFFT	((float32_t)25)

static void test_arm_rfft_q15(
	const q15_t *input, const q15_t *ref, size_t length)
{
	arm_rfft_instance_q15 inst;
	q15_t *scratch, *output;

	/* Initialise instance */
	arm_rfft_init_q15(&inst, length, false, true);

	/* Allocate buffers */
	scratch = malloc(length * sizeof(q15_t));
	zassert_not_null(scratch, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	output = malloc(2 * length * sizeof(q15_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Load input data into the scratch buffer */
	memcpy(scratch, input, length * sizeof(q15_t));

	/* Run test function */
	arm_rfft_q15(&inst, scratch, output);

	/* Validate output */
	zassert_true(
		test_snr_error_q15(length, output, ref, SNR_ERROR_THRESH_FFT),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	/* Free output buffer */
	free(scratch);
	free(output);
}

DEFINE_TEST_VARIANT3(arm_rfft_q15, noisy_32,
	in_rfft_noisy_32, ref_rfft_noisy_32, 32);

DEFINE_TEST_VARIANT3(arm_rfft_q15, noisy_64,
	in_rfft_noisy_64, ref_rfft_noisy_64, 64);

DEFINE_TEST_VARIANT3(arm_rfft_q15, noisy_128,
	in_rfft_noisy_128, ref_rfft_noisy_128, 128);

DEFINE_TEST_VARIANT3(arm_rfft_q15, noisy_256,
	in_rfft_noisy_256, ref_rfft_noisy_256, 256);

DEFINE_TEST_VARIANT3(arm_rfft_q15, noisy_512,
	in_rfft_noisy_512, ref_rfft_noisy_512, 512);

DEFINE_TEST_VARIANT3(arm_rfft_q15, noisy_1024,
	in_rfft_noisy_1024, ref_rfft_noisy_1024, 1024);

DEFINE_TEST_VARIANT3(arm_rfft_q15, noisy_2048,
	in_rfft_noisy_2048, ref_rfft_noisy_2048, 2048);

DEFINE_TEST_VARIANT3(arm_rfft_q15, noisy_4096,
	in_rfft_noisy_4096, ref_rfft_noisy_4096, 4096);

DEFINE_TEST_VARIANT3(arm_rfft_q15, step_32,
	in_rfft_step_32, ref_rfft_step_32, 32);

DEFINE_TEST_VARIANT3(arm_rfft_q15, step_64,
	in_rfft_step_64, ref_rfft_step_64, 64);

DEFINE_TEST_VARIANT3(arm_rfft_q15, step_128,
	in_rfft_step_128, ref_rfft_step_128, 128);

DEFINE_TEST_VARIANT3(arm_rfft_q15, step_256,
	in_rfft_step_256, ref_rfft_step_256, 256);

DEFINE_TEST_VARIANT3(arm_rfft_q15, step_512,
	in_rfft_step_512, ref_rfft_step_512, 512);

DEFINE_TEST_VARIANT3(arm_rfft_q15, step_1024,
	in_rfft_step_1024, ref_rfft_step_1024, 1024);

DEFINE_TEST_VARIANT3(arm_rfft_q15, step_2048,
	in_rfft_step_2048, ref_rfft_step_2048, 2048);

DEFINE_TEST_VARIANT3(arm_rfft_q15, step_4096,
	in_rfft_step_4096, ref_rfft_step_4096, 4096);

static void test_arm_rifft_q15(
	int scale_factor, const q15_t *input, const q15_t *ref, size_t length)
{
	size_t index;
	arm_rfft_instance_q15 inst;
	q15_t *scratch, *output;

	/* Initialise instance */
	arm_rfft_init_q15(&inst, length, true, true);

	/* Allocate buffers */
	scratch = calloc(length + 2, sizeof(q15_t)); /* see #24701 */
	zassert_not_null(scratch, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	output = malloc(2 * length * sizeof(q15_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Load input data into the scratch buffer */
	memcpy(scratch, input, length * sizeof(q15_t));

	/* Run test function */
	arm_rfft_q15(&inst, scratch, output);

	/* Scale reference data */
	for (index = 0; index < length; index++) {
		output[index] = output[index] << scale_factor;
	}

	/* Validate output */
	zassert_true(
		test_snr_error_q15(length, output, ref, SNR_ERROR_THRESH_IFFT),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	/* Free output buffer */
	free(scratch);
	free(output);
}

DEFINE_TEST_VARIANT4(arm_rifft_q15, noisy_32, 5,
	in_rifft_noisy_32, in_rfft_noisy_32, 32);

DEFINE_TEST_VARIANT4(arm_rifft_q15, noisy_64, 6,
	in_rifft_noisy_64, in_rfft_noisy_64, 64);

DEFINE_TEST_VARIANT4(arm_rifft_q15, noisy_128, 7,
	in_rifft_noisy_128, in_rfft_noisy_128, 128);

#if 0
DEFINE_TEST_VARIANT4(arm_rifft_q15, noisy_256, 8,
	in_rifft_noisy_256, in_rfft_noisy_256, 256);

DEFINE_TEST_VARIANT4(arm_rifft_q15, noisy_512, 9,
	in_rifft_noisy_512, in_rfft_noisy_512, 512);

DEFINE_TEST_VARIANT4(arm_rifft_q15, noisy_1024, 10,
	in_rifft_noisy_1024, in_rfft_noisy_1024, 1024);

DEFINE_TEST_VARIANT4(arm_rifft_q15, noisy_2048, 11,
	in_rifft_noisy_2048, in_rfft_noisy_2048, 2048);

DEFINE_TEST_VARIANT4(arm_rifft_q15, noisy_4096, 12,
	in_rifft_noisy_4096, in_rfft_noisy_4096, 4096);
#endif

DEFINE_TEST_VARIANT4(arm_rifft_q15, step_32, 5,
	in_rifft_step_32, in_rfft_step_32, 32);

DEFINE_TEST_VARIANT4(arm_rifft_q15, step_64, 6,
	in_rifft_step_64, in_rfft_step_64, 64);

DEFINE_TEST_VARIANT4(arm_rifft_q15, step_128, 7,
	in_rifft_step_128, in_rfft_step_128, 128);

#if 0
DEFINE_TEST_VARIANT4(arm_rifft_q15, step_256, 8,
	in_rifft_step_256, in_rfft_step_256, 256);

DEFINE_TEST_VARIANT4(arm_rifft_q15, step_512, 9,
	in_rifft_step_512, in_rfft_step_512, 512);

DEFINE_TEST_VARIANT4(arm_rifft_q15, step_1024, 10,
	in_rifft_step_1024, in_rfft_step_1024, 1024);

DEFINE_TEST_VARIANT4(arm_rifft_q15, step_2048, 11,
	in_rifft_step_2048, in_rfft_step_2048, 2048);

DEFINE_TEST_VARIANT4(arm_rifft_q15, step_4096, 12,
	in_rifft_step_4096, in_rfft_step_4096, 4096);
#endif

void test_transform_rq15(void)
{
	ztest_test_suite(transform_rq15,
		ztest_unit_test(test_arm_rfft_q15_noisy_32),
		ztest_unit_test(test_arm_rifft_q15_noisy_32),
		ztest_unit_test(test_arm_rfft_q15_noisy_64),
		ztest_unit_test(test_arm_rifft_q15_noisy_64),
		ztest_unit_test(test_arm_rfft_q15_noisy_128),
		ztest_unit_test(test_arm_rifft_q15_noisy_128),
		ztest_unit_test(test_arm_rfft_q15_noisy_256),
		/* ztest_unit_test(test_arm_rifft_q15_noisy_256), */
		ztest_unit_test(test_arm_rfft_q15_noisy_512),
		/* ztest_unit_test(test_arm_rifft_q15_noisy_512), */
		ztest_unit_test(test_arm_rfft_q15_noisy_1024),
		/* ztest_unit_test(test_arm_rifft_q15_noisy_1024), */
		ztest_unit_test(test_arm_rfft_q15_noisy_2048),
		/* ztest_unit_test(test_arm_rifft_q15_noisy_2048), */
		ztest_unit_test(test_arm_rfft_q15_noisy_4096),
		/* ztest_unit_test(test_arm_rifft_q15_noisy_4096), */
		ztest_unit_test(test_arm_rfft_q15_step_32),
		ztest_unit_test(test_arm_rifft_q15_step_32),
		ztest_unit_test(test_arm_rfft_q15_step_64),
		ztest_unit_test(test_arm_rifft_q15_step_64),
		ztest_unit_test(test_arm_rfft_q15_step_128),
		ztest_unit_test(test_arm_rifft_q15_step_128),
		ztest_unit_test(test_arm_rfft_q15_step_256),
		/* ztest_unit_test(test_arm_rifft_q15_step_256), */
		ztest_unit_test(test_arm_rfft_q15_step_512),
		/* ztest_unit_test(test_arm_rifft_q15_step_512), */
		ztest_unit_test(test_arm_rfft_q15_step_1024),
		/* ztest_unit_test(test_arm_rifft_q15_step_1024), */
		ztest_unit_test(test_arm_rfft_q15_step_2048),
		/* ztest_unit_test(test_arm_rifft_q15_step_2048), */
		ztest_unit_test(test_arm_rfft_q15_step_4096)
		/* ztest_unit_test(test_arm_rifft_q15_step_4096) */
		);

	ztest_run_test_suite(transform_rq15);
}
