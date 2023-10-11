/* Copyright (C) 2010-2021 ARM Limited or its affiliates. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file zephyr/dsp/basicmath.h
 *
 * @brief Public APIs for DSP basicmath
 */

#ifndef INCLUDE_ZEPHYR_DSP_BASICMATH_H_
#define INCLUDE_ZEPHYR_DSP_BASICMATH_H_

#include <zephyr/dsp/dsp.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ingroup math_dsp
 * @defgroup math_dsp_basic Basic Math Functions
 */

/**
 * @ingroup math_dsp_basic
 * @addtogroup math_dsp_basic_mult Vector Multiplication
 *
 * Element-by-element multiplication of two vectors.
 * <pre>
 *     dst[n] = src_a[n] * src_b[n],   0 <= n < block_size.
 * </pre>
 * There are separate functions for floating-point, Q7, Q15, and Q31 data types.
 * @{
 */

/**
 * @brief Q7 vector multiplication.
 *
 * @par Scaling and Overflow Behavior
 *   The function uses saturating arithmetic.
 *   Results outside of the allowable Q7 range [0x80 0x7F] are saturated.
 *
 * @param[in]  src_a      points to the first input vector
 * @param[in]  src_b      points to the second input vector
 * @param[out] dst        points to the output vector
 * @param[in]  block_size number of samples in each vector
 */
DSP_FUNC_SCOPE void zdsp_mult_q7(const q7_t *src_a, const q7_t *src_b, q7_t *dst,
				 uint32_t block_size);

/**
 * @brief Q15 vector multiplication.
 *
 * @par Scaling and Overflow Behavior
 *   The function uses saturating arithmetic.
 *   Results outside of the allowable Q15 range [0x8000 0x7FFF] are saturated.
 *
 * @param[in]  src_a      points to the first input vector
 * @param[in]  src_b      points to the second input vector
 * @param[out] dst        points to the output vector
 * @param[in]  block_size number of samples in each vector
 */
DSP_FUNC_SCOPE void zdsp_mult_q15(const q15_t *src_a, const q15_t *src_b, q15_t *dst,
				  uint32_t block_size);

/**
 * @brief Q31 vector multiplication.
 *
 * @par Scaling and Overflow Behavior
 *   The function uses saturating arithmetic.
 *   Results outside of the allowable Q31 range[0x80000000 0x7FFFFFFF] are saturated.
 *
 * @param[in]  src_a      points to the first input vector
 * @param[in]  src_b      points to the second input vector
 * @param[out] dst        points to the output vector
 * @param[in]  block_size number of samples in each vector
 */
DSP_FUNC_SCOPE void zdsp_mult_q31(const q31_t *src_a, const q31_t *src_b, q31_t *dst,
				  uint32_t block_size);

/**
 * @brief Floating-point vector multiplication.
 * @param[in]  src_a      points to the first input vector
 * @param[in]  src_b      points to the second input vector
 * @param[out] dst        points to the output vector
 * @param[in]  block_size number of samples in each vector
 */
DSP_FUNC_SCOPE void zdsp_mult_f32(const float32_t *src_a, const float32_t *src_b, float32_t *dst,
				  uint32_t block_size);

/**
 * @}
 */

/**
 * @ingroup math_dsp_basic
 * @addtogroup math_dsp_basic_add Vector Addition
 *
 * Element-by-element addition of two vectors.
 * <pre>
 *     dst[n] = src_a[n] + src_b[n],   0 <= n < block_size.
 * </pre>
 * There are separate functions for floating-point, Q7, Q15, and Q31 data types.
 * @{
 */

/**
 * @brief Floating-point vector addition.
 * @param[in]  src_a      points to the first input vector
 * @param[in]  src_b      points to the second input vector
 * @param[out] dst        points to the output vector
 * @param[in]  block_size number of samples in each vector
 */
DSP_FUNC_SCOPE void zdsp_add_f32(const float32_t *src_a, const float32_t *src_b, float32_t *dst,
				 uint32_t block_size);

/**
 * @brief Q7 vector addition.
 *
 * @par Scaling and Overflow Behavior
 *   The function uses saturating arithmetic.
 *   Results outside of the allowable Q7 range [0x80 0x7F] are saturated.
 *
 * @param[in]  src_a      points to the first input vector
 * @param[in]  src_b      points to the second input vector
 * @param[out] dst        points to the output vector
 * @param[in]  block_size number of samples in each vector
 */
