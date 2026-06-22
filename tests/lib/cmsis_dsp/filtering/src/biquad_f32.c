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

#include "biquad_f32.pat"

#define SNR_ERROR_THRESH	((float32_t)98)
#define REL_ERROR_THRESH	(1.2e-3)

ZTEST(filtering_biquad_f32, test_arm_biquad_cascade_df1_f32_default)
{
	size_t index;
	size_t length = ARRAY_SIZE(ref_default);
	size_t block_size = length / 2;
	const float32_t *input = (const float32_t *)in_default_val;
	const float32_t *coeff = (const float32_t *)in_default_coeff;
	const float32_t *ref = (const float32_t *)ref_default;
	float32_t *state, *output_buf, *output;
	arm_biquad_casd_df1_inst_f32 inst;
#if defined(CONFIG_ARMV8_1_M_MVEF) && defined(CONFIG_FPU)
	arm_biquad_mod_coef_f32 *coeff_mod;
#endif

	/* Allocate buffers */
	state = calloc(128, sizeof(float32_t));
	zassert_not_null(state, ASSERT_MSG_BUFFER_ALLOC_FAILED);

#if defined(CONFIG_ARMV8_1_M_MVEF) && defined(CONFIG_FPU)
	coeff_mod = calloc(47, sizeof(arm_biquad_mod_coef_f32)); /* 47 stages */
	zassert_not_null(coeff_mod, ASSERT_MSG_BUFFER_ALLOC_FAILED);
#endif

	/* FIXME: `length + 2` is required here because of ARM-software/CMSIS_5#1475 */
	output_buf = calloc(length + 2, sizeof(float32_t));
	zassert_not_null(output_buf, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	output = output_buf;

	/* Initialise instance */
#if defined(CONFIG_ARMV8_1_M_MVEF) && defined(CONFIG_FPU)
	arm_biquad_cascade_df1_mve_init_f32(&inst, 3, coeff, coeff_mod, state);
#else
	arm_biquad_cascade_df1_init_f32(&inst, 3, coeff, state);
#endif

	/* Enumerate blocks */
	for (index = 0; index < 2; index++) {
		/* Run test function */
		arm_biquad_cascade_df1_f32(&inst, input, output, block_size);

		/* Increment pointers */
		input += block_size;
		output += block_size;
	}

	/* Validate output */
	zassert_true(
		test_snr_error_f32(length, output_buf, ref, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_rel_error_f32(length, output_buf, ref, REL_ERROR_THRESH),
		ASSERT_MSG_REL_ERROR_LIMIT_EXCEED);

	/* Free buffers */
	free(state);
	free(output_buf);
}

ZTEST(filtering_biquad_f32, test_arm_biquad_cascade_df2t_f32_default)
{
	size_t index;
	size_t length = ARRAY_SIZE(ref_default);
	size_t block_size = length / 2;
	const float32_t *input = (const float32_t *)in_default_val;
	const float32_t *coeff = (const float32_t *)in_default_coeff;
	const float32_t *ref = (const float32_t *)ref_default;
	float32_t *state, *output_buf, *output;
	arm_biquad_cascade_df2T_instance_f32 inst;

	/* Allocate buffers */
	state = malloc(128 * sizeof(float32_t));
	zassert_not_null(state, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	output_buf = malloc(length * sizeof(float32_t));
	zassert_not_null(output_buf, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	output = output_buf;

	/* Initialise instance */
	arm_biquad_cascade_df2T_init_f32(&inst, 3, coeff, state);

	/* TODO: Add NEON support */

	/* Enumerate blocks */
	for (index = 0; index < 2; index++) {
		/* Run test function */
		arm_biquad_cascade_df2T_f32(&inst, input, output, block_size);

		/* Increment pointers */
		input += block_size;
		output += block_size;
	}

	/* Validate output */
	zassert_true(
		test_snr_error_f32(length, output_buf, ref, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_rel_error_f32(length, output_buf, ref, REL_ERROR_THRESH),
		ASSERT_MSG_REL_ERROR_LIMIT_EXCEED);

	/* Free buffers */
	free(state);
	free(output_buf);
}

ZTEST(filtering_biquad_f32, test_arm_biquad_cascade_df1_f32_rand)
{
	size_t sample_index, stage_count, block_size;
	size_t sample_count = ARRAY_SIZE(in_rand_config) / 2;
	size_t length = ARRAY_SIZE(ref_rand_mono);
	const uint16_t *config = in_rand_config;
	const float32_t *input = (const float32_t *)in_rand_mono_val;
	const float32_t *coeff = (const float32_t *)in_rand_coeff;
	const float32_t *ref = (const float32_t *)ref_rand_mono;
	float32_t *state, *output_buf, *output;
	arm_biquad_casd_df1_inst_f32 inst;
#if defined(CONFIG_ARMV8_1_M_MVEF) && defined(CONFIG_FPU)
	arm_biquad_mod_coef_f32 *coeff_mod;
#endif

	/* Allocate buffers */
	state = calloc(128, sizeof(float32_t));
	zassert_not_null(state, ASSERT_MSG_BUFFER_ALLOC_FAILED);

#if defined(CONFIG_ARMV8_1_M_MVEF) && defined(CONFIG_FPU)
	coeff_mod = calloc(47, sizeof(arm_biquad_mod_coef_f32)); /* 47 stages */
	zassert_not_null(coeff_mod, ASSERT_MSG_BUFFER_ALLOC_FAILED);
#endif

	output_buf = calloc(length, sizeof(float32_t));
	zassert_not_null(output_buf, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	output = output_buf;

	/* Enumerate samples */
	for (sample_index = 0; sample_index < sample_count; sample_index++) {
		/* Resolve sample configurations */
		stage_count = config[0];
		block_size = config[1];

		/* Initialise instance */
#if defined(CONFIG_ARMV8_1_M_MVEF) && defined(CONFIG_FPU)
		arm_biquad_cascade_df1_mve_init_f32(&inst, stage_count, coeff, coeff_mod, state);
#else
		arm_biquad_cascade_df1_init_f32(&inst, stage_count, coeff, state);
#endif

		/* Run test function */
		arm_biquad_cascade_df1_f32(&inst, input, output, block_size);

		/* Increment pointers */
		input += block_size;
		output += block_size;
		coeff += stage_count * 5;
		config += 2;
	}

	/* Validate output */
	zassert_true(
		test_snr_error_f32(length, output_buf, ref, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_rel_error_f32(length, output_buf, ref, REL_ERROR_THRESH),
		ASSERT_MSG_REL_ERROR_LIMIT_EXCEED);

	/* Free buffers */
	free(state);
	free(output_buf);
}

ZTEST(filtering_biquad_f32, test_arm_biquad_cascade_df2t_f32_rand)
{
	size_t sample_index, stage_count, block_size;
	size_t sample_count = ARRAY_SIZE(in_rand_config) / 2;
	size_t length = ARRAY_SIZE(ref_rand_mono);
	const uint16_t *config = in_rand_config;
	const float32_t *input = (const float32_t *)in_rand_mono_val;
	const float32_t *coeff = (const float32_t *)in_rand_coeff;
	const float32_t *ref = (const float32_t *)ref_rand_mono;
	float32_t *state, *output_buf, *output;
	arm_biquad_cascade_df2T_instance_f32 inst;

	/* Allocate buffers */
	state = malloc(128 * sizeof(float32_t));
	zassert_not_null(state, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	output_buf = malloc(length * sizeof(float32_t));
	zassert_not_null(output_buf, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	output = output_buf;

	/* Enumerate samples */
	for (sample_index = 0; sample_index < sample_count; sample_index++) {
		/* Resolve sample configurations */
		stage_count = config[0];
		block_size = config[1];

		/* Initialise instance */
		arm_biquad_cascade_df2T_init_f32(
			&inst, stage_count, coeff, state);

		/* TODO: Add NEON support */

		/* Run test function */
		arm_biquad_cascade_df2T_f32(&inst, input, output, block_size);

		/* Increment pointers */
		input += block_size;
		output += block_size;
		coeff += stage_count * 5;
		config += 2;
	}

	/* Validate output */
	zassert_true(
		test_snr_error_f32(length, output_buf, ref, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_rel_error_f32(length, output_buf, ref, REL_ERROR_THRESH),
		ASSERT_MSG_REL_ERROR_LIMIT_EXCEED);

	/* Free buffers */
	free(state);
	free(output_buf);
}

ZTEST(filtering_biquad_f32, test_arm_biquad_cascade_stereo_df2t_f32_rand)
{
	size_t sample_index, stage_count, block_size;
	size_t sample_count = ARRAY_SIZE(in_rand_config) / 2;
	size_t length = ARRAY_SIZE(ref_rand_stereo);
	const uint16_t *config = in_rand_config;
	const float32_t *input = (const float32_t *)in_rand_stereo_val;
	const float32_t *coeff = (const float32_t *)in_rand_coeff;
	const float32_t *ref = (const float32_t *)ref_rand_stereo;
	float32_t *state, *output_buf, *output;
	arm_biquad_cascade_stereo_df2T_instance_f32 inst;

	/* Allocate buffers */
	state = malloc(128 * sizeof(float32_t));
	zassert_not_null(state, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	output_buf = malloc(length * sizeof(float32_t));
	zassert_not_null(output_buf, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	output = output_buf;

	/* Enumerate samples */
	for (sample_index = 0; sample_index < sample_count; sample_index++) {
		/* Resolve sample configurations */
		stage_count = config[0];
		block_size = config[1];

		/* Initialise instance */
		arm_biquad_cascade_stereo_df2T_init_f32(
			&inst, stage_count, coeff, state);

		/* Run test function */
		arm_biquad_cascade_stereo_df2T_f32(
			&inst, input, output, block_size);

		/* Increment pointers */
		input += 2 * block_size;
		output += 2 * block_size;
		coeff += stage_count * 5;
		config += 2;
	}

	/* Validate output */
	zassert_true(
		test_snr_error_f32(length, output_buf, ref, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_rel_error_f32(length, output_buf, ref, REL_ERROR_THRESH),
		ASSERT_MSG_REL_ERROR_LIMIT_EXCEED);

	/* Free buffers */
	free(state);
	free(output_buf);
}

ZTEST_SUITE(filtering_biquad_f32, NULL, NULL, NULL, NULL, NULL);
