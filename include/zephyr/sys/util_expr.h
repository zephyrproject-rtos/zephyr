/*
 * Copyright (c) 2025 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_UTIL_EXPR_H_
#define ZEPHYR_INCLUDE_SYS_UTIL_EXPR_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ZTEST_UNITTEST
#include <zephyr/util_expr_nums_generated.h>
#endif

#include <zephyr/sys/util_macro.h>
#include <zephyr/sys/util_internal_expr.h>

/**
 * @brief Expression utilities
 *
 * This API provides a set of macros that perform bitwise operations, comparison operations, and
 * arithmetic operations on 32-bit numbers represented by 32 arguments of 0 or 1
 * (hereafter "bit-args").
 * This makes it possible to use arithmetic operations even at compile time.
 *
 * ### About bit-args format
 * The "bit-args" format is a way to represent binary numbers using individual bits
 * as separate arguments.
 * A 32-bit binary number must be provided as 32 separate arguments. Unless otherwise specified,
 * bit-args is assumed to be a 32-bit number.
 * Expr utilities can perform operations on numbers represented in this format.
 *
 * For example:
 * - The number `0b00000000000000000000000000001010` would be represented as
 *   `0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 *    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0`.
 *
 * @defgroup util-expr EXPR utilities
 * @ingroup sys-util
 * @{
 */

/**
 * @brief Refers to a predefined macro and converts a hexadecimal value to bit-args format.
 *
 * The values that can be converted depend on the existence of predefined macros.
 *
 * @param h hexadecimal number start with '0x'
 *
 * Example usage:
 * @code
 * Z_EXPR_HEX(0x3) // 0x3 in bit-args format (0, 0, ..., 1, 1)
 * @endcode
 */
#define EXPR_BITS_0X(...) Z_EXPR_BITS_FROM_HEX(NUM_VA_ARGS_LESS_1(__VA_ARGS__), __VA_ARGS__)

/**
 * @brief Refers to a predefined macro and converts a decimal value to bit-args format.
 *
 * @param d decimal number
 *
 * Example usage:
 * @code
 * Z_EXPR_DEC(3) // 3 in bit-args format (0, 0, ..., 1, 1)
 * @endcode
 */
#define EXPR_BITS(d) UTIL_CAT(Z_EXPR_BITS_, d)

/**
 * @brief Performs a bitwise OR operation.
 *
 * @param ... It takes 64 arguments as two bit-args numbers.
 * @param ... The first 32 are the first operand in bit-args format.
 * @param ... The remaining 32 are the second operand in bit-args format.
 * @return The operation result is expressed in bit-args format.
 *
 * Example usage:
 * @code
 * EXPR_OR(Z_EXPR_HEX(0x3), Z_EXPR_HEX(0x2)) // 0x3 in bit-args format
 * @endcode
 */
#define EXPR_OR(...) Z_EXPR_OR(__VA_ARGS__)

/**
 * @brief Performs a bitwise AND operation.
 *
 * @param ... It takes 64 arguments as two bit-args numbers.
 * @param ... The first 32 are the first operand in bit-args format.
 * @param ... The remaining 32 are the second operand in bit-args format.
 * @return The operation result is expressed in bit-args format.
 *
 * Example usage:
 * @code
 * EXPR_AND(Z_EXPR_HEX(0x3), Z_EXPR_HEX(0x2)) // 0x1 in bit-args format
 * @endcode
 */
#define EXPR_AND(...) Z_EXPR_AND(__VA_ARGS__)

/**
 * @brief Performs a bitwise XOR operation.
 *
 * @param ... It takes 64 arguments as two bit-args numbers.
 * @param ... The first 32 are the first operand in bit-args format.
 * @param ... The remaining 32 are the second operand in bit-args format.
 * @return The operation result is expressed in bit-args format.
 *
 * Example usage:
 * @code
 * EXPR_XOR(Z_EXPR_HEX(0x3), Z_EXPR_HEX(0x2)) // 0x1 in bit-args format
 * @endcode
 */