DSP_FUNC_SCOPE void zdsp_add_q7(const q7_t *src_a, const q7_t *src_b, q7_t *dst,
				uint32_t block_size);

/**
 * @brief Q15 vector addition.
 *
 * @par Scaling and Overflow Behavior
 *   The function uses saturating arithmetic.
 *   Results outside of the allowable Q15 range [0x8000 0x7FFF] are saturated.
 *
 * @param[in]  src_a      points to the first input vector
 * @param[in]  src_b      points to the second input vector
 * @param[out] dst        points to the output vector
 * @param[in]  block_size number of samples in each vector
 */
DSP_FUNC_SCOPE void zdsp_add_q15(const q15_t *src_a, const q15_t *src_b, q15_t *dst,
				 uint32_t block_size);

/**
 * @brief Q31 vector addition.
 *
 * @par Scaling and Overflow Behavior
 *   The function uses saturating arithmetic.
 *   Results outside of the allowable Q31 range [0x80000000 0x7FFFFFFF] are saturated.
 *
 * @param[in]  src_a      points to the first input vector
 * @param[in]  src_b      points to the second input vector
 * @param[out] dst        points to the output vector
 * @param[in]  block_size number of samples in each vector
 */
DSP_FUNC_SCOPE void zdsp_add_q31(const q31_t *src_a, const q31_t *src_b, q31_t *dst,
				 uint32_t block_size);

/**
 * @}
 */

/**
 * @ingroup math_dsp_basic
 * @addtogroup math_dsp_basic_sub Vector Subtraction
 *
 * Element-by-element subtraction of two vectors.
 * <pre>
 *     dst[n] = src_a[n] - src_b[n],   0 <= n < block_size.
 * </pre>
 * There are separate functions for floating-point, Q7, Q15, and Q31 data types.
 * @{
 */

/**
 * @brief Floating-point vector subtraction.
 * @param[in]  src_a      points to the first input vector
 * @param[in]  src_b      points to the second input vector
 * @param[out] dst        points to the output vector
 * @param[in]  block_size number of samples in each vector
 */
DSP_FUNC_SCOPE void zdsp_sub_f32(const float32_t *src_a, const float32_t *src_b, float32_t *dst,
				 uint32_t block_size);

/**
 * @brief Q7 vector subtraction.
 *
 * @par Scaling and Overflow Behavior
 *   The function uses saturating arithmetic.
 *   Results outside of the allowable Q7 range [0x80 0x7F] will be saturated.
 *
 * @param[in]  src_a      points to the first input vector
 * @param[in]  src_b      points to the second input vector
 * @param[out] dst        points to the output vector
 * @param[in]  block_size number of samples in each vector
 */
DSP_FUNC_SCOPE void zdsp_sub_q7(const q7_t *src_a, const q7_t *src_b, q7_t *dst,
				uint32_t block_size);

/**
 * @brief Q15 vector subtraction.
 *
 * @par Scaling and Overflow Behavior
 *   The function uses saturating arithmetic.
 *   Results outside of the allowable Q15 range [0x8000 0x7FFF] are saturated.
 *
 * @param[in]  src_a      points to the first input vector
 * @param[in]  src_b      points to the second input vector
 * @param[out] dst        points to the output vector
 * @param[in]  block_size number of samples in each vector
 */
DSP_FUNC_SCOPE void zdsp_sub_q15(const q15_t *src_a, const q15_t *src_b, q15_t *dst,
				 uint32_t block_size);

/**
 * @brief Q31 vector subtraction.
 *
 * @par Scaling and Overflow Behavior
 *   The function uses saturating arithmetic.
 *   Results outside of the allowable Q31 range [0x80000000 0x7FFFFFFF] are saturated.
 *
 * @param[in]  src_a      points to the first input vector
 * @param[in]  src_b      points to the second input vector
 * @param[out] dst        points to the output vector
 * @param[in]  block_size number of samples in each vector
 */
DSP_FUNC_SCOPE void zdsp_sub_q31(const q31_t *src_a, const q31_t *src_b, q31_t *dst,
				 uint32_t block_size);

/**
 * @}
 */

