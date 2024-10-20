/* Copyright (C) 2010-2021 ARM Limited or its affiliates. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file zephyr/dsp/basicmath_f16.h
 *
 * @brief Public APIs for DSP basicmath for 16 bit floating point
 */

#ifndef ZEPHYR_INCLUDE_DSP_BASICMATH_F16_H_
#define ZEPHYR_INCLUDE_DSP_BASICMATH_F16_H_

#ifndef CONFIG_FP16
#error "Cannot use float16 DSP functionality without CONFIG_FP16 enabled"
#endif /* CONFIG_FP16 */

#include <zephyr/dsp/dsp.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ingroup math_dsp_basic_mult
 * @brief Floating-point vector multiplication.
 * @param[in]  src_a      points to the first input vector
 * @param[in]  src_b      points to the second input vector
 * @param[out] dst        points to the output vector
 * @param[in]  block_size number of samples in each vector
 */
DSP_FUNC_SCOPE void zdsp_mult_f16(const float16_t *src_a, const float16_t *src_b, float16_t *dst,
				  uint32_t block_size);

/**
 * @ingroup math_dsp_basic_add
 * @brief Floating-point vector addition.
 * @param[in]  src_a      points to the first input vector
 * @param[in]  src_b      points to the second input vector
 * @param[out] dst        points to the output vector
 * @param[in]  block_size number of samples in each vector
 */
DSP_FUNC_SCOPE void zdsp_add_f16(const float16_t *src_a, const float16_t *src_b, float16_t *dst,
				 uint32_t block_size);

/**
 * @ingroup math_dsp_basic_sub
 * @brief Floating-point vector subtraction.
 * @param[in]  src_a      points to the first input vector
 * @param[in]  src_b      points to the second input vector
 * @param[out] dst        points to the output vector
 * @param[in]  block_size number of samples in each vector
 */
DSP_FUNC_SCOPE void zdsp_sub_f16(const float16_t *src_a, const float16_t *src_b, float16_t *dst,
				 uint32_t block_size);

/**
 * @ingroup math_dsp_basic_scale
 * @brief Multiplies a floating-point vector by a scalar.
 * @param[in]  src        points to the input vector
 * @param[in]  scale      scale factor to be applied
 * @param[out] dst        points to the output vector
 * @param[in]  block_size number of samples in the vector
 */
DSP_FUNC_SCOPE void zdsp_scale_f16(const float16_t *src, float16_t scale, float16_t *dst,
				   uint32_t block_size);

/**
 * @ingroup math_dsp_basic_abs
 * @brief Floating-point vector absolute value.
 * @param[in]  src        points to the input buffer
 * @param[out] dst        points to the output buffer
 * @param[in]  block_size number of samples in each vector
 */
DSP_FUNC_SCOPE void zdsp_abs_f16(const float16_t *src, float16_t *dst, uint32_t block_size);

/**
 * @ingroup math_dsp_basic_dot
 * @brief Dot product of floating-point vectors.
 * @param[in]  src_a      points to the first input vector
 * @param[in]  src_b      points to the second input vector
 * @param[in]  block_size number of samples in each vector
 * @param[out] result     output result returned here
 */
DSP_FUNC_SCOPE void zdsp_dot_prod_f16(const float16_t *src_a, const float16_t *src_b,
				      uint32_t block_size, float16_t *result);

/**
 * @ingroup math_dsp_basic_offset
 * @brief  Adds a constant offset to a floating-point vector.
 * @param[in]  src       points to the input vector
 * @param[in]  offset     is the offset to be added
 * @param[out] dst       points to the output vector
 * @param[in]  block_size  number of samples in the vector
 */
DSP_FUNC_SCOPE void zdsp_offset_f16(const float16_t *src, float16_t offset, float16_t *dst,
				    uint32_t block_size);

/**
 * @ingroup math_dsp_basic_negate
 * @brief  Negates the elements of a floating-point vector.
 * @param[in]  src        points to the input vector
 * @param[out] dst        points to the output vector
 * @param[in]  block_size number of samples in the vector
 */
DSP_FUNC_SCOPE void zdsp_negate_f16(const float16_t *src, float16_t *dst, uint32_t block_size);

/**
 * @ingroup math_dsp_basic_clip
 * @brief         Elementwise floating-point clipping
 * @param[in]     src          points to input values
 * @param[out]    dst          points to output clipped values
 * @param[in]     low          lower bound
 * @param[in]     high         higher bound
 * @param[in]     num_samples  number of samples to clip
 */
DSP_FUNC_SCOPE void zdsp_clip_f16(const float16_t *src, float16_t *dst, float16_t low,
				  float16_t high, uint32_t num_samples);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DSP_BASICMATH_F16_H_ */