#define EXPR_XOR(...) Z_EXPR_XOR(__VA_ARGS__)

/**
 * @brief Performs a bitwise NOT operation.
 *
 * @param ... The operand in bit-args format.
 * @return The operation result is expressed in bit-args format.
 *
 * Example usage:
 * @code
 * EXPR_NOT(Z_EXPR_HEX(0x3)) // 0xFFFFFFFC in bit-args format
 * @endcode
 */
#define EXPR_NOT(...) Z_EXPR_NOT(__VA_ARGS__)

/**
 * @brief Converts a binary number in bit args to hexadecimal.
 *
 * @param ... 32 bit args representing the binary number.
 * @return Hexadecimal representation of the binary number.
 */
#define EXPR_TO_NUM(...) Z_EXPR_TO_NUM(__VA_ARGS__)

/**
 * @brief Performs a left shift operation.
 *
 * @param ... It takes 64 arguments as two bit-args numbers.
 * @param ... The first 32 are the number to shift in bit-args format.
 * @param ... The remaining 32 are the shift amount in bit-args format.
 * @return The operation result is expressed in bit-args format.
 *
 * Example usage:
 * @code
 * EXPR_LSH(Z_EXPR_HEX(0x3), Z_EXPR_HEX(0x2)) // 0xc in bit-args format
 * @endcode
 */
#define EXPR_LSH(...) Z_EXPR_LSH(__VA_ARGS__)

/**
 * @brief Performs a right shift operation.
 *
 * @param ... It takes 64 arguments as two bit-args numbers.
 * @param ... The first 32 are the number to shift in bit-args format.
 * @param ... The remaining 32 are the shift amount in bit-args format.
 * @return The operation result is expressed in bit-args format.
 *
 * Example usage:
 * @code
 * EXPR_RSH(Z_EXPR_HEX(0x3), Z_EXPR_HEX(0x2)) // 0x0 in bit-args format
 * @endcode
 */
#define EXPR_RSH(...) Z_EXPR_RSH(__VA_ARGS__)

/**
 * @brief Checks equality between two bit-args.
 *
 * @param ... It takes 64 arguments as two bit-args numbers.
 * @param ... The first 32 are the left-hand side comparand in bit-args format.
 * @param ... The remaining 32 are the right-hand side comparand in bit-args format.
 * @return The operation result is expressed in bit-args format.
 *
 * Example usage:
 * @code
 * EXPR_EQ(Z_EXPR_HEX(0x3), Z_EXPR_HEX(0x2)) // 0x0 in bit-args format
 * @endcode
 */
#define EXPR_EQ(...) Z_EXPR_EQ(__VA_ARGS__)

/**
 * @brief Checks if the first bit arg is greater than the second bit arg.
 *
 * @param ... It takes 64 arguments as two bit-args numbers.
 * @param ... The first 32 are the left-hand side comparand in bit-args format.
 * @param ... The remaining 32 are the right-hand side comparand in bit-args format.
 * @return The operation result is expressed in bit-args format.
 *
 * Example usage:
 * @code
 * EXPR_GT(Z_EXPR_HEX(0x3), Z_EXPR_HEX(0x2)) // 0x1 in bit-args format
 * @endcode
 */
#define EXPR_GT(...) Z_EXPR_GT(__VA_ARGS__)

/**
 * @brief Checks if the first bit arg is less than the second bit arg.
 *
 * @param ... It takes 64 arguments as two bit-args numbers.
 * @param ... The first 32 are the left-hand side comparand in bit-args format.
 * @param ... The remaining 32 are the right-hand side comparand in bit-args format.
 * @return The operation result is expressed in bit-args format.
 *
 * Example usage:
 * @code
 * EXPR_LT(Z_EXPR_HEX(0x3), Z_EXPR_HEX(0x2)) // 0x0 in bit-args format
 * @endcode
 */