/**
 * @ingroup math_dsp_basic
 * @addtogroup math_dsp_basic_scale Vector Scale
 *
 * Multiply a vector by a scalar value. For floating-point data, the algorithm used is:
 * <pre>
 *     dst[n] = src[n] * scale,   0 <= n < block_size.
 * </pre>
 *
 * In the fixed-point Q7, Q15, and Q31 functions, scale is represented by a fractional
 * multiplication <code>scale_fract</code> and an arithmetic shift <code>shift</code>. The shift
 * allows the gain of the scaling operation to exceed 1.0. The algorithm used with fixed-point data
 * is:
 * <pre>
 *     dst[n] = (src[n] * scale_fract) << shift,   0 <= n < block_size.
 * </pre>
 *
 * The overall scale factor applied to the fixed-point data is
 * <pre>
 *     scale = scale_fract * 2^shift.
 * </pre>
 * The functions support in-place computation allowing the source and destination pointers to
 * reference the same memory buffer.
 * @{
 */

/**
 * @brief Multiplies a floating-point vector by a scalar.
 * @param[in]  src        points to the input vector
 * @param[in]  scale      scale factor to be applied
 * @param[out] dst        points to the output vector
 * @param[in]  block_size number of samples in the vector
 */
DSP_FUNC_SCOPE void zdsp_scale_f32(const float32_t *src, float32_t scale, float32_t *dst,
				   uint32_t block_size);

/**
 * @brief Multiplies a Q7 vector by a scalar.
 *
 * @par Scaling and Overflow Behavior
 *   The input data <code>*src</code> and <code>scale_fract</code> are in 1.7 format.
 *   These are multiplied to yield a 2.14 intermediate result and this is shifted with saturation to
 *   1.7 format.
 *
 * @param[in]  src         points to the input vector
 * @param[in]  scale_fract fractional portion of the scale value
 * @param[in]  shift       number of bits to shift the result by
 * @param[out] dst         points to the output vector
 * @param[in]  block_size  number of samples in the vector
 */
DSP_FUNC_SCOPE void zdsp_scale_q7(const q7_t *src, q7_t scale_fract, int8_t shift, q7_t *dst,
				  uint32_t block_size);

/**
 * @brief Multiplies a Q15 vector by a scalar.
 *
 * @par Scaling and Overflow Behavior
 *   The input data <code>*src</code> and <code>scale_fract</code> are in 1.15 format.
 *   These are multiplied to yield a 2.30 intermediate result and this is shifted with saturation to
 *   1.15 format.
 *
 * @param[in]  src         points to the input vector
 * @param[in]  scale_fract fractional portion of the scale value
 * @param[in]  shift       number of bits to shift the result by
 * @param[out] dst         points to the output vector
 * @param[in]  block_size  number of samples in the vector
 */
DSP_FUNC_SCOPE void zdsp_scale_q15(const q15_t *src, q15_t scale_fract, int8_t shift, q15_t *dst,
				   uint32_t block_size);

/**
 * @brief Multiplies a Q31 vector by a scalar.
 *
 * @par Scaling and Overflow Behavior
 *   The input data <code>*src</code> and <code>scale_fract</code> are in 1.31 format.
 *   These are multiplied to yield a 2.62 intermediate result and this is shifted with saturation to
 *   1.31 format.
 *
 * @param[in]  src         points to the input vector
 * @param[in]  scale_fract fractional portion of the scale value
 * @param[in]  shift       number of bits to shift the result by
 * @param[out] dst         points to the output vector
 * @param[in]  block_size  number of samples in the vector
 */
DSP_FUNC_SCOPE void zdsp_scale_q31(const q31_t *src, q31_t scale_fract, int8_t shift, q31_t *dst,
				   uint32_t block_size);

/**
 * @}
 */

/**
 * @ingroup math_dsp_basic
 * @addtogroup math_dsp_basic_abs Vector Absolute Value
 *
 * Computes the absolute value of a vector on an element-by-element basis.
 * <pre>
 *     dst[n] = abs(src[n]),   0 <= n < block_size.
 * </pre>
 * The functions support in-place computation allowing the source and destination pointers to
 * reference the same memory buffer. There are separate functions for floating-point, Q7, Q15, and
 * Q31 data types.
 * @{
 */

/**
 * @brief Floating-point vector absolute value.
 * @param[in]  src        points to the input buffer
 * @param[out] dst        points to the output buffer
 * @param[in]  block_size number of samples in each vector
 */
DSP_FUNC_SCOPE void zdsp_abs_f32(const float32_t *src, float32_t *dst, uint32_t block_size);

