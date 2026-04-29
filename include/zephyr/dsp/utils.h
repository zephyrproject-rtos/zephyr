/*
 * Copyright (C) 2024 OWL Services LLC. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file zephyr/dsp/utils.h
 *
 * @brief Extra utility functions for DSP
 */

#ifndef INCLUDE_ZEPHYR_DSP_UTILS_H_
#define INCLUDE_ZEPHYR_DSP_UTILS_H_

#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/dsp/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ingroup math_dsp
 * @defgroup math_dsp_utils_shifts Float/Fixed point shift conversion functions
 *
 * Convert number representation in Float or Double to/from Q31/Q15/Q7.
 *
 * @{
 */

/**
 * @ingroup math_dsp_utils_shifts
 * @defgroup math_dsp_basic_conv_to_float Fixed to Float point conversions
 *
 * Convert number Q7/Q15/Q31 to Float or Double representation with shift.
 *
 * There are separate functions for floating-point, Q7, Q15, and Q31 data types.
 * @{
 */

/**
 * @brief Convert a Q7 fixed-point value to a floating-point (float32_t) value with a left shift.
 *
 * @param src The input Q7 fixed-point value.
 * @param m   The number of bits to left shift the input value (0 to 7).
 * @return The converted floating-point (float32_t) value.
 */
static inline float32_t zdsp_q7_to_f32_shift(q7_t src, uint32_t m)
{
	return (float32_t)((src << m) / (float32_t)(1U << 7));
}

/**
 * @brief Convert a Q15 fixed-point value to a floating-point (float32_t) value with a left shift.
 *
 * @param src The input Q15 fixed-point value.
 * @param m   The number of bits to left shift the input value (0 to 15).
 * @return The converted floating-point (float32_t) value.
 */
static inline float32_t zdsp_q15_to_f32_shift(q15_t src, uint32_t m)
{
	return (float32_t)((src << m) / (float32_t)(1U << 15));
}

/**
 * @brief Convert a Q31 fixed-point value to a floating-point (float32_t) value with a left shift.
 *
 * @param src The input Q31 fixed-point value.
 * @param m   The number of bits to left shift the input value (0 to 31).
 * @return The converted floating-point (float32_t) value.
 */
static inline float32_t zdsp_q31_to_f32_shift(q31_t src, uint32_t m)
{
	return (float32_t)(((int64_t)src) << m) / (float32_t)(1U << 31);
}

/**
 * @brief Convert a Q7 fixed-point value to a floating-point (float64_t) value with a left shift.
 *
 * @param src The input Q7 fixed-point value.
 * @param m   The number of bits to left shift the input value (0 to 7).
 * @return The converted floating-point (float64_t) value.
 */
static inline float64_t zdsp_q7_to_f64_shift(q7_t src, uint32_t m)
{
	return ((float64_t)(src << m)) / (1U << 7);
}

/**
 * @brief Convert a Q15 fixed-point value to a floating-point (float64_t) value with a left shift.
 *
 * @param src The input Q15 fixed-point value.
 * @param m   The number of bits to left shift the input value (0 to 15).
 * @return The converted floating-point (float64_t) value.
 */
static inline float64_t zdsp_q15_to_f64_shift(q15_t src, uint32_t m)
{
	return ((float64_t)(src << m)) / (1UL << 15);
}

/**
 * @brief Convert a Q31 fixed-point value to a floating-point (float64_t) value with a left shift.
 *
 * @param src The input Q31 fixed-point value.
 * @param m   The number of bits to left shift the input value (0 to 31).
 * @return The converted floating-point (float64_t) value.
 */
static inline float64_t zdsp_q31_to_f64_shift(q31_t src, uint32_t m)
{
	return (float64_t)(((int64_t)src) << m) / (1ULL << 31);
}

/**
 * @}
 */

/**
 * @ingroup math_dsp_utils_shifts
 * @defgroup math_dsp_basic_conv_to_fixed Float to Fixed point conversions
 *
 * Convert number representation in Float or Double to Q31/Q15/Q7.
 *
 * There are separate functions for floating-point, Q7, Q15, and Q31 data types.
 * @{
 */

/**
 * @brief Convert a floating-point (float32_t) value to a Q7 fixed-point value with a right shift.
 *
 * @param src The input floating-point (float32_t) value.
 * @param m   The number of bits to right shift the input value (0 to 7).
 * @return The converted Q7 fixed-point value.
 */
static inline q7_t zdsp_f32_to_q7_shift(float32_t src, uint32_t m)
{
	return (q7_t)clamp((int32_t)(src * (1U << 7)) >> m, INT8_MIN, INT8_MAX);
}

/**
 * @brief Convert a floating-point (float32_t) value to a Q15 fixed-point value with a right shift.
 *
 * @param src The input floating-point (float32_t) value.
 * @param m   The number of bits to right shift the input value (0 to 15).
 * @return The converted Q15 fixed-point value.
 */
static inline q15_t zdsp_f32_to_q15_shift(float32_t src, uint32_t m)
{
	return (q15_t)clamp((int32_t)(src * (1U << 15)) >> m, INT16_MIN, INT16_MAX);
}

/**
 * @brief Convert a floating-point (float32_t) value to a Q31 fixed-point value with a right shift.
 *
 * @param src The input floating-point (float32_t) value.
 * @param m   The number of bits to right shift the input value (0 to 31).
 * @return The converted Q31 fixed-point value.
 */
static inline q31_t zdsp_f32_to_q31_shift(float32_t src, uint32_t m)
{
	return (q31_t)clamp((int64_t)(src * (1U << 31)) >> m, INT32_MIN, INT32_MAX);
}

/**
 * @brief Convert a floating-point (float64_t) value to a Q7 fixed-point value with a right shift.
 *
 * @param src The input floating-point (float64_t) value.
 * @param m   The number of bits to right shift the input value (0 to 7).
 * @return The converted Q7 fixed-point value.
 */
static inline q7_t zdsp_f64_to_q7_shift(float64_t src, uint32_t m)
{
	return (q7_t)clamp((int32_t)(src * (1U << 7)) >> m, INT8_MIN, INT8_MAX);
}

/**
 * @brief Convert a floating-point (float64_t) value to a Q15 fixed-point value with a right shift.
 *
 * @param src The input floating-point (float64_t) value.
 * @param m   The number of bits to right shift the input value (0 to 15).
 * @return The converted Q15 fixed-point value.
 */
static inline q15_t zdsp_f64_to_q15_shift(float64_t src, uint32_t m)
{
	return (q15_t)clamp((int32_t)(src * (1U << 15)) >> m, INT16_MIN, INT16_MAX);
}

/**
 * @brief Convert a floating-point (float64_t) value to a Q31 fixed-point value with a right shift.
 *
 * @param src The input floating-point (float64_t) value.
 * @param m   The number of bits to right shift the input value (0 to 31).
 * @return The converted Q31 fixed-point value.
 */
static inline q31_t zdsp_f64_to_q31_shift(float64_t src, uint32_t m)
{
	return (q31_t)clamp((int64_t)(src * (1U << 31)) >> m, INT32_MIN, INT32_MAX);
}

/**
 * @}
 */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_ZEPHYR_DSP_UTILS_H_ */
