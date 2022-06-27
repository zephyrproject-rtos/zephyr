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

#include "biquad_q31.pat"

#define SNR_ERROR_THRESH		((float32_t)115)
#define ABS_ERROR_THRESH_Q31		((q31_t)1000)

#define SNR_ERROR_THRESH_32x64		((float32_t)140)
#define ABS_ERROR_THRESH_Q31_32x64	((q31_t)25)

static void test_arm_biquad_cascade_df1_q31(void)
{
	size_t index;
	size_t length = ARRAY_SIZE(ref_default);
	size_t block_size = length / 2;
	const q31_t *input = in_default_val;
	const q31_t *coeff = in_default_coeff;
	const q31_t *ref = ref_default;
	q31_t *state, *output_buf, *output;
	arm_biquad_casd_df1_inst_q31 inst;

	/* Allocate buffers */
	state = malloc(32 * sizeof(q31_t));
	zassert_not_null(state, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	output_buf = malloc(length * sizeof(q31_t));
	zassert_not_null(output_buf, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	output = output_buf;

	/* Initialise instance */
	arm_biquad_cascade_df1_init_q31(&inst, 3, coeff, state, 2);

	/* Enumerate blocks */
	for (index = 0; index < 2; index++) {
		/* Run test function */
		arm_biquad_cascade_df1_q31(&inst, input, output, block_size);

		/* Increment pointers */
		input += block_size;
		output += block_size;
	}

	/* Validate output */
	zassert_true(
		test_snr_error_q31(length, output_buf, ref, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_near_equal_q31(length, output_buf, ref,
			ABS_ERROR_THRESH_Q31),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free buffers */
	free(state);
	free(output_buf);
}

static void test_arm_biquad_cascade_df1_32x64_q31(void)
{
	size_t index;
	size_t length = ARRAY_SIZE(ref_default);
	size_t block_size = length / 2;
	const q31_t *input = in_default_val;
	const q31_t *coeff = in_default_coeff;
	const q31_t *ref = ref_default;
	q63_t *state;
	q31_t *output_buf, *output;
	arm_biquad_cas_df1_32x64_ins_q31 inst;

	/* Allocate buffers */
	state = malloc(32 * sizeof(q63_t));
	zassert_not_null(state, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	output_buf = malloc(length * sizeof(q31_t));
	zassert_not_null(output_buf, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	output = output_buf;

	/* Initialise instance */
	arm_biquad_cas_df1_32x64_init_q31(&inst, 3, coeff, state, 2);

	/* Enumerate blocks */
	for (index = 0; index < 2; index++) {
		/* Run test function */
		arm_biquad_cas_df1_32x64_q31(&inst, input, output, block_size);

		/* Increment pointers */
		input += block_size;
		output += block_size;
	}

	/* Validate output */
	zassert_true(
		test_snr_error_q31(length, output_buf, ref,
			SNR_ERROR_THRESH_32x64),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_near_equal_q31(length, output_buf, ref,
			ABS_ERROR_THRESH_Q31_32x64),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free buffers */
	free(state);
	free(output_buf);
}

void test_filtering_biquad_q31(void)
{
	ztest_test_suite(filtering_biquad_q31,
		ztest_unit_test(test_arm_biquad_cascade_df1_q31),
		ztest_unit_test(test_arm_biquad_cascade_df1_32x64_q31)
		);

	ztest_run_test_suite(filtering_biquad_q31);
}