/**
 * @brief Q7 vector absolute value.
 *
 * @par Scaling and Overflow Behavior
 *   The function uses saturating arithmetic.
 *   The Q7 value -1 (0x80) will be saturated to the maximum allowable positive value 0x7F.
 *
 * @param[in]  src        points to the input buffer
 * @param[out] dst        points to the output buffer
 * @param[in]  block_size number of samples in each vector
 */
DSP_FUNC_SCOPE void zdsp_abs_q7(const q7_t *src, q7_t *dst, uint32_t block_size);

/**
 * @brief Q15 vector absolute value.
 *
 * @par Scaling and Overflow Behavior
 *   The function uses saturating arithmetic.
 *   The Q15 value -1 (0x8000) will be saturated to the maximum allowable positive value 0x7FFF.
 *
 * @param[in]  src        points to the input buffer
 * @param[out] dst        points to the output buffer
 * @param[in]  block_size number of samples in each vector
 */
DSP_FUNC_SCOPE void zdsp_abs_q15(const q15_t *src, q15_t *dst, uint32_t block_size);

/**
 * @brief Q31 vector absolute value.
 *
 * @par Scaling and Overflow Behavior
 *   The function uses saturating arithmetic.
 *   The Q31 value -1 (0x80000000) will be saturated to the maximum allowable positive value
 *   0x7FFFFFFF.
 *
 * @param[in]  src        points to the input buffer
 * @param[out] dst        points to the output buffer
 * @param[in]  block_size number of samples in each vector
 */
DSP_FUNC_SCOPE void zdsp_abs_q31(const q31_t *src, q31_t *dst, uint32_t block_size);

/**
 * @}
 */

/**
 * @ingroup math_dsp_basic
 * @addtogroup math_dsp_basic_dot Vector Dot Product
 *
 * Computes the dot product of two vectors. The vectors are multiplied element-by-element and then
 * summed.
 * <pre>
 *     sum = src_a[0]*src_b[0] + src_a[1]*src_b[1] + ... + src_a[block_size-1]*src_b[block_size-1]
 * </pre>
 * There are separate functions for floating-point, Q7, Q15, and Q31 data types.
 * @{
 */

/**
 * @brief Dot product of floating-point vectors.
 * @param[in]  src_a      points to the first input vector
 * @param[in]  src_b      points to the second input vector
 * @param[in]  block_size number of samples in each vector
 * @param[out] result     output result returned here
 */
DSP_FUNC_SCOPE void zdsp_dot_prod_f32(const float32_t *src_a, const float32_t *src_b,
				      uint32_t block_size, float32_t *result);

/**
 * @brief Dot product of Q7 vectors.
 *
 * @par Scaling and Overflow Behavior
 *   The intermediate multiplications are in 1.7 x 1.7 = 2.14 format and these results are added to
 *   an accumulator in 18.14 format. Nonsaturating additions are used and there is no danger of wrap
 *   around as long as the vectors are less than 2^18 elements long. The return result is in 18.14
 *   format.
 *
 * @param[in]  src_a      points to the first input vector
 * @param[in]  src_b      points to the second input vector
 * @param[in]  block_size number of samples in each vector
 * @param[out] result     output result returned here
 */
DSP_FUNC_SCOPE void zdsp_dot_prod_q7(const q7_t *src_a, const q7_t *src_b, uint32_t block_size,
				     q31_t *result);

/**
 * @brief Dot product of Q15 vectors.
 *
 * @par Scaling and Overflow Behavior
 *   The intermediate multiplications are in 1.15 x 1.15 = 2.30 format and these results are added
 *   to a 64-bit accumulator in 34.30 format. Nonsaturating additions are used and given that there
 *   are 33 guard bits in the accumulator there is no risk of overflow. The return result is in
 *   34.30 format.
 *
 * @param[in]  src_a      points to the first input vector
 * @param[in]  src_b      points to the second input vector
 * @param[in]  block_size number of samples in each vector
 * @param[out] result     output result returned here
 */
DSP_FUNC_SCOPE void zdsp_dot_prod_q15(const q15_t *src_a, const q15_t *src_b, uint32_t block_size,
				      q63_t *result);

