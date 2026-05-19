/*
 * Copyright (c) 2019 Facebook.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Extra arithmetic and bit manipulation functions.
 * @defgroup math_extras Math extras
 * @ingroup utilities
 *
 * Portable wrapper functions for a number of arithmetic and bit-counting functions that are often
 * provided by compiler builtins. If the compiler does not have an appropriate builtin, a portable C
 * implementation is used instead.
 *
 * @{
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

/**
 * @brief Add two unsigned 16-bit integers.
 * @param a First operand.
 * @param b Second operand.
 * @param result Pointer to the result.
 * @return true if the operation overflowed.
 */
static bool u16_add_overflow(uint16_t a, uint16_t b, uint16_t *result);

/**
 * @brief Add two unsigned 32-bit integers.
 * @param a First operand.
 * @param b Second operand.
 * @param result Pointer to the result.
 * @return true if the operation overflowed.
 */

static bool u32_add_overflow(uint32_t a, uint32_t b, uint32_t *result);

/**
 * @brief Add two unsigned 64-bit integers.
 * @param a First operand.
 * @param b Second operand.
 * @param result Pointer to the result.
 * @return true if the operation overflowed.
 */
static bool u64_add_overflow(uint64_t a, uint64_t b, uint64_t *result);

/**
 * @brief Add two size_t integers.
 * @param a First operand.
 * @param b Second operand.
 * @param result Pointer to the result.
 * @return true if the operation overflowed.
 */
static bool size_add_overflow(size_t a, size_t b, size_t *result);

/**@}*/

/**
 * @name Unsigned integer multiplication with overflow detection.
 *
 * These functions compute `a * b` and store the result in `*result`, returning
 * true if the operation overflowed.
 */
/**@{*/

/**
 * @brief Multiply two unsigned 16-bit integers.
 * @param a First operand.
 * @param b Second operand.
 * @param result Pointer to the result.
 * @return true if the operation overflowed.
 */
static bool u16_mul_overflow(uint16_t a, uint16_t b, uint16_t *result);

/**
 * @brief Multiply two unsigned 32-bit integers.
 * @param a First operand.
 * @param b Second operand.
 * @param result Pointer to the result.
 * @return true if the operation overflowed.
 */

static bool u32_mul_overflow(uint32_t a, uint32_t b, uint32_t *result);
/**
 * @brief Multiply two unsigned 64-bit integers.
 * @param a First operand.
 * @param b Second operand.
 * @param result Pointer to the result.
 * @return true if the operation overflowed.
 */
static bool u64_mul_overflow(uint64_t a, uint64_t b, uint64_t *result);

/**
 * @brief Multiply two size_t integers.
 * @param a First operand.
 * @param b Second operand.
 * @param result Pointer to the result.
 * @return true if the operation overflowed.
 */
static bool size_mul_overflow(size_t a, size_t b, size_t *result);

/**@}*/

/**
 * @name Count leading zeros.
 *
 * Count the number of leading zero bits in the bitwise representation of `x`.
 * When `x = 0`, this is the size of `x` in bits.
 */
/**@{*/

/**
 * @brief Count the number of leading zero bits in a 32-bit integer.
 * @param x Integer to count leading zeros in.
 * @return Number of leading zero bits in `x`.
 */
static int u32_count_leading_zeros(uint32_t x);

/**
 * @brief Count the number of leading zero bits in a 64-bit integer.
 * @param x Integer to count leading zeros in.
 * @return Number of leading zero bits in `x`.
 */
static int u64_count_leading_zeros(uint64_t x);

/**@}*/

/**
 * @name Count trailing zeros.
 *
 * Count the number of trailing zero bits in the bitwise representation of `x`.
 * When `x = 0`, this is the size of `x` in bits.
 */
/**@{*/

/**
 * @brief Count the number of trailing zero bits in a 32-bit integer.
 * @param x Integer to count trailing zeros in.
 * @return Number of trailing zero bits in `x`.
 */
static int u32_count_trailing_zeros(uint32_t x);

/**
 * @brief Count the number of trailing zero bits in a 64-bit integer.
 * @param x Integer to count trailing zeros in.
 * @return Number of trailing zero bits in `x`.
 */
static int u64_count_trailing_zeros(uint64_t x);

/**@}*/

/**
 * @name 128-bit arithmetic.
 *
 * Functions for performing arithmetic operations on 128-bit integers.
 * These functions provide support for high-precision calculations on platforms
 * where native 128-bit types are not available.
 */
/**@{*/

/**
 * @brief 128-bit integer structure.
 *
 * Representation of a 128-bit integer using two 64-bit words.
 */
typedef struct {
	/** Low-order 64 bits. */
	uint64_t low;
	/** High-order 64 bits (includes sign bit). */
	uint64_t high;
} int128_t;

/**
 * @brief Multiply two signed 64-bit integers and store the result in a 128-bit integer.
 *
 * This function performs a full precision multiplication of two signed 64-bit
 * values. The result is guaranteed to fit in a 128-bit representation without overflow.
 *
 * @param a Multiplicand.
 * @param b Multiplier.
 * @param result Pointer to the @ref int128_t structure where the result will be stored.
 */
static void i128_multiply_i64_i64(int64_t a, int64_t b, int128_t *result);

/**@}*/

/**@}*/

#include <zephyr/sys/math_extras_impl.h>

#endif /* ZEPHYR_INCLUDE_SYS_MATH_EXTRAS_H_ */
