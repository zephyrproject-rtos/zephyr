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
#include "../../common/test_common.h"

#include "decim_q31.pat"

#define SNR_ERROR_THRESH		((float32_t)100)
#define ABS_ERROR_THRESH_Q31		((q31_t)2)
#define STATE_BUF_LEN			(16 + 768 - 1)

ZTEST(filtering_decim_q31, test_arm_fir_decimate_q31)
{
	uint32_t decim_factor, tap_count;
	size_t sample_index, block_size, ref_size;
	size_t sample_count = ARRAY_SIZE(in_config_decim) / 4;
	size_t length = ARRAY_SIZE(ref_decim);
	const uint32_t *config = in_config_decim;
	const q31_t *input = in_val_decim;
	const q31_t *coeff = in_coeff_decim;
	const q31_t *ref = ref_decim;
	q31_t *state, *output_buf, *output;
	arm_status status;
	arm_fir_decimate_instance_q31 inst;

	/* Allocate buffers */
	state = malloc(STATE_BUF_LEN * sizeof(q31_t));
	zassert_not_null(state, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	output_buf = malloc(length * sizeof(q31_t));
	zassert_not_null(output_buf, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	output = output_buf;

	/* Enumerate samples */
	for (sample_index = 0; sample_index < sample_count; sample_index++) {
		/* Resolve sample configurations */
		decim_factor = config[0];
		tap_count = config[1];
		block_size = config[2];
		ref_size = config[3];

		/* Initialise instance */
		status = arm_fir_decimate_init_q31(&inst, tap_count,
						   decim_factor, coeff, state,
						   block_size);

		zassert_equal(status, ARM_MATH_SUCCESS,
			      ASSERT_MSG_INCORRECT_COMP_RESULT);

		/* Run test function */
		arm_fir_decimate_q31(&inst, input, output, block_size);

		/* Increment pointers */
		input += block_size;
		output += ref_size;
		coeff += tap_count;
		config += 4;
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

ZTEST(filtering_decim_q31, test_arm_fir_interpolate_q31)
{
	uint32_t filter_length, tap_count;
	size_t sample_index, block_size, ref_size;
	size_t sample_count = ARRAY_SIZE(in_config_interp) / 4;
	size_t length = ARRAY_SIZE(ref_interp);
	const uint32_t *config = in_config_interp;
	const q31_t *input = in_val_interp;
	const q31_t *coeff = in_coeff_interp;
	const q31_t *ref = ref_interp;
	q31_t *state, *output_buf, *output;
	arm_status status;
	arm_fir_interpolate_instance_q31 inst;

	/* Allocate buffers */
	state = malloc(STATE_BUF_LEN * sizeof(q31_t));
	zassert_not_null(state, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	output_buf = malloc(length * sizeof(q31_t));
	zassert_not_null(output_buf, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	output = output_buf;

	/* Enumerate samples */
	for (sample_index = 0; sample_index < sample_count; sample_index++) {
		/* Resolve sample configurations */
		filter_length = config[0];
		tap_count = config[1];
		block_size = config[2];
		ref_size = config[3];

		/* Initialise instance */
		status = arm_fir_interpolate_init_q31(&inst, filter_length,
						      tap_count, coeff,
						      state, block_size);

		zassert_equal(status, ARM_MATH_SUCCESS,
			      ASSERT_MSG_INCORRECT_COMP_RESULT);

		/* Run test function */
		arm_fir_interpolate_q31(&inst, input, output, block_size);

		/* Increment pointers */
		input += block_size;
		output += ref_size;
		coeff += tap_count;
		config += 4;
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

ZTEST_SUITE(filtering_decim_q31, NULL, NULL, NULL, NULL, NULL);
