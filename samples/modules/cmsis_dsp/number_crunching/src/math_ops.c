/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <stdlib.h>

#include "math_ops.h"
#include "input.h"

static int start, stop;

static inline int test_near_equal_q15(uint32_t length, const int16_t *in_a,
				      const int16_t *in_b, int16_t threshold)
{
	int i;

	for (i = 0; i < length; i++) {
		if (abs(in_a[i] - in_b[i]) > threshold) {
			return 0;
		}
	}

	return 1;
}

void test_vec_sum_int16_op(void)
{
	int16_t output[VEC_LENGTH];
	int ret = 1; /* test passed successfully */

	printk("[Library Test] == Vector Sum test  ==\r\n");
	start = k_cycle_get_32();

	/* Run test function */
	vec_sum_int16(in_a, in_b, output, VEC_LENGTH);
	stop = k_cycle_get_32();

	/* Validate output */
	ret = test_near_equal_q15(VEC_LENGTH, output, ref_add, ABS_ERROR_THRESH_Q15);

	printk("[Library Test] Vector Sum takes %d cycles\r\n", stop - start);
	printk("[Library Test] == Vector Sum test end with %d ==\r\n\r\n", ret);
}

void test_power_int16_op(void)
{
	int64_t output;
	int ret = 1; /* test passed successfully */

	printk("[Library Test] == Vector power sum test  ==\r\n");
	start = k_cycle_get_32();

	/* Run test function */
	vec_power_int16(in_a, &output, 0, VEC_LENGTH);
	stop = k_cycle_get_32();

	/* Validate output */
	if (output != ref_power_16[0]) {
		printk("[Library Test] Mismatch: expected %lld result %lld\r\n",
		       ref_power_16[0], output);
		ret = 0; /* test failed */
	}

	printk("[Library Test] Vector power sum takes %d cycles\r\n", stop - start);
	printk("[Library Test] == Vector power sum test end with %d ==\r\n\r\n", ret);
}

void test_power_int32_op(void)
{
	int64_t output;

	printk("[Library Test] == Vector power sum test  ==\r\n");
	start = k_cycle_get_32();

	/* Run test function */
	vec_power_int32(fir_in_ref, &output, RSH, FIR_LENGTH);
	stop = k_cycle_get_32();

	printk("[Library Test] Vector power sum takes %d cycles\r\n", stop - start);
	printk("[Library Test] == Vector power sum test end ==\r\n\r\n");
}

void test_fft_op(void)
{
	int i;
	int32_t fft_in[FFT_LENGTH];

	/* Create input */
	for (i = 0; i < FFT_LENGTH; i++)
		fft_in[i] = FFT_LENGTH * (1 + i % 2); /* only real part */

	printk("[Library Test] == Fast Fourier Transform on Real Data test  ==\r\n");
	start = k_cycle_get_32();

	/* Run test function */
	fft_real32(fft_in, fft_out, FFT_LENGTH);
	stop = k_cycle_get_32();

	printk("[Library Test] Fast Fourier Transform on Real Data takes %d cycles\r\n",
	       stop - start);
	printk("[Library Test] == Fast Fourier Transform on Real Data test end ==\r\n\r\n");
}

void test_iir_op(void)
{
	printk("[Library Test] == Bi-quad Real Block IIR test  ==\r\n");
	start = k_cycle_get_32();

	/* Run test function */
	real_block_iir_32(IIR_M, coef_sos, coef_g, iir_in, iir_out, IIR_LENGTH);
	stop = k_cycle_get_32();

	printk("[Library Test] Bi-quad Real Block IIR takes %d cycles\r\n", stop - start);
	printk("[Library Test] == Bi-quad Real Block IIR end ==\r\n\r\n");
}

void test_fir_blms_op(void)
{
	int i;
	int32_t fir_coef[FIR_M];
	int32_t fir_in[FIR_LENGTH];
	int32_t fir_ref[FIR_LENGTH + FIR_M];

	for (i = 0; i < FIR_M; i++)
		fir_coef[i] = fir_coef_ref[i];
	for (i = 0; i < FIR_LENGTH; i++)
		fir_in[i] = fir_in_ref[i];
	for (i = 0; i < FIR_LENGTH + FIR_M; i++)
		fir_ref[i] = fir_ref_ref[i];

	printk("[Library Test] == Least Mean Square (LMS) Filter for Real Data test  ==\r\n");
	start = k_cycle_get_32();

	/* Run test function */
	lms_iir_32(fir_err, fir_coef, fir_in, fir_ref, MU, FIR_LENGTH, FIR_M);
	stop = k_cycle_get_32();

	printk("[Library Test] Least Mean Square (LMS) Filter for Real Data test takes %d cycles\r\n",
	       stop - start);
	printk("[Library Test] == Least Mean Square (LMS) Filter for Real Data test end ==\r\n\r\n");
}