#define EXPR_LT(...) Z_EXPR_LT(__VA_ARGS__)

/**
 * @brief Checks if the first 32-bit number is greater than or equal to the second.
 *
 * @param ... It takes 64 arguments as two bit-args numbers.
 * @param ... The first 32 are the left-hand side comparand in bit-args format.
 * @param ... The remaining 32 are the right-hand side comparand in bit-args format.
 * @return The operation result is expressed in bit-args format.
 *
 * Example usage:
 * @code
 * EXPR_GTE(Z_EXPR_HEX(0x3), Z_EXPR_HEX(0x2)) // 0x1 in bit-args format
 * @endcode
 */
#define EXPR_GTE(...) Z_EXPR_GTE(__VA_ARGS__)

/**
 * @brief Checks if the first 32-bit number is less than or equal to the second.
 *
 * @param ... It takes 64 arguments as two bit-args numbers.
 * @param ... The first 32 are the left-hand side comparand in bit-args format.
 * @param ... The remaining 32 are the right-hand side comparand in bit-args format.
 * @return The operation result is expressed in bit-args format.
 *
 * Example usage:
 * @code
 * EXPR_LTE(Z_EXPR_HEX(0x3), Z_EXPR_HEX(0x2)) // 0x0 in bit-args format
 * @endcode
 */
#define EXPR_LTE(...) Z_EXPR_LTE(__VA_ARGS__)

/**
 * @brief Checks equality between two bit args.
 *
 * @param ... It takes 64 arguments as two bit-args numbers.
 * @param ... The first 32 are the left-hand side comparand in bit-args format.
 * @param ... The remaining 32 are the right-hand side comparand in bit-args format.
 * @return The operation result in 0(false) or 1(true).
 *
 * Example usage:
 * @code
 * EXPR_IS_EQ(Z_EXPR_HEX(0x3), Z_EXPR_HEX(0x2)) // 0 (false)
 * @endcode
 */
#define EXPR_IS_EQ(...) Z_EXPR_IS_EQ(__VA_ARGS__)

/**
 * @brief Checks if the first bit-args is greater than the second bit-args.
 *
 * @param ... It takes 64 arguments as two bit-args numbers.
 * @param ... The first 32 are the left-hand side comparand in bit-args format.
 * @param ... The remaining 32 are the right-hand side comparand in bit-args format.
 * @return The operation result in 0(false) or 1(true).
 *
 * Example usage:
 * @code
 * EXPR_IS_GT(Z_EXPR_HEX(0x3), Z_EXPR_HEX(0x2)) // 1 (true)
 * @endcode
 */
#define EXPR_IS_GT(...) Z_EXPR_IS_GT(__VA_ARGS__)

/**
 * @brief Checks if the first bit arg is less than the second bit arg.
 *
 * @param ... It takes 64 arguments as two bit-args numbers.
 * @param ... The first 32 are the left-hand side comparand in bit-args format.
 * @param ... The remaining 32 are the right-hand side comparand in bit-args format.
 * @return The operation result in 0(false) or 1(true).
 *
 * Example usage:
 * @code
 * EXPR_IS_LT(Z_EXPR_HEX(0x3), Z_EXPR_HEX(0x2)) // 0 (false)
 * @endcode
 */
#define EXPR_IS_LT(...) Z_EXPR_IS_LT(__VA_ARGS__)

/**
 * @brief Checks if the first 32-bit number is greater than or equal to the second.
 *
 * @param ... It takes 64 arguments as two bit-args numbers.
 * @param ... The first 32 are the left-hand side comparand in bit-args format.
 * @param ... The remaining 32 are the right-hand side comparand in bit-args format.
 * @return The operation result in 0(false) or 1(true).
 *
 * Example usage:
 * @code
 * EXPR_IS_GTE(Z_EXPR_HEX(0x3), Z_EXPR_HEX(0x2)) // 1 (true)
 * @endcode
 */