/**
 * @brief Dot product of Q31 vectors.
 *
 * @par Scaling and Overflow Behavior
 *   The intermediate multiplications are in 1.31 x 1.31 = 2.62 format and these are truncated to
 *   2.48 format by discarding the lower 14 bits. The 2.48 result is then added without saturation
 *   to a 64-bit accumulator in 16.48 format. There are 15 guard bits in the accumulator and there
 *   is no risk of overflow as long as the length of the vectors is less than 2^16 elements. The
 *   return result is in 16.48 format.
 *
 * @param[in]  src_a      points to the first input vector
 * @param[in]  src_b      points to the second input vector
 * @param[in]  block_size number of samples in each vector
 * @param[out] result     output result returned here
 */
DSP_FUNC_SCOPE void zdsp_dot_prod_q31(const q31_t *src_a, const q31_t *src_b, uint32_t block_size,
				      q63_t *result);

/**
 * @}
 */

/**
 * @ingroup math_dsp_basic
 * @addtogroup math_dsp_basic_shift Vector Shift
 *
 * Shifts the elements of a fixed-point vector by a specified number of bits.
 * There are separate functions for Q7, Q15, and Q31 data types. The underlying algorithm used is:
 * <pre>
 *     dst[n] = src[n] << shift,   0 <= n < block_size.
 * </pre>
 * If <code>shift</code> is positive then the elements of the vector are shifted to the left.
 * If <code>shift</code> is negative then the elements of the vector are shifted to the right.
 *
 * The functions support in-place computation allowing the source and destination pointers to
 * reference the same memory buffer.
 * @{
 */

/**
 * @brief  Shifts the elements of a Q7 vector a specified number of bits.
 *
 * @par Scaling and Overflow Behavior
 *   The function uses saturating arithmetic.
 *   Results outside of the allowable Q7 range [0x80 0x7F] are saturated.
 *
 * @param[in]  src        points to the input vector
 * @param[in]  shift_bits number of bits to shift.  A positive value shifts left; a negative value
 *                        shifts right.
 * @param[out] dst        points to the output vector
 * @param[in]  block_size number of samples in the vector
 */
DSP_FUNC_SCOPE void zdsp_shift_q7(const q7_t *src, int8_t shift_bits, q7_t *dst,
				  uint32_t block_size);

/**
 * @brief  Shifts the elements of a Q15 vector a specified number of bits.
 *
 * @pre Scaling and Overflow Behavior
 *   The function uses saturating arithmetic.
 *   Results outside of the allowable Q15 range [0x8000 0x7FFF] are saturated.
 *
 * @param[in]  src        points to the input vector
 * @param[in]  shift_bits number of bits to shift.  A positive value shifts left; a negative value
 *                        shifts right.
 * @param[out] dst        points to the output vector
 * @param[in]  block_size number of samples in the vector
 */
DSP_FUNC_SCOPE void zdsp_shift_q15(const q15_t *src, int8_t shift_bits, q15_t *dst,
				   uint32_t block_size);

/**
 * @brief  Shifts the elements of a Q31 vector a specified number of bits.
 *
 * @par Scaling and Overflow Behavior
 *   The function uses saturating arithmetic.
 *   Results outside of the allowable Q31 range [0x80000000 0x7FFFFFFF] are saturated.
 *
 * @param[in]  src       points to the input vector
 * @param[in]  shift_bits  number of bits to shift.  A positive value shifts left; a negative value
 * shifts right.
 * @param[out] dst       points to the output vector
 * @param[in]  block_size  number of samples in the vector
 */
DSP_FUNC_SCOPE void zdsp_shift_q31(const q31_t *src, int8_t shift_bits, q31_t *dst,
				   uint32_t block_size);

/**
 * @}
 */

/**
 * @ingroup math_dsp_basic
 * @addtogroup math_dsp_basic_offset Vector Offset
 *
 * Adds a constant offset to each element of a vector.
 * <pre>
 *     dst[n] = src[n] + offset,   0 <= n < block_size.
 * </pre>
 * The functions support in-place computation allowing the source and destination pointers to
 * reference the same memory buffer. There are separate functions for floating-point, Q7, Q15, and
 * Q31 data types.
 *
 * @{
 */

/**
 * @brief  Adds a constant offset to a floating-point vector.
 * @param[in]  src       points to the input vector
 * @param[in]  offset     is the offset to be added
 * @param[out] dst       points to the output vector
 * @param[in]  block_size  number of samples in the vector
 */
DSP_FUNC_SCOPE void zdsp_offset_f32(const float32_t *src, float32_t offset, float32_t *dst,
				    uint32_t block_size);

