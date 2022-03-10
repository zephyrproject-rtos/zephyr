/**
 * @file zephyr/math/q_types.h
 *
 * @brief Common fixed point fractional types.
 */

/*
 * Copyright (c) 2022 Google Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef INCLUDE_ZEPHYR_MATH_Q_TYPES_H_
#define INCLUDE_ZEPHYR_MATH_Q_TYPES_H_

#include <stdint.h>

#ifndef INCLUDE_MATH_FP_H_
#error "Please include <zephyr/math/fp.h> instead"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @typedef q1_23_8_t
 * @brief Signed fixed point type
 *
 * Represents a number with:
 * - 1 sign bit
 * - 23 integer bits
 * - 8 bit decimal resolution
 *
 * Min: -(2^23) - 1
 * Max: 2^23
 * Accuracy: 1 / (2^8 - 1)
 */
typedef struct {
	int32_t value;
} q1_23_8_t;


/**
 * @typedef q1_15_16_t
 * @brief Signed fixed point type
 *
 * Represents a number with:
 * - 1 sign bit
 * - 15 integer bits
 * - 16 bit decimal resolution
 *
 * Min: -(2^15) - 1
 * Max: 2^15
 * Accuracy: 1 / (2^16 - 1)
 */
typedef struct {
	int32_t value;
} q1_15_16_t;


/**
 * @typedef q1_0_31_t
 * @brief Signed fixed point type
 *
 * Represents a number with:
 * - 1 sign bit
 * - 0 integer bits
 * - 31 bit decimal resolution
 *
 * Min: - 1
 * Max: 1
 * Accuracy: 1 / (2^31 - 1)
 */
typedef struct {
	int32_t value;
} q1_0_31_t;

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_ZEPHYR_MATH_Q_TYPES_H_ */
