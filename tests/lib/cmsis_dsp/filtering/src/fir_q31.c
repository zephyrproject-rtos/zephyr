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

#include "fir_q31.pat"

#define SNR_ERROR_THRESH	((float32_t)100)
#define ABS_ERROR_THRESH_Q31	((q31_t)2)

#define COEFF_PADDING		(4)

ZTEST(filtering_fir_q31, test_arm_fir_q31)
{
	size_t sample_index, block_index;
	size_t block_size, tap_count;
	size_t sample_count = ARRAY_SIZE(in_config) / 2;
	size_t length = ARRAY_SIZE(ref_val);
	const uint16_t *config = in_config;
	const q31_t *input = (const q31_t *)in_val;
	const q31_t *coeff = (const q31_t *)in_coeff;
	const q31_t *ref = (const q31_t *)ref_val;
	q31_t *state, *output_buf, *output;
	arm_fir_instance_q31 inst;
#if defined(CONFIG_ARMV8_1_M_MVEI) && defined(CONFIG_FPU)
	q31_t coeff_padded[32];
	int round;
#endif

	/* Allocate buffers */
	state = malloc(3 * 47 * sizeof(q31_t));
	zassert_not_null(state, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	output_buf = malloc(length * sizeof(q31_t));
	zassert_not_null(output_buf, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	output = output_buf;

	/* Enumerate samples */
	for (sample_index = 0; sample_index < sample_count; sample_index++) {
		/* Resolve sample configurations */
		block_size = config[0];
		tap_count = config[1];

#if defined(CONFIG_ARMV8_1_M_MVEI) && defined(CONFIG_FPU)
		/* Copy coefficients and pad to zero */
		memset(coeff_padded, 127, sizeof(coeff_padded));
		round = tap_count / COEFF_PADDING;
		if ((round * COEFF_PADDING) < tap_count) {
			round++;
		}
		round = round * COEFF_PADDING;
		memset(coeff_padded, 0, round * sizeof(q31_t));
		memcpy(coeff_padded, coeff, tap_count * sizeof(q31_t));
#endif

		/* Initialise instance */
#if defined(CONFIG_ARMV8_1_M_MVEI) && defined(CONFIG_FPU)
		arm_fir_init_q31(&inst, tap_count, coeff_padded, state, block_size);
#else
		arm_fir_init_q31(&inst, tap_count, coeff, state, block_size);
#endif

		/* Reset input pointer */
		input = (const q31_t *)in_val;

		/* Enumerate blocks */
		for (block_index = 0; block_index < 2; block_index++) {
			/* Run test function */
			arm_fir_q31(&inst, input, output, block_size);

			/* Increment pointers */
			input += block_size;
			output += block_size;
		}

		/* Increment pointers */
		coeff += tap_count;
		config += 2;
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

ZTEST_SUITE(filtering_fir_q31, NULL, NULL, NULL, NULL, NULL);
