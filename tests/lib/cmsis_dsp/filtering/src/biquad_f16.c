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
#include "../../common/test_common.h"

#include "biquad_f16.pat"

#define SNR_ERROR_THRESH	((float32_t)27)
#define REL_ERROR_THRESH	(5.0e-2)
#define ABS_ERROR_THRESH	(1.0e-1)

static void test_arm_biquad_cascade_df1_f16_default(void)
{
	size_t index;
	size_t length = ARRAY_SIZE(ref_default);
	size_t block_size = length / 2;
	const float16_t *input = (const float16_t *)in_default_val;
	const float16_t *coeff = (const float16_t *)in_default_coeff;
	const float16_t *ref = (const float16_t *)ref_default;
	float16_t *state, *output_buf, *output;
	arm_biquad_casd_df1_inst_f16 inst;
#if defined(CONFIG_ARMV8_1_M_MVEF) && defined(CONFIG_FPU)
	arm_biquad_mod_coef_f16 *coeff_mod;
#endif

	/* Allocate buffers */
	state = calloc(128, sizeof(float16_t));
	zassert_not_null(state, ASSERT_MSG_BUFFER_ALLOC_FAILED);

#if defined(CONFIG_ARMV8_1_M_MVEF) && defined(CONFIG_FPU)
	coeff_mod = calloc(47, sizeof(arm_biquad_mod_coef_f16)); /* 47 stages */
	zassert_not_null(coeff_mod, ASSERT_MSG_BUFFER_ALLOC_FAILED);
#endif

	/* FIXME: `length + 2` is required here because of ARM-software/CMSIS_5#1475 */
	output_buf = calloc(length + 2, sizeof(float16_t));
	zassert_not_null(output_buf, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	output = output_buf;

	/* Initialise instance */
#if defined(CONFIG_ARMV8_1_M_MVEF) && defined(CONFIG_FPU)
	arm_biquad_cascade_df1_mve_init_f16(&inst, 3, coeff, coeff_mod, state);
#else
	arm_biquad_cascade_df1_init_f16(&inst, 3, coeff, state);
#endif

	/* Enumerate blocks */
	for (index = 0; index < 2; index++) {
		/* Run test function */
		arm_biquad_cascade_df1_f16(&inst, input, output, block_size);

		/* Increment pointers */
		input += block_size;
		output += block_size;
	}

	/* Validate output */
	zassert_true(
		test_snr_error_f16(length, output_buf, ref, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_close_error_f16(length, ref, output_buf,
			ABS_ERROR_THRESH, REL_ERROR_THRESH),
		ASSERT_MSG_ERROR_LIMIT_EXCEED);

	/* Free buffers */
	free(state);
	free(output_buf);
}

static void test_arm_biquad_cascade_df2t_f16_default(void)
{
	size_t index;
	size_t length = ARRAY_SIZE(ref_default);
	size_t block_size = length / 2;
	const float16_t *input = (const float16_t *)in_default_val;
	const float16_t *coeff = (const float16_t *)in_default_coeff;
	const float16_t *ref = (const float16_t *)ref_default;
	float16_t *state, *output_buf, *output;
	arm_biquad_cascade_df2T_instance_f16 inst;

	/* Allocate buffers */
	state = malloc(128 * sizeof(float16_t));
	zassert_not_null(state, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	output_buf = malloc(length * sizeof(float16_t));
	zassert_not_null(output_buf, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	output = output_buf;

	/* Initialise instance */
	arm_biquad_cascade_df2T_init_f16(&inst, 3, coeff, state);

	/* TODO: Add NEON support */

	/* Enumerate blocks */
	for (index = 0; index < 2; index++) {
		/* Run test function */
		arm_biquad_cascade_df2T_f16(&inst, input, output, block_size);

		/* Increment pointers */
		input += block_size;
		output += block_size;
	}

	/* Validate output */
	zassert_true(
		test_snr_error_f16(length, output_buf, ref, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_close_error_f16(length, ref, output_buf,
			ABS_ERROR_THRESH, REL_ERROR_THRESH),
		ASSERT_MSG_ERROR_LIMIT_EXCEED);

	/* Free buffers */
	free(state);
	free(output_buf);
}

static void test_arm_biquad_cascade_df1_f16_rand(void)
{
	size_t sample_index, stage_count, block_size;
	size_t sample_count = ARRAY_SIZE(in_rand_config) / 2;
	size_t length = ARRAY_SIZE(ref_rand_mono);
	const uint16_t *config = in_rand_config;
	const float16_t *input = (const float16_t *)in_rand_mono_val;
	const float16_t *coeff = (const float16_t *)in_rand_coeff;
	const float16_t *ref = (const float16_t *)ref_rand_mono;
	float16_t *state, *output_buf, *output;
	arm_biquad_casd_df1_inst_f16 inst;
#if defined(CONFIG_ARMV8_1_M_MVEF) && defined(CONFIG_FPU)
	arm_biquad_mod_coef_f16 *coeff_mod;
#endif

	/* Allocate buffers */
	state = calloc(128, sizeof(float16_t));
	zassert_not_null(state, ASSERT_MSG_BUFFER_ALLOC_FAILED);

#if defined(CONFIG_ARMV8_1_M_MVEF) && defined(CONFIG_FPU)
	coeff_mod = calloc(47, sizeof(arm_biquad_mod_coef_f16)); /* 47 stages */
	zassert_not_null(coeff_mod, ASSERT_MSG_BUFFER_ALLOC_FAILED);
#endif

	output_buf = calloc(length, sizeof(float16_t));
	zassert_not_null(output_buf, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	output = output_buf;

	/* Enumerate samples */
	for (sample_index = 0; sample_index < sample_count; sample_index++) {
		/* Resolve sample configurations */
		stage_count = config[0];
		block_size = config[1];

		/* Initialise instance */
#if defined(CONFIG_ARMV8_1_M_MVEF) && defined(CONFIG_FPU)
		arm_biquad_cascade_df1_mve_init_f16(&inst, stage_count, coeff, coeff_mod, state);
#else
		arm_biquad_cascade_df1_init_f16(&inst, stage_count, coeff, state);
#endif

		/* Run test function */
		arm_biquad_cascade_df1_f16(&inst, input, output, block_size);

		/* Increment pointers */
		input += block_size;
		output += block_size;
		coeff += stage_count * 5;
		config += 2;
	}

	/* Validate output */
	zassert_true(
		test_snr_error_f16(length, output_buf, ref, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_close_error_f16(length, ref, output_buf,
			ABS_ERROR_THRESH, REL_ERROR_THRESH),
		ASSERT_MSG_ERROR_LIMIT_EXCEED);

	/* Free buffers */
	free(state);
	free(output_buf);
}

static void test_arm_biquad_cascade_df2t_f16_rand(void)
{
	size_t sample_index, stage_count, block_size;
	size_t sample_count = ARRAY_SIZE(in_rand_config) / 2;
	size_t length = ARRAY_SIZE(ref_rand_mono);
	const uint16_t *config = in_rand_config;
	const float16_t *input = (const float16_t *)in_rand_mono_val;
	const float16_t *coeff = (const float16_t *)in_rand_coeff;
	const float16_t *ref = (const float16_t *)ref_rand_mono;
	float16_t *state, *output_buf, *output;
	arm_biquad_cascade_df2T_instance_f16 inst;

	/* Allocate buffers */
	state = malloc(128 * sizeof(float16_t));
	zassert_not_null(state, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	output_buf = malloc(length * sizeof(float16_t));
	zassert_not_null(output_buf, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	output = output_buf;

	/* Enumerate samples */
	for (sample_index = 0; sample_index < sample_count; sample_index++) {
		/* Resolve sample configurations */
		stage_count = config[0];
		block_size = config[1];

		/* Initialise instance */
		arm_biquad_cascade_df2T_init_f16(
			&inst, stage_count, coeff, state);

		/* TODO: Add NEON support */

		/* Run test function */
		arm_biquad_cascade_df2T_f16(&inst, input, output, block_size);

		/* Increment pointers */
		input += block_size;
		output += block_size;
		coeff += stage_count * 5;
		config += 2;
	}

	/* Validate output */
	zassert_true(
		test_snr_error_f16(length, output_buf, ref, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_close_error_f16(length, ref, output_buf,
			ABS_ERROR_THRESH, REL_ERROR_THRESH),
		ASSERT_MSG_ERROR_LIMIT_EXCEED);

	/* Free buffers */
	free(state);
	free(output_buf);
}

static void test_arm_biquad_cascade_stereo_df2t_f16_rand(void)
{
	size_t sample_index, stage_count, block_size;
	size_t sample_count = ARRAY_SIZE(in_rand_config) / 2;
	size_t length = ARRAY_SIZE(ref_rand_stereo);
	const uint16_t *config = in_rand_config;
	const float16_t *input = (const float16_t *)in_rand_stereo_val;
	const float16_t *coeff = (const float16_t *)in_rand_coeff;
	const float16_t *ref = (const float16_t *)ref_rand_stereo;
	float16_t *state, *output_buf, *output;
	arm_biquad_cascade_stereo_df2T_instance_f16 inst;

	/* Allocate buffers */
	state = malloc(128 * sizeof(float16_t));
	zassert_not_null(state, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	output_buf = malloc(length * sizeof(float16_t));
	zassert_not_null(output_buf, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	output = output_buf;

	/* Enumerate samples */
	for (sample_index = 0; sample_index < sample_count; sample_index++) {
		/* Resolve sample configurations */
		stage_count = config[0];
		block_size = config[1];

		/* Initialise instance */
		arm_biquad_cascade_stereo_df2T_init_f16(
			&inst, stage_count, coeff, state);

		/* Run test function */
		arm_biquad_cascade_stereo_df2T_f16(
			&inst, input, output, block_size);

		/* Increment pointers */
		input += 2 * block_size;
		output += 2 * block_size;
		coeff += stage_count * 5;
		config += 2;
	}

	/* Validate output */
	zassert_true(
		test_snr_error_f16(length, output_buf, ref, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_close_error_f16(length, ref, output_buf,
			ABS_ERROR_THRESH, REL_ERROR_THRESH),
		ASSERT_MSG_ERROR_LIMIT_EXCEED);

	/* Free buffers */
	free(state);
	free(output_buf);
}

void test_filtering_biquad_f16(void)
{
	ztest_test_suite(filtering_biquad_f16,
		ztest_unit_test(test_arm_biquad_cascade_df1_f16_default),
		ztest_unit_test(test_arm_biquad_cascade_df2t_f16_default),
		ztest_unit_test(test_arm_biquad_cascade_df1_f16_rand),
		ztest_unit_test(test_arm_biquad_cascade_df2t_f16_rand),
		ztest_unit_test(test_arm_biquad_cascade_stereo_df2t_f16_rand)
		);

	ztest_run_test_suite(filtering_biquad_f16);
}
