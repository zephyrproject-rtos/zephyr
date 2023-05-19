/* Copyright (c) 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZEPHYR_DSP_CONVERSIONS_H
#define ZEPHYR_INCLUDE_ZEPHYR_DSP_CONVERSIONS_H

#include <zephyr/dsp/types.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ingroup math_dsp
 * @defgroup math_conversions Basic Math Conversions
 *
 * Enables easy conversion of Q7, Q15, Q31, and float types.
 * @{
 */

/**
 * @brief Convert Q31 format to float
 *
 * @param[in]  src The Q31 input vector
 * @param[in]  shift The shift value associated with the @p src
 * @param[out] dst The float output vector
 * @param[in]  count Number of elements in each vector
 */
static inline void zdsp_q31_to_float(const q31_t *src, int8_t shift, float *dst, uint32_t count)
{
	for (uint32_t i = 0; i < count; ++i) {
		int64_t shifted = src[i];

		if (shift < 0) {
			shifted = shifted >> -shift;
		} else {
			shifted = shifted << shift;
		}
		dst[i] = (float)shifted / (float)INT32_MAX;
	}
}

/**
 * @brief Convert Q15 format to float
 *
 * @param[in]  src The Q15 input vector
 * @param[in]  shift The shift value associated with the @p src
 * @param[out] dst The float output vector
 * @param[in]  count Number of elements in each vector
 */
static inline void zdsp_q15_to_float(const q15_t *src, int8_t shift, float *dst, uint32_t count)
{
	for (uint32_t i = 0; i < count; ++i) {
		int64_t shifted = src[i];

		if (shift < 0) {
			shifted = shifted >> -shift;
		} else {
			shifted = shifted << shift;
		}
		dst[i] = (float)shifted / (float)INT16_MAX;
	}
}

/**
 * @brief Convert Q7 format to float
 *
 * @param[in]  src The Q7 input vector
 * @param[in]  shift The shift value associated with the @p src
 * @param[out] dst The float output vector
 * @param[in]  count Number of elements in each vector
 */
static inline void zdsp_q7_to_float(const q7_t *src, int8_t shift, float *dst, uint32_t count)
{
	for (uint32_t i = 0; i < count; ++i) {
		int64_t shifted = src[i];

		if (shift < 0) {
			shifted = shifted >> -shift;
		} else {
			shifted = shifted << shift;
		}
		dst[i] = (float)shifted / (float)INT8_MAX;
	}
}

/**
 * @brief Convert floats to Q31 format
 *
 * @param[in]  src The float input vector
 * @param[in]  shift The shift value for @p src
 * @param[out] dst The Q31 output vector
 * @param[in]  count Number of elements in each vector
 */
static inline void zdsp_float_to_q31(const float *src, int8_t shift, q31_t *dst, uint32_t count)
{
	int64_t multiplier = INT32_MAX;

	if (shift < 0) {
		multiplier = multiplier << -shift;
	} else {
		multiplier = multiplier >> shift;
	}
	for (uint32_t i = 0; i < count; ++i) {
		dst[i] = (int32_t)(src[i] * (float)multiplier);
	}
}

/**
 * @brief Convert floats to Q15 format
 *
 * @param[in]  src The float input vector
 * @param[in]  shift The shift value for @p src
 * @param[out] dst The Q15 output vector
 * @param[in]  count Number of elements in each vector
 */
static inline void zdsp_float_to_q15(const float *src, int8_t shift, q15_t *dst, uint32_t count)
{
	int64_t multiplier = INT16_MAX;

	if (shift < 0) {
		multiplier = multiplier << -shift;
	} else {
		multiplier = multiplier >> shift;
	}
	for (uint32_t i = 0; i < count; ++i) {
		dst[i] = (int16_t)(src[i] * (float)multiplier);
	}
}

/**
 * @brief Convert floats to Q7 format
 *
 * @param[in]  src The float input vector
 * @param[in]  shift The shift value for @p src
 * @param[out] dst The Q7 output vector
 * @param[in]  count Number of elements in each vector
 */
static inline void zdsp_float_to_q7(const float *src, int8_t shift, q7_t *dst, uint32_t count)
{
	int64_t multiplier = INT8_MAX;

	if (shift < 0) {
		multiplier = multiplier << -shift;
	} else {
		multiplier = multiplier >> shift;
	}
	for (uint32_t i = 0; i < count; ++i) {
		dst[i] = (int8_t)(src[i] * (float)multiplier);
	}
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ZEPHYR_DSP_CONVERSIONS_H */