#define EXPR_IS_GTE(...) Z_EXPR_IS_GTE(__VA_ARGS__)

/**
 * @brief Checks if the first 32-bit number is less than or equal to the second.
 *
 * @param ... It takes 64 arguments as two bit-args numbers.
 * @param ... The first 32 are the left-hand side comparand in bit-args format.
 * @param ... The remaining 32 are the right-hand side comparand in bit-args format.
 * @return The operation result in 0(false) or 1(true).
 *
 * Example usage:
 * @code
 * EXPR_IS_LTE(Z_EXPR_HEX(0x3), Z_EXPR_HEX(0x2)) // 0 (false)
 * @endcode
 */
#define EXPR_IS_LTE(...) Z_EXPR_IS_LTE(__VA_ARGS__)

/**
 * @brief Performs addition operation.
 *
 * @param ... It takes 64 arguments as two bit-args numbers.
 * @param ... The first 32 are the augend in bit-args format.
 * @param ... The remaining 32 are the addend in bit-args format.
 * @return The operation result is expressed in bit-args format.
 *
 * Example usage:
 * @code
 * EXPR_ADD(Z_EXPR_HEX(0x3), Z_EXPR_HEX(0x2)) // 0x0 in bit-args format
 * @endcode
 */
#define EXPR_ADD(...) Z_EXPR_ADD(__VA_ARGS__)

/**
 * @brief Performs subtraction operation.
 *
 * @param ... It takes 64 arguments as two bit-args numbers.
 * @param ... The first 32 are the minuend in bit-args format.
 * @param ... The remaining 32 are the subtrahend in bit-args format.
 * @return The operation result is expressed in bit-args format.
 *
 * Example usage:
 * @code
 * EXPR_ADD(Z_EXPR_HEX(0x3), Z_EXPR_HEX(0x2)) // 0x1 in bit-args format
 * @endcode
 */
#define EXPR_SUB(...) Z_EXPR_SUB(__VA_ARGS__)

/**
 * @brief Performs multiplication operation.
 *
 * @param ... It takes 64 arguments as two bit-args numbers.
 * @param ... The first 32 are the multiplicand in bit-args format.
 * @param ... The remaining 32 are the multiplier in bit-args format.
 * @return The operation result is expressed in bit-args format.
 *
 * Example usage:
 * @code
 * EXPR_ADD(Z_EXPR_HEX(0x3), Z_EXPR_HEX(0x2)) // 0x6 in bit-args format
 * @endcode
 */
#define EXPR_MUL(...) Z_EXPR_MUL(__VA_ARGS__)

/**
 * @brief Performs division operation.
 *
 * @param ... It takes 64 arguments as two bit-args numbers.
 * @param ... The first 32 are the dividend in bit-args format.
 * @param ... The remaining 32 are the divisor in bit-args format.
 * @return The operation result expressed in bit-args format.
 *
 * @note Division by zero is undefined behavior.
 *
 * Example usage:
 * @code
 * EXPR_ADD(Z_EXPR_HEX(0x3), Z_EXPR_HEX(0x2)) // 0x1 in bit-args format
 * @endcode
 */
#define EXPR_DIV(...) Z_EXPR_DIV(__VA_ARGS__)

/**
 * @brief Performs modulo operation.
 *
 * @note Division by zero is undefined behavior.
 *
 * @param ... It takes 64 arguments as two bit-args numbers.
 * @param ... The first 32 are the dividend in bit-args format.
 * @param ... The remaining 32 are the divisor in bit-args format.
 * @return The operation result is expressed in bit-args format.
 *
 * Example usage:
 * @code
 * EXPR_ADD(Z_EXPR_HEX(0x3), Z_EXPR_HEX(0x2)) // 0x1 in bit-args format
 * @endcode
 */
#define EXPR_MOD(...) Z_EXPR_MOD(__VA_ARGS__)

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_UTIL_EXPR_H_ */
