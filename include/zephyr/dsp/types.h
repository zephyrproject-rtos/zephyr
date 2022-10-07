/* Copyright (c) 2022 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef INCLUDE_ZEPHYR_DSP_TYPES_H_
#define INCLUDE_ZEPHYR_DSP_TYPES_H_

#include <stdint.h>

/**
 * @addtogroup math_dsp
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @typedef q7_t
 * @brief 8-bit fractional data type in 1.7 format.
 */
typedef int8_t q7_t;

/**
 * @typedef q15_t
 * @brief 16-bit fractional data type in 1.15 format.
 */
typedef int16_t q15_t;

/**
 * @typedef q31_t
 * @brief 32-bit fractional data type in 1.31 format.
 */
typedef int32_t q31_t;

/**
 * @typedef q63_t
 * @brief 64-bit fractional data type in 1.63 format.
 */
typedef int64_t q63_t;

/**
 * @typedef float16_t
 * @brief 16-bit floating point type definition.
 */
#if defined(CONFIG_FP16)
typedef __fp16 float16_t;
#endif /* CONFIG_FP16 */

/**
 * @typedef float32_t
 * @brief 32-bit floating-point type definition.
 */
typedef float float32_t;

/**
 * @typedef float64_t
 * @brief 64-bit floating-point type definition.
 */
typedef double float64_t;

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* INCLUDE_ZEPHYR_DSP_TYPES_H_ */
