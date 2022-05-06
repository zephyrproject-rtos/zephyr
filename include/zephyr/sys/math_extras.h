/*
 * Copyright (c) 2019 Facebook.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Extra arithmetic and bit manipulation functions.
 *
 * @details This header file provides portable wrapper functions for a number of
 * arithmetic and bit-counting functions that are often provided by compiler
 * builtins. If the compiler does not have an appropriate builtin, a portable C
 * implementation is used instead.
 */

#ifndef ZEPHYR_INCLUDE_SYS_MATH_EXTRAS_H_
#define ZEPHYR_INCLUDE_SYS_MATH_EXTRAS_H_

#include <zephyr/types.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @name Unsigned integer addition with overflow detection.
 *
 * These functions compute `a + b` and store the result in `*result`, returning
 * true if the operation overflowed.
 */
/**@{*/
static bool u16_add_overflow(uint16_t a, uint16_t b, uint16_t *result);
static bool u32_add_overflow(uint32_t a, uint32_t b, uint32_t *result);
static bool u64_add_overflow(uint64_t a, uint64_t b, uint64_t *result);
static bool size_add_overflow(size_t a, size_t b, size_t *result);
/**@}*/

/**
 * @name Unsigned integer multiplication with overflow detection.
 *
 * These functions compute `a * b` and store the result in `*result`, returning
 * true if the operation overflowed.
 */
/**@{*/
static bool u16_mul_overflow(uint16_t a, uint16_t b, uint16_t *result);
static bool u32_mul_overflow(uint32_t a, uint32_t b, uint32_t *result);
static bool u64_mul_overflow(uint64_t a, uint64_t b, uint64_t *result);
static bool size_mul_overflow(size_t a, size_t b, size_t *result);
/**@}*/

/**
 * @name Count leading zeros.
 *
 * Count the number of leading zero bits in the bitwise representation of `x`.
 * When `x = 0`, this is the size of `x` in bits.
 */
/**@{*/
static int u32_count_leading_zeros(uint32_t x);
static int u64_count_leading_zeros(uint64_t x);
/**@}*/

/**
 * @name Count trailing zeros.
 *
 * Count the number of trailing zero bits in the bitwise representation of `x`.
 * When `x = 0`, this is the size of `x` in bits.
 */
/**@{*/
static int u32_count_trailing_zeros(uint32_t x);
static int u64_count_trailing_zeros(uint64_t x);
/**@}*/

#include <zephyr/sys/math_extras_impl.h>

#endif /* ZEPHYR_INCLUDE_SYS_MATH_EXTRAS_H_ */