/**
 * @brief  Adds a constant offset to a Q7 vector.
 *
 * @par Scaling and Overflow Behavior
 *   The function uses saturating arithmetic.
 *   Results outside of the allowable Q7 range [0x80 0x7F] are saturated.
 *
 * @param[in]  src       points to the input vector
 * @param[in]  offset     is the offset to be added
 * @param[out] dst       points to the output vector
 * @param[in]  block_size  number of samples in the vector
 */
DSP_FUNC_SCOPE void zdsp_offset_q7(const q7_t *src, q7_t offset, q7_t *dst, uint32_t block_size);

/**
 * @brief  Adds a constant offset to a Q15 vector.
 *
 * @par Scaling and Overflow Behavior
 *   The function uses saturating arithmetic.
 *   Results outside of the allowable Q15 range [0x8000 0x7FFF] are saturated.
 *
 * @param[in]  src        points to the input vector
 * @param[in]  offset     is the offset to be added
 * @param[out] dst        points to the output vector
 * @param[in]  block_size number of samples in the vector
 */
DSP_FUNC_SCOPE void zdsp_offset_q15(const q15_t *src, q15_t offset, q15_t *dst,
				    uint32_t block_size);

/**
 * @brief  Adds a constant offset to a Q31 vector.
 *
 * @par Scaling and Overflow Behavior
 *   The function uses saturating arithmetic.
 *   Results outside of the allowable Q31 range [0x80000000 0x7FFFFFFF] are saturated.
 *
 * @param[in]  src       points to the input vector
 * @param[in]  offset     is the offset to be added
 * @param[out] dst       points to the output vector
 * @param[in]  block_size  number of samples in the vector
 */
DSP_FUNC_SCOPE void zdsp_offset_q31(const q31_t *src, q31_t offset, q31_t *dst,
				    uint32_t block_size);

/**
 * @}
 */

/**
 * @ingroup math_dsp_basic
 * @addtogroup math_dsp_basic_negate Vector Negate
 *
 * Negates the elements of a vector.
 * <pre>
 *     dst[n] = -src[n],   0 <= n < block_size.
 * </pre>
 * The functions support in-place computation allowing the source and destination pointers to
 * reference the same memory buffer. There are separate functions for floating-point, Q7, Q15, and
 * Q31 data types.
 *
 * @{
 */

/**
 * @brief  Negates the elements of a floating-point vector.
 * @param[in]  src        points to the input vector
 * @param[out] dst        points to the output vector
 * @param[in]  block_size number of samples in the vector
 */
DSP_FUNC_SCOPE void zdsp_negate_f32(const float32_t *src, float32_t *dst, uint32_t block_size);

/**
 * @brief  Negates the elements of a Q7 vector.
 *
 * @par Scaling and Overflow Behavior
 *   The function uses saturating arithmetic.
 *   The Q7 value -1 (0x80) is saturated to the maximum allowable positive value 0x7F.
 *
 * @param[in]  src        points to the input vector
 * @param[out] dst        points to the output vector
 * @param[in]  block_size number of samples in the vector
 */
DSP_FUNC_SCOPE void zdsp_negate_q7(const q7_t *src, q7_t *dst, uint32_t block_size);

/**
 * @brief  Negates the elements of a Q15 vector.
 *
 * @par Scaling and Overflow Behavior
 *   The function uses saturating arithmetic.
 *   The Q15 value -1 (0x8000) is saturated to the maximum allowable positive value 0x7FFF.
 *
 * @param[in]  src        points to the input vector
 * @param[out] dst        points to the output vector
 * @param[in]  block_size number of samples in the vector
 */
DSP_FUNC_SCOPE void zdsp_negate_q15(const q15_t *src, q15_t *dst, uint32_t block_size);

/**
 * @brief  Negates the elements of a Q31 vector.
 *
 * @par Scaling and Overflow Behavior
 *   The function uses saturating arithmetic.
 *   The Q31 value -1 (0x80000000) is saturated to the maximum allowable positive value 0x7FFFFFFF.
 *
 * @param[in]  src        points to the input vector
 * @param[out] dst        points to the output vector
 * @param[in]  block_size number of samples in the vector
 */
DSP_FUNC_SCOPE void zdsp_negate_q31(const q31_t *src, q31_t *dst, uint32_t block_size);

