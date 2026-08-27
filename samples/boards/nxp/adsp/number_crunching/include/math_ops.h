/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

/* test vector sum on int16_t elements */
void test_vec_sum_int16_op(void);
/**
 * @brief Vector Sum - makes pair wise saturated summation of vectors
 *
 * @param[in]		*input1 - points to the first input vector
 * @param[in]		*input2 - points to the second input vector
 * @param[out]		*output - points to the output vector
 * @param[in]		length - number of samples in each vector
 *
 * @return none
 */
void vec_sum_int16(const int16_t *input1, const int16_t *input2,
		   int16_t *output, uint32_t length);

/* test sum of the squares of the int16_t vector elements */
void test_power_int16_op(void);
/**
 * @brief Power of a Vector - makes sum of the squares of the elements of
 *			      an int16_t vector
 *
 * @param[in]		*input - points to the input vector
 * @param[in]		length - size of the input vector
 * @param[in]		rsh - right shift of result
 * @param[out]		*output - sum of the squares value
 *
 * @return none
 *
 * @details
 * For NatureDSP, rsh is in range 0...31 and output may scaled down with
 * saturation by rsh bits.
 *
 * For CMSIS-DSP, intermediate multiplication yields a 2.30 format,
 * and this result is added without saturation to a 64-bit accumulator
 * in 34.30 format.
 *
 * Therefore, for some cases, rsh is not used, it can be ignored.
 */
void vec_power_int16(const int16_t *input, int64_t *output, int rsh, uint32_t length);

/* test sum of the squares of the int32_t vector elements */
void test_power_int32_op(void);
/**
 * @brief Power of a Vector - makes sum of the squares of the elements of
 *			      an int32_t vector
 *
 * @param[in]		*input - points to the input vector
 * @param[in]		length - size of the input vector
 * @param[in]		rsh - right shift of result
 * @param[out]		*output - sum of the squares value
 *
 * @return none
 *
 * @details
 * For NatureDSP, rsh is in range 31...62 and output may scaled down with
 * saturation by rsh bits.
 *
 * For CMSIS-DSP, intermediate multiplication yields a 2.62 format,
 * and this result is truncated to 2.48 format by discarding the lower 14 bits.
 * The 2.48 result is then added without saturation to a 64-bit accumulator
 * in 16.48 format.
 *
 * Therefore, for some cases, rsh is not used, it can be ignored.
 */
void vec_power_int32(const int32_t *input, int64_t *output, int rsh, uint32_t length);

/* test Fast Fourier Transform (FFT) */
void test_fft_op(void);
/**
 * @brief Fast Fourier Transform on Real Data - make FFT on real data
 *
 * @param[in]		*input - points to the input buffer
 * @param[in]		length - length of the FFT
 * @param[out]		*output - points to the output buffer
 *
 * @return none
 */
void fft_real32(int32_t *input, int32_t *output, int length);

/* test Bi-quad Real Block Infinite Impulse Response (IIR) Filter */
void test_iir_op(void);
/**
 * @brief Bi-quad Real Block IIR - makes a real IIR filter
 *				   (cascaded IIR direct form I using
 *				   5 coefficients per bi-quad + gain term)
 *
 * @param[in]		M - number of bi-quad sections
 * @param[in]		*coef_sos - points to the filter coefficients
 * @param[in]		*coef_g - points to the scale factor for each section
 * @param[in]		*input - points to the block of input data
 * @param[in]		blockSize - number of samples to process
 * @param[out]		*output - points to the block of output data
 *
 * @return none
 */
void real_block_iir_32(int M, const int32_t *coef_sos, const int16_t *coef_g,
		       const int32_t *input, int32_t *output, int block_size);

/* test Least Mean Square (LMS) Filter for Real Data */
void test_fir_blms_op(void);
/**
 * @brief Blockwise Adaptive LMS Algorithm for Real Data - performs filtering of
 *				reference samples 'ref', computation of
 *				error 'err' over a block of
 *				input samples 'input'
 *
 * @param[in]		*coef - points to coefficient buffer
 * @param[in]		*input -points to the block of input data
 * @param[in]		*ref - points to the block of reference data
 * @param[in]		mu - step size that controls filter coefficient updates
 * @param[in]		blockSize - number of samples to process
 * @param[in]		M - number of filter coefficients
 * @param[out]		*output - points to the block of output data
 * @param[out]		*err - points to the block of error data
 *
 * @return	none
 */
void lms_iir_32(int32_t *err, int32_t *coef, int32_t *input, int32_t *ref,
		int32_t mu, int block_size, int M);
