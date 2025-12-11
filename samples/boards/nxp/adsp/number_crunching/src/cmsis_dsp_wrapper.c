/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <stdio.h>

#include "arm_math.h"

void vec_sum_int16(const int16_t *in_a, const int16_t *in_b, int16_t *out, uint32_t length)
{
	printk("[Backend] CMSIS-DSP module\n");
	arm_add_q15(in_a, in_b, out, length);
}

void vec_power_int16(const int16_t *in, int64_t *out, int rsh, uint32_t length)
{
	printk("[Backend] CMSIS-DSP module\n");
	arm_power_q15(in, length, out);
}

void vec_power_int32(const int32_t *in, int64_t *out, int rsh, uint32_t length)
{
	printk("[Backend] CMSIS-DSP module\n");
	arm_power_q31(in, length, out);
}

void fft_real32(int32_t *in, int32_t *out, int length)
{
	/* Instance structure for the Q31 RFFT */
	arm_rfft_instance_q31 rFFT;

	printk("[Backend] CMSIS-DSP module\n");

	/*
	 * Initialize the FFT
	 * value = 0: forward transform
	 * value = 1: enables bit reversal of output
	 */
	arm_rfft_init_q31(&rFFT, length, 0, 1);
	/* Apply FFT */
	arm_rfft_q31(&rFFT, in, out);
}

void real_block_iir_32(int M, const int32_t *coef_sos, const int16_t *coef_g,
		       const int32_t *in, int32_t *out, int block_size)
{
	/* Instance of the Q31 Biquad cascade structure */
	arm_biquad_casd_df1_inst_q31 handle;
	/*
	 * State variables array
	 * Each Bi-quad stage has 4 state variables.
	 * The state array has a total length of 4*M values.
	 */
	q31_t biquadStateBandQ31[4 * M];

	printk("[Backend] CMSIS-DSP module\n");

	/*
	 * Initialize the state and coefficient buffers for all Bi-quad sections
	 * value = 2: Shift to be applied after the accumulator
	 */
	arm_biquad_cascade_df1_init_q31(&handle, M, coef_sos, &biquadStateBandQ31[0], 2);
	/* Call the Q31 Bi-quad Cascade DF1 process function */
	arm_biquad_cascade_df1_q31(&handle, in, out, block_size);
}

void lms_iir_32(int32_t *err, int32_t *coef, int32_t *input, int32_t *ref,
		int32_t mu, int block_size, int M)
{
	arm_lms_instance_q31 handle;

	printk("[Backend] CMSIS-DSP module\n");
	arm_lms_init_q31(&handle, M, coef, ref, mu, block_size, 0);
	arm_lms_q31(&handle, input, ref, coef, err, block_size);
}
