/* Copyright (c) 2026 James Roy <rruuaanng@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file zephyr/dsp/fastmath.h
 *
 * @brief Public APIs for Fast Math.
 */

#ifndef ZEPHYR_INCLUDE_DSP_FASTMATH_H_
#define ZEPHYR_INCLUDE_DSP_FASTMATH_H_

#include <zephyr/dsp/dsp.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ingroup math_dsp
 * @defgroup math_fastmath Fast Math Functions
 * Elementary functions for DSP.
 * @{
 */

/**
 * @brief  Fast approximation to the trigonometric sine function for floating-point data.
 * @param[in] x  input value in radians.
 * @return  sin(x).
 */
DSP_FUNC_SCOPE float32_t zdsp_sin_f32(DSP_DATA float32_t x);

/**
 * @brief  Fast approximation to the trigonometric sine function for Q15 data.
 * @param[in] x  Scaled input value in radians.
 * @return  sin(x).
 */
DSP_FUNC_SCOPE q15_t zdsp_sin_q15(DSP_DATA q15_t x);

/**
 * @brief  Fast approximation to the trigonometric sine function for Q31 data.
 * @param[in] x  Scaled input value in radians.
 * @return  sin(x).
 */
DSP_FUNC_SCOPE q31_t zdsp_sin_q31(DSP_DATA q31_t x);

/**
 * @brief  Fast approximation to the trigonometric cosine function for floating-point data.
 * @param[in] x  input value in radians.
 * @return  cos(x).
 */
DSP_FUNC_SCOPE float32_t zdsp_cos_f32(DSP_DATA float32_t x);

/**
 * @brief  Fast approximation to the trigonometric cosine function for Q15 data.
 * @param[in] x  Scaled input value in radians.
 * @return  cos(x).
 */
DSP_FUNC_SCOPE q15_t zdsp_cos_q15(DSP_DATA q15_t x);

/**
 * @brief Fast approximation to the trigonometric cosine function for Q31 data.
 * @param[in] x  Scaled input value in radians.
 * @return  cos(x).
 */
DSP_FUNC_SCOPE q31_t zdsp_cos_q31(DSP_DATA q31_t x);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DSP_FASTMATH_H_ */