/**
 * @}
 */

/**
 * @ingroup math_dsp_basic
 * @addtogroup math_dsp_basic_and Vector bitwise AND
 *
 * Compute the logical bitwise AND.
 *
 * There are separate functions for uint32_t, uint16_t, and uint7_t data types.
 * @{
 */

/**
 * @brief         Compute the logical bitwise AND of two fixed-point vectors.
 * @param[in]     src_a      points to input vector A
 * @param[in]     src_b      points to input vector B
 * @param[out]    dst        points to output vector
 * @param[in]     block_size number of samples in each vector
 */
DSP_FUNC_SCOPE void zdsp_and_u8(const uint8_t *src_a, const uint8_t *src_b, uint8_t *dst,
				uint32_t block_size);

/**
 * @brief         Compute the logical bitwise AND of two fixed-point vectors.
 * @param[in]     src_a      points to input vector A
 * @param[in]     src_b      points to input vector B
 * @param[out]    dst        points to output vector
 * @param[in]     block_size number of samples in each vector
 */
DSP_FUNC_SCOPE void zdsp_and_u16(const uint16_t *src_a, const uint16_t *src_b, uint16_t *dst,
				 uint32_t block_size);

/**
 * @brief         Compute the logical bitwise AND of two fixed-point vectors.
 * @param[in]     src_a      points to input vector A
 * @param[in]     src_b      points to input vector B
 * @param[out]    dst        points to output vector
 * @param[in]     block_size number of samples in each vector
 */
DSP_FUNC_SCOPE void zdsp_and_u32(const uint32_t *src_a, const uint32_t *src_b, uint32_t *dst,
				 uint32_t block_size);

/**
 * @}
 */

/**
 * @ingroup math_dsp_basic
 * @addtogroup math_dsp_basic_or Vector bitwise OR
 *
 * Compute the logical bitwise OR.
 *
 * There are separate functions for uint32_t, uint16_t, and uint7_t data types.
 * @{
 */

/**
 * @brief         Compute the logical bitwise OR of two fixed-point vectors.
 * @param[in]     src_a      points to input vector A
 * @param[in]     src_b      points to input vector B
 * @param[out]    dst        points to output vector
 * @param[in]     block_size number of samples in each vector
 */
DSP_FUNC_SCOPE void zdsp_or_u8(const uint8_t *src_a, const uint8_t *src_b, uint8_t *dst,
			       uint32_t block_size);

/**
 * @brief         Compute the logical bitwise OR of two fixed-point vectors.
 * @param[in]     src_a      points to input vector A
 * @param[in]     src_b      points to input vector B
 * @param[out]    dst        points to output vector
 * @param[in]     block_size number of samples in each vector
 */
DSP_FUNC_SCOPE void zdsp_or_u16(const uint16_t *src_a, const uint16_t *src_b, uint16_t *dst,
				uint32_t block_size);

/**
 * @brief         Compute the logical bitwise OR of two fixed-point vectors.
 * @param[in]     src_a      points to input vector A
 * @param[in]     src_b      points to input vector B
 * @param[out]    dst        points to output vector
 * @param[in]     block_size number of samples in each vector
 */
DSP_FUNC_SCOPE void zdsp_or_u32(const uint32_t *src_a, const uint32_t *src_b, uint32_t *dst,
				uint32_t block_size);

/**
 * @}
 */

/**
 * @ingroup math_dsp_basic
 * @addtogroup math_dsp_basic_not Vector bitwise NOT
 *
 * Compute the logical bitwise NOT.
 *
 * There are separate functions for uint32_t, uint16_t, and uint7_t data types.
 * @{
 */

/**
 * @brief         Compute the logical bitwise NOT of a fixed-point vector.
 * @param[in]     src        points to input vector
 * @param[out]    dst        points to output vector
 * @param[in]     block_size number of samples in each vector
 */
DSP_FUNC_SCOPE void zdsp_not_u8(const uint8_t *src, uint8_t *dst, uint32_t block_size);

/**
 * @brief         Compute the logical bitwise NOT of a fixed-point vector.
 * @param[in]     src        points to input vector
 * @param[out]    dst        points to output vector
 * @param[in]     block_size number of samples in each vector
 */
DSP_FUNC_SCOPE void zdsp_not_u16(const uint16_t *src, uint16_t *dst, uint32_t block_size);

