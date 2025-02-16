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

#include <zephyr/expr_dec_generated.h>
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
#define EXPR_HEX(h) UTIL_CAT(Z_EXPR_HEX_, h)

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
#define EXPR_DEC(d) UTIL_CAT(Z_EXPR_DEC_, d)

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
#define EXPR_HEX_ENCODE(...) Z_EXPR_HEX_ENCODE(__VA_ARGS__)

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
 * EXPR_LSHIFT(Z_EXPR_HEX(0x3), Z_EXPR_HEX(0x2)) // 0xc in bit-args format
 * @endcode
 */
#define EXPR_LSHIFT(...) Z_EXPR_LSHIFT(__VA_ARGS__)

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
 * EXPR_RSHIFT(Z_EXPR_HEX(0x3), Z_EXPR_HEX(0x2)) // 0x0 in bit-args format
 * @endcode
 */
#define EXPR_RSHIFT(...) Z_EXPR_RSHIFT(__VA_ARGS__)

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
 * @brief Performs a bitwise OR operation on two hexadecimal values.
 *
 * @param a First hexadecimal value.
 * @param b Second hexadecimal value.
 * @return The operation result in hexadecimal format.
 */
#define EXPR_OR_HEX(a, b) Z_EXPR_HEX_ENCODE(EXPR_OR(EXPR_HEX(a), EXPR_HEX(b)))

/**
 * @brief Performs a bitwise AND operation on two hexadecimal values.
 *
 * @param a First hexadecimal value.
 * @param b Second hexadecimal value.
 * @return The result of the AND operation is in hexadecimal format.
 */
#define EXPR_AND_HEX(a, b) Z_EXPR_HEX_ENCODE(EXPR_AND(EXPR_HEX(a), EXPR_HEX(b)))

/**
 * @brief Performs a bitwise XOR operation on two hexadecimal values.
 *
 * @param a First hexadecimal value.
 * @param b Second hexadecimal value.
 * @return The result of the XOR operation is in hexadecimal format.
 */
#define EXPR_XOR_HEX(a, b) Z_EXPR_HEX_ENCODE(EXPR_XOR(EXPR_HEX(a), EXPR_HEX(b)))

/**
 * @brief Performs a bitwise NOT operation on a hexadecimal value.
 *
 * @param a Hexadecimal value.
 * @return The result of the NOT operation is in hexadecimal format.
 */
#define EXPR_NOT_HEX(a) Z_EXPR_HEX_ENCODE(EXPR_NOT(EXPR_HEX(a)))

/**
 * @brief Performs a left shift operation on a hexadecimal value.
 *
 * @param a Hexadecimal value.
 * @param n Number of positions to shift.
 * @return The result of the left shift operation is in hexadecimal format.
 */
#define EXPR_LSHIFT_HEX(a, n) Z_EXPR_HEX_ENCODE(EXPR_LSHIFT(n, EXPR_HEX(a)))

/**
 * @brief Performs a right shift operation on a hexadecimal value.
 *
 * @param a Hexadecimal value.
 * @param n Number of positions to shift.
 * @return The result of the right shift operation is in hexadecimal format.
 */
#define EXPR_RSHIFT_HEX(a, n) Z_EXPR_HEX_ENCODE(EXPR_RSHIFT(n, EXPR_HEX(a)))

/**
 * @brief Checks equality between two hexadecimal values.
 *
 * @param a First hexadecimal value.
 * @param b Second hexadecimal value.
 * @return 1 if the values are equal otherwise, 0.
 */
#define EXPR_EQ_HEX(a, b) Z_EXPR_HEX_ENCODE(EXPR_EQ(EXPR_HEX(a), EXPR_HEX(b)))

/**
 * @brief Checks if the first hexadecimal value is less than the second.
 *
 * @param a First hexadecimal value.
 * @param b Second hexadecimal value.
 * @return 1 if the first value is less otherwise, 0.
 */
#define EXPR_LT_HEX(a, b) Z_EXPR_HEX_ENCODE(EXPR_LT(EXPR_HEX(a), EXPR_HEX(b)))

/**
 * @brief Checks if the first hexadecimal value is greater than the second.
 *
 * @param a First hexadecimal value.
 * @param b Second hexadecimal value.
 * @return 1 if the first value is greater otherwise, 0.
 */
#define EXPR_GT_HEX(a, b) Z_EXPR_HEX_ENCODE(EXPR_GT(EXPR_HEX(a), EXPR_HEX(b)))

/**
 * @brief Checks if the first hexadecimal value is less than or equal to the second.
 *
 * @param a First hexadecimal value.
 * @param b Second hexadecimal value.
 * @return 1 if the first value is less than or equal otherwise, 0.
 */
#define EXPR_LTE_HEX(a, b) Z_EXPR_HEX_ENCODE(EXPR_LTE(EXPR_HEX(a), EXPR_HEX(b)))

/**
 * @brief Checks if the first hexadecimal value is greater than or equal to the second.
 *
 * @param a First hexadecimal value.
 * @param b Second hexadecimal value.
 * @return 1 if the first value is greater than or equal otherwise, 0.
 */
