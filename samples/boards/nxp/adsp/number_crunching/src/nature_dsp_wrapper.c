/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <stdio.h>

#include "NatureDSP_Signal.h"

/**
 * For fft_real32x32, scaling options are:
 * 2 - 32-bit dynamic scaling
 * 3 - fixed scaling before each stage
 */
#define FFT_SCALING_OPTION 3
#define RSH 31

void vec_sum_int16(const int16_t *in_a, const int16_t *in_b, int16_t *out, uint32_t length)
{
	printk("[Backend] NatureDSP library\n");
	vec_add16x16(out, in_a, in_b, length);
}

void vec_power_int16(const int16_t *in, int64_t *out, int rsh, uint32_t length)
{
	printk("[Backend] NatureDSP library\n");
	out[0] = vec_power16x16(in, rsh, length);
}

void vec_power_int32(const int32_t *in, int64_t *out, int rsh, uint32_t length)
{
	printk("[Backend] NatureDSP library\n");
	out[0] = vec_power32x32(in, rsh, length);
}

void fft_real32(int32_t *in, int32_t *out, int length)
{
	/* FFT handle for 32x32 */
	extern const fft_handle_t rfft32_32;

	printk("[Backend] NatureDSP library\n");

	/* Apply FFT */
	fft_real32x32(out, in, rfft32_32, FFT_SCALING_OPTION);
}

void real_block_iir_32(int M, const int32_t *coef_sos, const int16_t *coef_g,
		       const int32_t *in, int32_t *out, int block_size)
{
	bqriir32x32_df1_handle_t handle;
	static int32_t objmem[256] = {};

	printk("[Backend] NatureDSP library\n");
	/*
	 * Initialization routine for IIR filters
	 * value = 0: total gain shift amount applied to output signal
	 *
	 * Returns: handle to the object
	 */
	handle = bqriir32x32_df1_init(objmem, M, coef_sos, coef_g, 0);
	/*
	 * Call Bi-quad Real Block IIR
	 * value = NULL: scratch memory area (for fixed-point functions only)
	 *
	 * The filter calculates block_size output samples using
	 * coef_sos and coef_g matrices.
	 */
	bqriir32x32_df1(handle, NULL, out, in, block_size);
}

void lms_iir_32(int32_t *err, int32_t *coef, int32_t *input, int32_t *ref,
		int32_t mu, int block_size, int M)
{
	int64_t norm64;

	printk("[Backend] NatureDSP library\n");

	/*
	 * Compute the normalization factor,
	 * which is the power of the reference signal times block_size,
	 * where block_size is the number of samples to process
	 */
	vec_power_int32(ref, &norm64, RSH, block_size);
	/*
	 * Call Blockwise Adaptive LMS Algorithm for Real Data
	 * value = NULL: scratch memory area (for fixed-point functions only)
	 *
	 * The filter calculates block_size output samples using
	 * coef_sos and coef_g matrices.
	 */
	fir_blms32x32(err, coef, input, ref, norm64, mu, block_size, M);
}
