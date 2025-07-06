/*
 * Copyright (C) 2024 OWL Services LLC. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file zephyr/dsp/utils.h
 *
 * @brief Extra functions and macros for DSP
 */

#ifndef INCLUDE_ZEPHYR_DSP_UTILS_H_
#define INCLUDE_ZEPHYR_DSP_UTILS_H_

#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/dsp/dsp.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ingroup math_dsp
 * @defgroup math_dsp_utils_shifts Float/Fixed point shift conversion functions
 */

/**
 * @ingroup math_dsp_utils_shifts
 * @addtogroup math_dsp_basic_conv_to_float Fixed to Float point conversions
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
#define Z_SHIFT_Q7_TO_F32(src, m) ((float32_t)(((src << m)) / (float32_t)(1U << 7)))

/**
 * @brief Convert a Q15 fixed-point value to a floating-point (float32_t) value with a left shift.
 *
 * @param src The input Q15 fixed-point value.
 * @param m   The number of bits to left shift the input value (0 to 15).
 * @return The converted floating-point (float32_t) value.
 */
#define Z_SHIFT_Q15_TO_F32(src, m) ((float32_t)((src << m) / (float32_t)(1U << 15)))

/**
 * @brief Convert a Q31 fixed-point value to a floating-point (float32_t) value with a left shift.
 *
 * @param src The input Q31 fixed-point value.
 * @param m   The number of bits to left shift the input value (0 to 31).
 * @return The converted floating-point (float32_t) value.
 */
#define Z_SHIFT_Q31_TO_F32(src, m) ((float32_t)(((int64_t)src) << m) / (float32_t)(1U << 31))

/**
 * @brief Convert a Q7 fixed-point value to a floating-point (float64_t) value with a left shift.
 *
 * @param src The input Q7 fixed-point value.
 * @param m   The number of bits to left shift the input value (0 to 7).
 * @return The converted floating-point (float64_t) value.
 */
#define Z_SHIFT_Q7_TO_F64(src, m) (((float64_t)(src << m)) / (1U << 7))

/**
 * @brief Convert a Q15 fixed-point value to a floating-point (float64_t) value with a left shift.
 *
 * @param src The input Q15 fixed-point value.
 * @param m   The number of bits to left shift the input value (0 to 15).
 * @return The converted floating-point (float64_t) value.
 */
#define Z_SHIFT_Q15_TO_F64(src, m) (((float64_t)(src << m)) / (1UL << 15))

/**
 * @brief Convert a Q31 fixed-point value to a floating-point (float64_t) value with a left shift.
 *
 * @param src The input Q31 fixed-point value.
 * @param m   The number of bits to left shift the input value (0 to 31).
 * @return The converted floating-point (float64_t) value.
 */
#define Z_SHIFT_Q31_TO_F64(src, m) ((float64_t)(((int64_t)src) << m) / (1ULL << 31))

/**
 * @}
 */

/**
 * @ingroup math_dsp_utils_shifts
 * @addtogroup math_dsp_basic_conv_to_fixed Float to Fixed point conversions
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
#define Z_SHIFT_F32_TO_Q7(src, m)                                                                  \
	((q7_t)Z_CLAMP((int32_t)(src * (1U << 7)) >> m, INT8_MIN, INT8_MAX))

/**
 * @brief Convert a floating-point (float32_t) value to a Q15 fixed-point value with a right shift.
 *
 * @param src The input floating-point (float32_t) value.
 * @param m   The number of bits to right shift the input value (0 to 15).
 * @return The converted Q15 fixed-point value.
 */
#define Z_SHIFT_F32_TO_Q15(src, m)                                                                 \
	((q15_t)Z_CLAMP((int32_t)(src * (1U << 15)) >> m, INT16_MIN, INT16_MAX))

/**
 * @brief Convert a floating-point (float32_t) value to a Q31 fixed-point value with a right shift.
 *
 * @param src The input floating-point (float32_t) value.
 * @param m   The number of bits to right shift the input value (0 to 31).
 * @return The converted Q31 fixed-point value.
 */
#define Z_SHIFT_F32_TO_Q31(src, m)                                                                 \
	((q31_t)Z_CLAMP((int64_t)(src * (1U << 31)) >> m, INT32_MIN, INT32_MAX))

/**
 * @brief Convert a floating-point (float64_t) value to a Q7 fixed-point value with a right shift.
 *
 * @param src The input floating-point (float64_t) value.
 * @param m   The number of bits to right shift the input value (0 to 7).
 * @return The converted Q7 fixed-point value.
 */
#define Z_SHIFT_F64_TO_Q7(src, m)                                                                  \
	((q7_t)Z_CLAMP((int32_t)(src * (1U << 7)) >> m, INT8_MIN, INT8_MAX))

/**
 * @brief Convert a floating-point (float64_t) value to a Q15 fixed-point value with a right shift.
 *
 * @param src The input floating-point (float64_t) value.
 * @param m   The number of bits to right shift the input value (0 to 15).
 * @return The converted Q15 fixed-point value.
 */
#define Z_SHIFT_F64_TO_Q15(src, m)                                                                 \
	((q15_t)Z_CLAMP((int32_t)(src * (1U << 15)) >> m, INT16_MIN, INT16_MAX))

/**
 * @brief Convert a floating-point (float64_t) value to a Q31 fixed-point value with a right shift.
 *
 * @param src The input floating-point (float64_t) value.
 * @param m   The number of bits to right shift the input value (0 to 31).
 * @return The converted Q31 fixed-point value.
 */
#define Z_SHIFT_F64_TO_Q31(src, m)                                                                 \
	((q31_t)Z_CLAMP((int64_t)(src * (1U << 31)) >> m, INT32_MIN, INT32_MAX))

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_ZEPHYR_DSP_UTILS_H_ */