#define EXPR_GTE_HEX(a, b) Z_EXPR_HEX_ENCODE(EXPR_GTE(EXPR_HEX(a), EXPR_HEX(b)))

/**
 * @brief Computes the sum of two hexadecimal values.
 *
 * @param a The augend in hexadecimal value.
 * @param b The addend hexadecimal value.
 * @return The result of the addition in hexadecimal format.
 */
#define EXPR_ADD_HEX(a, b) Z_EXPR_HEX_ENCODE(EXPR_ADD(EXPR_HEX(a), EXPR_HEX(b)))

/**
 * @brief Computes the difference between two hexadecimal values.
 *
 * @param a The minuend hexadecimal value.
 * @param b The subtrahend in hexadecimal value.
 * @return The result of the subtraction in hexadecimal format.
 */
#define EXPR_SUB_HEX(a, b) Z_EXPR_HEX_ENCODE(EXPR_SUB(EXPR_HEX(a), EXPR_HEX(b)))

/**
 * @brief Computes the product of two hexadecimal values.
 *
 * @param a First hexadecimal value.
 * @param b Second hexadecimal value.
 * @return The result of the multiplication in hexadecimal format.
 */
#define EXPR_MUL_HEX(a, b) Z_EXPR_HEX_ENCODE(EXPR_MUL(EXPR_HEX(a), EXPR_HEX(b)))

/**
 * @brief Computes the division of two hexadecimal values.
 *
 * @param a The dividend in hexadecimal value.
 * @param b The divisor hexadecimal value.
 * @return The result of the division in hexadecimal format.
 */
#define EXPR_DIV_HEX(a, b) Z_EXPR_HEX_ENCODE(EXPR_DIV(EXPR_HEX(a), EXPR_HEX(b)))

/**
 * @brief Computes the modulus of two hexadecimal values.
 *
 * @param a The dividend in hexadecimal value.
 * @param b The divisor hexadecimal value.
 * @return The result of the modulus in hexadecimal format.
 */
#define EXPR_MOD_HEX(a, b) Z_EXPR_HEX_ENCODE(EXPR_MOD(EXPR_HEX(a), EXPR_HEX(b)))

/**
 * @brief Like <tt>a == b</tt>, but does evaluation and
 * short-circuiting at C preprocessor time.
 *
 * This however only works for integer literal from 0 to 4096 (literals with U suffix,
 * e.g. 0U are also included).
 *
 * Examples:
 *
 *   IS_EQ(1, 1)   -> 1
 *   IS_EQ(1U, 1U) -> 1
 *   IS_EQ(1U, 1)  -> 1
 *   IS_EQ(1, 1U)  -> 1
 *   IS_EQ(1, 0)   -> 0
 *
 * @param a Integer literal (can be with U suffix)
 * @param b Integer literal
 *
 */
#define IS_EQ(a, b) EXPR_IS_EQ(EXPR_DEC(a), EXPR_DEC(b))

#define UTIL_X2_X(...) Z_EXPR_HEX_ENCODE(__VA_ARGS__)
/**
 * @brief UTIL_X2(y) for an integer literal y from 0 to 4095 expands to an
 * integer literal whose value is 2y.
 */
#define UTIL_X2(y)     UTIL_X2_X(EXPR_LSHIFT(EXPR_DEC(y), EXPR_DEC(1)))

#define UTIL_ADD_X(...) Z_EXPR_HEX_ENCODE(__VA_ARGS__)
/**
 * @brief UTIL_ADD(x, n) executes n-times UTIL_INC on x to create the value x + n.
 *        Both the arguments and the result of the operation must be in the range
 *        of 0 to 4095.
 */
#define UTIL_ADD(a, b)  UTIL_ADD_X(EXPR_ADD(EXPR_DEC(a), EXPR_DEC(b)))

#define UTIL_SUB_XX(...) Z_EXPR_HEX_ENCODE(__VA_ARGS__)
#define UTIL_SUB_X(...)                                                                            \
	UTIL_SUB_XX(COND_CODE_1(Z_EXPR_IS_GT(GET_ARGS_FIRST_N(32, __VA_ARGS__),                    \
					     GET_ARGS_LESS_N(32, __VA_ARGS__)),                    \
			        (Z_EXPR_NUM_0),                                                    \
				(GET_ARGS_FIRST_N(32, __VA_ARGS__))))

/**
 * @brief UTIL_SUB(x, n) executes n-times UTIL_DEC on x to create the value x - n.
 *        Both the arguments and the result of the operation must be in the range
 *        of 0 to 4095.
 */
#define UTIL_SUB(a, b) UTIL_SUB_X(EXPR_SUB(EXPR_DEC(a), EXPR_DEC(b)), EXPR_DEC(a))

/**
 * @brief UTIL_INC(x) for an integer literal x from 0 to 4095 expands to an
 * integer literal whose value is x+1.
 *
 * @see UTIL_DEC(x)
 */
#define UTIL_INC(x) UTIL_ADD(x, 1)

/**
 * @brief UTIL_DEC(x) for an integer literal x from 0 to 4095 expands to an
 * integer literal whose value is x-1.
 *
 * @see UTIL_INC(x)
 */
#define UTIL_DEC(x) UTIL_SUB(x, 1)

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_UTIL_EXPR_H_ */