/**
 * @brief         Compute the logical bitwise NOT of a fixed-point vector.
 * @param[in]     src        points to input vector
 * @param[out]    dst        points to output vector
 * @param[in]     block_size number of samples in each vector
 */
DSP_FUNC_SCOPE void zdsp_not_u32(const uint32_t *src, uint32_t *dst, uint32_t block_size);

/**
 * @}
 */

/**
 * @ingroup math_dsp_basic
 * @addtogroup math_dsp_basic_xor Vector bitwise XOR
 *
 * Compute the logical bitwise XOR.
 *
 * There are separate functions for uint32_t, uint16_t, and uint7_t data types.
 * @{
 */

/**
 * @brief         Compute the logical bitwise XOR of two fixed-point vectors.
 * @param[in]     src_a      points to input vector A
 * @param[in]     src_b      points to input vector B
 * @param[out]    dst        points to output vector
 * @param[in]     block_size number of samples in each vector
 */
DSP_FUNC_SCOPE void zdsp_xor_u8(const uint8_t *src_a, const uint8_t *src_b, uint8_t *dst,
				uint32_t block_size);

/**
 * @brief         Compute the logical bitwise XOR of two fixed-point vectors.
 * @param[in]     src_a      points to input vector A
 * @param[in]     src_b      points to input vector B
 * @param[out]    dst        points to output vector
 * @param[in]     block_size number of samples in each vector
 */
DSP_FUNC_SCOPE void zdsp_xor_u16(const uint16_t *src_a, const uint16_t *src_b, uint16_t *dst,
				 uint32_t block_size);

/**
 * @brief         Compute the logical bitwise XOR of two fixed-point vectors.
 * @param[in]     src_a      points to input vector A
 * @param[in]     src_b      points to input vector B
 * @param[out]    dst        points to output vector
 * @param[in]     block_size number of samples in each vector
 */
DSP_FUNC_SCOPE void zdsp_xor_u32(const uint32_t *src_a, const uint32_t *src_b, uint32_t *dst,
				 uint32_t block_size);

/**
 * @}
 */

/**
 * @ingroup math_dsp_basic
 * @addtogroup math_dsp_basic_clip Vector Clipping
 *
 * Element-by-element clipping of a value.
 *
 * The value is constrained between 2 bounds.
 *
 * There are separate functions for floating-point, Q7, Q15, and Q31 data types.
 * @{
 */

/**
 * @brief         Elementwise floating-point clipping
 * @param[in]     src          points to input values
 * @param[out]    dst          points to output clipped values
 * @param[in]     low          lower bound
 * @param[in]     high         higher bound
 * @param[in]     num_samples  number of samples to clip
 */
DSP_FUNC_SCOPE void zdsp_clip_f32(const float32_t *src, float32_t *dst, float32_t low,
				  float32_t high, uint32_t num_samples);

/**
 * @brief         Elementwise fixed-point clipping
 * @param[in]     src          points to input values
 * @param[out]    dst          points to output clipped values
 * @param[in]     low          lower bound
 * @param[in]     high         higher bound
 * @param[in]     num_samples  number of samples to clip
 */
DSP_FUNC_SCOPE void zdsp_clip_q31(const q31_t *src, q31_t *dst, q31_t low, q31_t high,
				  uint32_t num_samples);

/**
 * @brief         Elementwise fixed-point clipping
 * @param[in]     src          points to input values
 * @param[out]    dst          points to output clipped values
 * @param[in]     low          lower bound
 * @param[in]     high         higher bound
 * @param[in]     num_samples  number of samples to clip
 */
DSP_FUNC_SCOPE void zdsp_clip_q15(const q15_t *src, q15_t *dst, q15_t low, q15_t high,
				  uint32_t num_samples);

/**
 * @brief         Elementwise fixed-point clipping
 * @param[in]     src          points to input values
 * @param[out]    dst          points to output clipped values
 * @param[in]     low          lower bound
 * @param[in]     high         higher bound
 * @param[in]     num_samples  number of samples to clip
 */
DSP_FUNC_SCOPE void zdsp_clip_q7(const q7_t *src, q7_t *dst, q7_t low, q7_t high,
				 uint32_t num_samples);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#ifdef CONFIG_FP16
#include <zephyr/dsp/basicmath_f16.h>
#endif /* CONFIG_FP16 */

#endif /* INCLUDE_ZEPHYR_DSP_BASICMATH_H_ */
