/*
 * Copyright (c) 2025 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_UTIL_INTERNAL_EXPR_H_
#define ZEPHYR_INCLUDE_SYS_UTIL_INTERNAL_EXPR_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/sys/util_macro.h>
#include "util_expr_num.h"

#define Z_EXPR_NUM_0                                                                               \
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  \
		0, 0
#define Z_EXPR_NUM_1                                                                               \
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  \
		0, 1

#define Z_EXPR_IS_ZERO_0  1
#define Z_EXPR_IS_ZERO(n) UTIL_CAT(Z_EXPR_IS_ZERO_, n)

/*
 * Z_EXPR_BIN2DEC_<BinaryString>
 * Converts a 5-bit binary string to its decimal representation.
 */

#define Z_EXPR_BIN2DEC_00000 0
#define Z_EXPR_BIN2DEC_00001 1
#define Z_EXPR_BIN2DEC_00010 2
#define Z_EXPR_BIN2DEC_00011 3
#define Z_EXPR_BIN2DEC_00100 4
#define Z_EXPR_BIN2DEC_00101 5
#define Z_EXPR_BIN2DEC_00110 6
#define Z_EXPR_BIN2DEC_00111 7
#define Z_EXPR_BIN2DEC_01000 8
#define Z_EXPR_BIN2DEC_01001 9
#define Z_EXPR_BIN2DEC_01010 10
#define Z_EXPR_BIN2DEC_01011 11
#define Z_EXPR_BIN2DEC_01100 12
#define Z_EXPR_BIN2DEC_01101 13
#define Z_EXPR_BIN2DEC_01110 14
#define Z_EXPR_BIN2DEC_01111 15
#define Z_EXPR_BIN2DEC_10000 16
#define Z_EXPR_BIN2DEC_10001 17
#define Z_EXPR_BIN2DEC_10010 18
#define Z_EXPR_BIN2DEC_10011 19
#define Z_EXPR_BIN2DEC_10100 20
#define Z_EXPR_BIN2DEC_10101 21
#define Z_EXPR_BIN2DEC_10110 22
#define Z_EXPR_BIN2DEC_10111 23
#define Z_EXPR_BIN2DEC_11000 24
#define Z_EXPR_BIN2DEC_11001 25
#define Z_EXPR_BIN2DEC_11010 26
#define Z_EXPR_BIN2DEC_11011 27
#define Z_EXPR_BIN2DEC_11100 28
#define Z_EXPR_BIN2DEC_11101 29
#define Z_EXPR_BIN2DEC_11110 30
#define Z_EXPR_BIN2DEC_11111 31

/*
 * Z_EXPR_BIN2HEX_<BinaryString>
 * Converts a 4-bit binary string to its hexadecimal representation.
 */

#define Z_EXPR_BIN2HEX_0000 0
#define Z_EXPR_BIN2HEX_0001 1
#define Z_EXPR_BIN2HEX_0010 2
#define Z_EXPR_BIN2HEX_0011 3
#define Z_EXPR_BIN2HEX_0100 4
#define Z_EXPR_BIN2HEX_0101 5
#define Z_EXPR_BIN2HEX_0110 6
#define Z_EXPR_BIN2HEX_0111 7
#define Z_EXPR_BIN2HEX_1000 8
#define Z_EXPR_BIN2HEX_1001 9
#define Z_EXPR_BIN2HEX_1010 a
#define Z_EXPR_BIN2HEX_1011 b
#define Z_EXPR_BIN2HEX_1100 c
#define Z_EXPR_BIN2HEX_1101 d
#define Z_EXPR_BIN2HEX_1110 e
#define Z_EXPR_BIN2HEX_1111 f

#define Z_EXPR_BIT_NOT_0 1
#define Z_EXPR_BIT_NOT_1 0

#define Z_EXPR_BIT_NOT_X(x) Z_EXPR_BIT_NOT_##x

/**
 * @brief Computes the bitwise NOT
 *
 * @p ... A bit operand. Only allow 0 or 1.
 *
 * Example:
 * @code
 * Z_EXPR_BIT_NOT(0) // Expands to 1
 * @endcode
 */
#define Z_EXPR_BIT_NOT(...) Z_EXPR_BIT_NOT_X(__VA_ARGS__)

#define Z_EXPR_BIT_AND_00 0
#define Z_EXPR_BIT_AND_01 0
#define Z_EXPR_BIT_AND_10 0
#define Z_EXPR_BIT_AND_11 1

#define Z_EXPR_BIT_AND_X(x, y) Z_EXPR_BIT_AND_##x##y

/**
 * @brief Computes the bitwise AND
 *
 * @p ... Two of bit operands. Only allow 0 or 1.
 *
 * Example:
 * @code
 * Z_EXPR_BIT_AND(0, 1) // Expands to 0
 * @endcode
 */
#define Z_EXPR_BIT_AND(...) Z_EXPR_BIT_AND_X(__VA_ARGS__)

#define Z_EXPR_BIT_OR_00 0
#define Z_EXPR_BIT_OR_01 1
#define Z_EXPR_BIT_OR_10 1
#define Z_EXPR_BIT_OR_11 1

#define Z_EXPR_BIT_OR_X(x, y) Z_EXPR_BIT_OR_##x##y

/**
 * Computes the bitwise OR
 *
 * @p ... Two of bit operands. Only allow 0 or 1.
 *
 * Example:
 * @code
 * Z_EXPR_BIT_OR(0, 1) // Expands to 1
 * @endcode
 */
#define Z_EXPR_BIT_OR(...) Z_EXPR_BIT_OR_X(__VA_ARGS__)

#define Z_EXPR_BIT_XOR_00 0
#define Z_EXPR_BIT_XOR_01 1
#define Z_EXPR_BIT_XOR_10 1
#define Z_EXPR_BIT_XOR_11 0

#define Z_EXPR_BIT_XOR_X(x, y) Z_EXPR_BIT_XOR_##x##y

/**
 * @brief Computes the bitwise XOR
 *
 * @p ... Two of bit operands. Only allow 0 or 1.
 *
 * Example:
 * @code
 * Z_EXPR_BIT_XOR(0, 1) // Expands to 1
 * @endcode
 */
#define Z_EXPR_BIT_XOR(...) Z_EXPR_BIT_XOR_X(__VA_ARGS__)

/**
 * Converts a 5-bit binary args to its decimal equivalent.
 *
 * @param ... Five bits (0 or 1).
 * @return Decimal equivalent of the binary number.
 *
 * Example:
 * @code
 * Z_EXPR_BIN2DEC_5(0, 0, 1, 0, 1) // Expands to 5
 * @endcode
 */
#define Z_EXPR_BIN2DEC_5(...) _CONCAT_6(Z_EXPR_BIN2DEC_, GET_ARGS_FIRST_N(5, __VA_ARGS__))

/**
 * Converts a 4-bit binary args to its hexadecimal equivalent.
 *
 * @param ... Four bits (0 or 1).
 * @return Hexadecimal equivalent of the binary number.
 *
 * Example:
 * @code
 * Z_EXPR_BIN2HEX_4(1, 0, 1, 0) // Expands to 'a'
 * @endcode
 */
#define Z_EXPR_BIN2HEX_4(...) UTIL_CAT(Z_EXPR_BIN2HEX_, _CONCAT_4(__VA_ARGS__))

#define Z_EXPR_TRIM_LZERO_N(n, ...)                                                                \
	COND_CODE_1(Z_EXPR_IS_ZERO(GET_ARG_N(1, __VA_ARGS__)),                                           \
		    (Z_UTIL_DEC(n), GET_ARGS_LESS_N(1, __VA_ARGS__)), (n, __VA_ARGS__))

#define Z_EXPR_TRIM_LZERO_0(...)                                                                   \
	COND_CODE_1(Z_EXPR_IS_ZERO(GET_ARG_N(1, __VA_ARGS__)),                 \
					    (1, 0), (__VA_ARGS__))
#define Z_EXPR_TRIM_LZERO_1(...)  Z_EXPR_TRIM_LZERO_0(Z_EXPR_TRIM_LZERO_N(__VA_ARGS__))
#define Z_EXPR_TRIM_LZERO_2(...)  Z_EXPR_TRIM_LZERO_1(Z_EXPR_TRIM_LZERO_N(__VA_ARGS__))
#define Z_EXPR_TRIM_LZERO_3(...)  Z_EXPR_TRIM_LZERO_2(Z_EXPR_TRIM_LZERO_N(__VA_ARGS__))
#define Z_EXPR_TRIM_LZERO_4(...)  Z_EXPR_TRIM_LZERO_3(Z_EXPR_TRIM_LZERO_N(__VA_ARGS__))
#define Z_EXPR_TRIM_LZERO_5(...)  Z_EXPR_TRIM_LZERO_4(Z_EXPR_TRIM_LZERO_N(__VA_ARGS__))
#define Z_EXPR_TRIM_LZERO_6(...)  Z_EXPR_TRIM_LZERO_5(Z_EXPR_TRIM_LZERO_N(__VA_ARGS__))
#define Z_EXPR_TRIM_LZERO_7(...)  Z_EXPR_TRIM_LZERO_6(Z_EXPR_TRIM_LZERO_N(__VA_ARGS__))
#define Z_EXPR_TRIM_LZERO_8(...)  Z_EXPR_TRIM_LZERO_7(Z_EXPR_TRIM_LZERO_N(__VA_ARGS__))
#define Z_EXPR_TRIM_LZERO_9(...)  Z_EXPR_TRIM_LZERO_8(Z_EXPR_TRIM_LZERO_N(__VA_ARGS__))
#define Z_EXPR_TRIM_LZERO_10(...) Z_EXPR_TRIM_LZERO_9(Z_EXPR_TRIM_LZERO_N(__VA_ARGS__))
#define Z_EXPR_TRIM_LZERO_11(...) Z_EXPR_TRIM_LZERO_10(Z_EXPR_TRIM_LZERO_N(__VA_ARGS__))
#define Z_EXPR_TRIM_LZERO_12(...) Z_EXPR_TRIM_LZERO_11(Z_EXPR_TRIM_LZERO_N(__VA_ARGS__))
#define Z_EXPR_TRIM_LZERO_13(...) Z_EXPR_TRIM_LZERO_12(Z_EXPR_TRIM_LZERO_N(__VA_ARGS__))
#define Z_EXPR_TRIM_LZERO_14(...) Z_EXPR_TRIM_LZERO_13(Z_EXPR_TRIM_LZERO_N(__VA_ARGS__))
#define Z_EXPR_TRIM_LZERO_15(...) Z_EXPR_TRIM_LZERO_14(Z_EXPR_TRIM_LZERO_N(__VA_ARGS__))
#define Z_EXPR_TRIM_LZERO_16(...) Z_EXPR_TRIM_LZERO_15(Z_EXPR_TRIM_LZERO_N(__VA_ARGS__))
#define Z_EXPR_TRIM_LZERO_17(...) Z_EXPR_TRIM_LZERO_16(Z_EXPR_TRIM_LZERO_N(__VA_ARGS__))
#define Z_EXPR_TRIM_LZERO_18(...) Z_EXPR_TRIM_LZERO_17(Z_EXPR_TRIM_LZERO_N(__VA_ARGS__))
#define Z_EXPR_TRIM_LZERO_19(...) Z_EXPR_TRIM_LZERO_18(Z_EXPR_TRIM_LZERO_N(__VA_ARGS__))
#define Z_EXPR_TRIM_LZERO_20(...) Z_EXPR_TRIM_LZERO_19(Z_EXPR_TRIM_LZERO_N(__VA_ARGS__))
#define Z_EXPR_TRIM_LZERO_21(...) Z_EXPR_TRIM_LZERO_20(Z_EXPR_TRIM_LZERO_N(__VA_ARGS__))
#define Z_EXPR_TRIM_LZERO_22(...) Z_EXPR_TRIM_LZERO_21(Z_EXPR_TRIM_LZERO_N(__VA_ARGS__))
#define Z_EXPR_TRIM_LZERO_23(...) Z_EXPR_TRIM_LZERO_22(Z_EXPR_TRIM_LZERO_N(__VA_ARGS__))
#define Z_EXPR_TRIM_LZERO_24(...) Z_EXPR_TRIM_LZERO_23(Z_EXPR_TRIM_LZERO_N(__VA_ARGS__))
#define Z_EXPR_TRIM_LZERO_25(...) Z_EXPR_TRIM_LZERO_24(Z_EXPR_TRIM_LZERO_N(__VA_ARGS__))
#define Z_EXPR_TRIM_LZERO_26(...) Z_EXPR_TRIM_LZERO_25(Z_EXPR_TRIM_LZERO_N(__VA_ARGS__))
#define Z_EXPR_TRIM_LZERO_27(...) Z_EXPR_TRIM_LZERO_26(Z_EXPR_TRIM_LZERO_N(__VA_ARGS__))
#define Z_EXPR_TRIM_LZERO_28(...) Z_EXPR_TRIM_LZERO_27(Z_EXPR_TRIM_LZERO_N(__VA_ARGS__))
#define Z_EXPR_TRIM_LZERO_29(...) Z_EXPR_TRIM_LZERO_28(Z_EXPR_TRIM_LZERO_N(__VA_ARGS__))
#define Z_EXPR_TRIM_LZERO_30(...) Z_EXPR_TRIM_LZERO_29(Z_EXPR_TRIM_LZERO_N(__VA_ARGS__))
#define Z_EXPR_TRIM_LZERO_31(...) Z_EXPR_TRIM_LZERO_30(Z_EXPR_TRIM_LZERO_N(__VA_ARGS__))
#define Z_EXPR_TRIM_LZERO_32(...) Z_EXPR_TRIM_LZERO_31(Z_EXPR_TRIM_LZERO_N(__VA_ARGS__))

/**
 * Trims leading zeros from args.
 *
 * @param n Maximum number of iterations for trimming.
 * @param ... list of numbers.
 * @return The list: The first element is the removed-zero number,
 *         and the following is args with leading zeros removed.
 *
 * Example:
 * @code
 * Z_EXPR_TRIM_LZERO(8, 0, 0, 1, 0) // Expands to 1, 0
 * @endcode
 */
#define Z_EXPR_TRIM_LZERO(n, ...) UTIL_CAT(Z_EXPR_TRIM_LZERO_, n)(n, __VA_ARGS__)

#define Z_EXPR_GET_ARGS_LESS_N(...) GET_ARGS_LESS_N(__VA_ARGS__)

#define Z_EXPR_TRIM_LZERO_AND_CONCAT___(fn, ...) fn(__VA_ARGS__)
#define Z_EXPR_TRIM_LZERO_AND_CONCAT__(...)      Z_EXPR_TRIM_LZERO_AND_CONCAT___(__VA_ARGS__)
#define Z_EXPR_TRIM_LZERO_AND_CONCAT_(n, ...)                                                      \
	Z_EXPR_TRIM_LZERO_AND_CONCAT__(UTIL_CAT(_CONCAT_, n), __VA_ARGS__)
#define Z_EXPR_TRIM_LZERO_AND_CONCAT(...)                                                          \
	Z_EXPR_TRIM_LZERO_AND_CONCAT_(Z_EXPR_TRIM_LZERO(8, __VA_ARGS__))

#define Z_EXPR_HEX_ENCODE(a31, a30, a29, a28, a27, a26, a25, a24, a23, a22, a21, a20, a19, a18,    \
			  a17, a16, a15, a14, a13, a12, a11, a10, a09, a08, a07, a06, a05, a04,    \
			  a03, a02, a01, a00)                                                      \
	UTIL_CAT(0x, Z_EXPR_TRIM_LZERO_AND_CONCAT(Z_EXPR_BIN2HEX_4(a31, a30, a29, a28),            \
						  Z_EXPR_BIN2HEX_4(a27, a26, a25, a24),            \
						  Z_EXPR_BIN2HEX_4(a23, a22, a21, a20),            \
						  Z_EXPR_BIN2HEX_4(a19, a18, a17, a16),            \
						  Z_EXPR_BIN2HEX_4(a15, a14, a13, a12),            \
						  Z_EXPR_BIN2HEX_4(a11, a10, a09, a08),            \
						  Z_EXPR_BIN2HEX_4(a07, a06, a05, a04),            \
						  Z_EXPR_BIN2HEX_4(a03, a02, a01, a00)))

/*
 * Create tuple (a31, b31, a30, b30,...) list from two 32-bit sequences (a31..a00 and b31..b00).
 */
#define Z_EXPR_TUPLIFY(a31, a30, a29, a28, a27, a26, a25, a24, a23, a22, a21, a20, a19, a18, a17,  \
		       a16, a15, a14, a13, a12, a11, a10, a09, a08, a07, a06, a05, a04, a03, a02,  \
		       a01, a00, b31, b30, b29, b28, b27, b26, b25, b24, b23, b22, b21, b20, b19,  \
		       b18, b17, b16, b15, b14, b13, b12, b11, b10, b09, b08, b07, b06, b05, b04,  \
		       b03, b02, b01, b00)                                                         \
	a31, b31, a30, b30, a29, b29, a28, b28, a27, b27, a26, b26, a25, b25, a24, b24, a23, b23,  \
		a22, b22, a21, b21, a20, b20, a19, b19, a18, b18, a17, b17, a16, b16, a15, b15,    \
		a14, b14, a13, b13, a12, b12, a11, b11, a10, b10, a09, b09, a08, b08, a07, b07,    \
		a06, b06, a05, b05, a04, b04, a03, b03, a02, b02, a01, b01, a00, b00

#define Z_EXPR_N_BITS_OR_N(x, ...)                                                                 \
	Z_EXPR_BIT_OR(x, GET_ARG_N(1, __VA_ARGS__)), GET_ARGS_LESS_N(1, __VA_ARGS__)

#define Z_EXPR_N_BITS_OR_0(...)  __VA_ARGS__
#define Z_EXPR_N_BITS_OR_1(...)  Z_EXPR_N_BITS_OR_0(Z_EXPR_N_BITS_OR_N(__VA_ARGS__))
#define Z_EXPR_N_BITS_OR_2(...)  Z_EXPR_N_BITS_OR_1(Z_EXPR_N_BITS_OR_N(__VA_ARGS__))
#define Z_EXPR_N_BITS_OR_3(...)  Z_EXPR_N_BITS_OR_2(Z_EXPR_N_BITS_OR_N(__VA_ARGS__))
#define Z_EXPR_N_BITS_OR_4(...)  Z_EXPR_N_BITS_OR_3(Z_EXPR_N_BITS_OR_N(__VA_ARGS__))
#define Z_EXPR_N_BITS_OR_5(...)  Z_EXPR_N_BITS_OR_4(Z_EXPR_N_BITS_OR_N(__VA_ARGS__))
#define Z_EXPR_N_BITS_OR_6(...)  Z_EXPR_N_BITS_OR_5(Z_EXPR_N_BITS_OR_N(__VA_ARGS__))
#define Z_EXPR_N_BITS_OR_7(...)  Z_EXPR_N_BITS_OR_6(Z_EXPR_N_BITS_OR_N(__VA_ARGS__))
#define Z_EXPR_N_BITS_OR_8(...)  Z_EXPR_N_BITS_OR_7(Z_EXPR_N_BITS_OR_N(__VA_ARGS__))
#define Z_EXPR_N_BITS_OR_9(...)  Z_EXPR_N_BITS_OR_8(Z_EXPR_N_BITS_OR_N(__VA_ARGS__))
#define Z_EXPR_N_BITS_OR_10(...) Z_EXPR_N_BITS_OR_9(Z_EXPR_N_BITS_OR_N(__VA_ARGS__))
#define Z_EXPR_N_BITS_OR_11(...) Z_EXPR_N_BITS_OR_10(Z_EXPR_N_BITS_OR_N(__VA_ARGS__))
#define Z_EXPR_N_BITS_OR_12(...) Z_EXPR_N_BITS_OR_11(Z_EXPR_N_BITS_OR_N(__VA_ARGS__))
#define Z_EXPR_N_BITS_OR_13(...) Z_EXPR_N_BITS_OR_12(Z_EXPR_N_BITS_OR_N(__VA_ARGS__))
#define Z_EXPR_N_BITS_OR_14(...) Z_EXPR_N_BITS_OR_13(Z_EXPR_N_BITS_OR_N(__VA_ARGS__))
#define Z_EXPR_N_BITS_OR_15(...) Z_EXPR_N_BITS_OR_14(Z_EXPR_N_BITS_OR_N(__VA_ARGS__))
#define Z_EXPR_N_BITS_OR_16(...) Z_EXPR_N_BITS_OR_15(Z_EXPR_N_BITS_OR_N(__VA_ARGS__))
#define Z_EXPR_N_BITS_OR_17(...) Z_EXPR_N_BITS_OR_16(Z_EXPR_N_BITS_OR_N(__VA_ARGS__))
#define Z_EXPR_N_BITS_OR_18(...) Z_EXPR_N_BITS_OR_17(Z_EXPR_N_BITS_OR_N(__VA_ARGS__))
#define Z_EXPR_N_BITS_OR_19(...) Z_EXPR_N_BITS_OR_18(Z_EXPR_N_BITS_OR_N(__VA_ARGS__))
#define Z_EXPR_N_BITS_OR_20(...) Z_EXPR_N_BITS_OR_19(Z_EXPR_N_BITS_OR_N(__VA_ARGS__))
#define Z_EXPR_N_BITS_OR_21(...) Z_EXPR_N_BITS_OR_20(Z_EXPR_N_BITS_OR_N(__VA_ARGS__))
#define Z_EXPR_N_BITS_OR_22(...) Z_EXPR_N_BITS_OR_21(Z_EXPR_N_BITS_OR_N(__VA_ARGS__))
#define Z_EXPR_N_BITS_OR_23(...) Z_EXPR_N_BITS_OR_22(Z_EXPR_N_BITS_OR_N(__VA_ARGS__))
#define Z_EXPR_N_BITS_OR_24(...) Z_EXPR_N_BITS_OR_23(Z_EXPR_N_BITS_OR_N(__VA_ARGS__))
#define Z_EXPR_N_BITS_OR_25(...) Z_EXPR_N_BITS_OR_24(Z_EXPR_N_BITS_OR_N(__VA_ARGS__))
#define Z_EXPR_N_BITS_OR_26(...) Z_EXPR_N_BITS_OR_25(Z_EXPR_N_BITS_OR_N(__VA_ARGS__))
#define Z_EXPR_N_BITS_OR_27(...) Z_EXPR_N_BITS_OR_26(Z_EXPR_N_BITS_OR_N(__VA_ARGS__))
#define Z_EXPR_N_BITS_OR_28(...) Z_EXPR_N_BITS_OR_27(Z_EXPR_N_BITS_OR_N(__VA_ARGS__))
#define Z_EXPR_N_BITS_OR_29(...) Z_EXPR_N_BITS_OR_28(Z_EXPR_N_BITS_OR_N(__VA_ARGS__))
#define Z_EXPR_N_BITS_OR_30(...) Z_EXPR_N_BITS_OR_29(Z_EXPR_N_BITS_OR_N(__VA_ARGS__))
#define Z_EXPR_N_BITS_OR_31(...) Z_EXPR_N_BITS_OR_30(Z_EXPR_N_BITS_OR_N(__VA_ARGS__))
#define Z_EXPR_N_BITS_OR_32(...) Z_EXPR_N_BITS_OR_31(Z_EXPR_N_BITS_OR_N(__VA_ARGS__))

#define Z_EXPR_N_BITS_OR_X(n, ...) UTIL_CAT(Z_EXPR_N_BITS_OR_, Z_UTIL_DEC(n))(__VA_ARGS__)
#define Z_EXPR_N_BITS_OR(...)      GET_ARG_N(1, Z_EXPR_N_BITS_OR_X(__VA_ARGS__))

#define Z_EXPR_IS_TRUE_N(n, ...)                                                                   \
	COND_CODE_1(Z_EXPR_IS_ZERO(n),                                                             \
		    (0),                                                                           \
		    (GET_ARG_N(1, UTIL_CAT(Z_EXPR_N_BITS_OR_, Z_UTIL_DEC(n))(__VA_ARGS__))))

#define Z_EXPR_IS_FALSE_N(...) Z_EXPR_IS_FALSE_N_X(__VA_ARGS__)

/**
 * Performs bitwise OR on two 32-bit numbers in bit-args format.
 */
#define Z_EXPR_OR(a31, a30, a29, a28, a27, a26, a25, a24, a23, a22, a21, a20, a19, a18, a17, a16,  \
		  a15, a14, a13, a12, a11, a10, a09, a08, a07, a06, a05, a04, a03, a02, a01, a00,  \
		  b31, b30, b29, b28, b27, b26, b25, b24, b23, b22, b21, b20, b19, b18, b17, b16,  \
		  b15, b14, b13, b12, b11, b10, b09, b08, b07, b06, b05, b04, b03, b02, b01, b00)  \
	Z_EXPR_BIT_OR(a31, b31), Z_EXPR_BIT_OR(a30, b30), Z_EXPR_BIT_OR(a29, b29),                 \
		Z_EXPR_BIT_OR(a28, b28), Z_EXPR_BIT_OR(a27, b27), Z_EXPR_BIT_OR(a26, b26),         \
		Z_EXPR_BIT_OR(a25, b25), Z_EXPR_BIT_OR(a24, b24), Z_EXPR_BIT_OR(a23, b23),         \
		Z_EXPR_BIT_OR(a22, b22), Z_EXPR_BIT_OR(a21, b21), Z_EXPR_BIT_OR(a20, b20),         \
		Z_EXPR_BIT_OR(a19, b19), Z_EXPR_BIT_OR(a18, b18), Z_EXPR_BIT_OR(a17, b17),         \
		Z_EXPR_BIT_OR(a16, b16), Z_EXPR_BIT_OR(a15, b15), Z_EXPR_BIT_OR(a14, b14),         \
		Z_EXPR_BIT_OR(a13, b13), Z_EXPR_BIT_OR(a12, b12), Z_EXPR_BIT_OR(a11, b11),         \
		Z_EXPR_BIT_OR(a10, b10), Z_EXPR_BIT_OR(a09, b09), Z_EXPR_BIT_OR(a08, b08),         \
		Z_EXPR_BIT_OR(a07, b07), Z_EXPR_BIT_OR(a06, b06), Z_EXPR_BIT_OR(a05, b05),         \
		Z_EXPR_BIT_OR(a04, b04), Z_EXPR_BIT_OR(a03, b03), Z_EXPR_BIT_OR(a02, b02),         \
		Z_EXPR_BIT_OR(a01, b01), Z_EXPR_BIT_OR(a00, b00)

#define Z_EXPR_OR_EXPAND(...) Z_EXPR_OR(__VA_ARGS__)

/**
 * Performs bitwise AND on two 32-bit numbers in bit-args format.
 */
#define Z_EXPR_AND(a31, a30, a29, a28, a27, a26, a25, a24, a23, a22, a21, a20, a19, a18, a17, a16, \
		   a15, a14, a13, a12, a11, a10, a09, a08, a07, a06, a05, a04, a03, a02, a01, a00, \
		   b31, b30, b29, b28, b27, b26, b25, b24, b23, b22, b21, b20, b19, b18, b17, b16, \
		   b15, b14, b13, b12, b11, b10, b09, b08, b07, b06, b05, b04, b03, b02, b01, b00) \
	Z_EXPR_BIT_AND(a31, b31), Z_EXPR_BIT_AND(a30, b30), Z_EXPR_BIT_AND(a29, b29),              \
		Z_EXPR_BIT_AND(a28, b28), Z_EXPR_BIT_AND(a27, b27), Z_EXPR_BIT_AND(a26, b26),      \
		Z_EXPR_BIT_AND(a25, b25), Z_EXPR_BIT_AND(a24, b24), Z_EXPR_BIT_AND(a23, b23),      \
		Z_EXPR_BIT_AND(a22, b22), Z_EXPR_BIT_AND(a21, b21), Z_EXPR_BIT_AND(a20, b20),      \
		Z_EXPR_BIT_AND(a19, b19), Z_EXPR_BIT_AND(a18, b18), Z_EXPR_BIT_AND(a17, b17),      \
		Z_EXPR_BIT_AND(a16, b16), Z_EXPR_BIT_AND(a15, b15), Z_EXPR_BIT_AND(a14, b14),      \
		Z_EXPR_BIT_AND(a13, b13), Z_EXPR_BIT_AND(a12, b12), Z_EXPR_BIT_AND(a11, b11),      \
		Z_EXPR_BIT_AND(a10, b10), Z_EXPR_BIT_AND(a09, b09), Z_EXPR_BIT_AND(a08, b08),      \
		Z_EXPR_BIT_AND(a07, b07), Z_EXPR_BIT_AND(a06, b06), Z_EXPR_BIT_AND(a05, b05),      \
		Z_EXPR_BIT_AND(a04, b04), Z_EXPR_BIT_AND(a03, b03), Z_EXPR_BIT_AND(a02, b02),      \
		Z_EXPR_BIT_AND(a01, b01), Z_EXPR_BIT_AND(a00, b00)

/**
 * Performs bitwise XOR on two 32-bit numbers in bit-args format.
 */
#define Z_EXPR_XOR(a31, a30, a29, a28, a27, a26, a25, a24, a23, a22, a21, a20, a19, a18, a17, a16, \
		   a15, a14, a13, a12, a11, a10, a09, a08, a07, a06, a05, a04, a03, a02, a01, a00, \
		   b31, b30, b29, b28, b27, b26, b25, b24, b23, b22, b21, b20, b19, b18, b17, b16, \
		   b15, b14, b13, b12, b11, b10, b09, b08, b07, b06, b05, b04, b03, b02, b01, b00) \
	Z_EXPR_BIT_XOR(a31, b31), Z_EXPR_BIT_XOR(a30, b30), Z_EXPR_BIT_XOR(a29, b29),              \
		Z_EXPR_BIT_XOR(a28, b28), Z_EXPR_BIT_XOR(a27, b27), Z_EXPR_BIT_XOR(a26, b26),      \
		Z_EXPR_BIT_XOR(a25, b25), Z_EXPR_BIT_XOR(a24, b24), Z_EXPR_BIT_XOR(a23, b23),      \
		Z_EXPR_BIT_XOR(a22, b22), Z_EXPR_BIT_XOR(a21, b21), Z_EXPR_BIT_XOR(a20, b20),      \
		Z_EXPR_BIT_XOR(a19, b19), Z_EXPR_BIT_XOR(a18, b18), Z_EXPR_BIT_XOR(a17, b17),      \
		Z_EXPR_BIT_XOR(a16, b16), Z_EXPR_BIT_XOR(a15, b15), Z_EXPR_BIT_XOR(a14, b14),      \
		Z_EXPR_BIT_XOR(a13, b13), Z_EXPR_BIT_XOR(a12, b12), Z_EXPR_BIT_XOR(a11, b11),      \
		Z_EXPR_BIT_XOR(a10, b10), Z_EXPR_BIT_XOR(a09, b09), Z_EXPR_BIT_XOR(a08, b08),      \
		Z_EXPR_BIT_XOR(a07, b07), Z_EXPR_BIT_XOR(a06, b06), Z_EXPR_BIT_XOR(a05, b05),      \
		Z_EXPR_BIT_XOR(a04, b04), Z_EXPR_BIT_XOR(a03, b03), Z_EXPR_BIT_XOR(a02, b02),      \
		Z_EXPR_BIT_XOR(a01, b01), Z_EXPR_BIT_XOR(a00, b00)

/**
 * Performs bitwise NOT on a 32-bit number in bit-args format.
 */
#define Z_EXPR_NOT(a31, a30, a29, a28, a27, a26, a25, a24, a23, a22, a21, a20, a19, a18, a17, a16, \
		   a15, a14, a13, a12, a11, a10, a09, a08, a07, a06, a05, a04, a03, a02, a01, a00) \
	Z_EXPR_BIT_NOT(a31), Z_EXPR_BIT_NOT(a30), Z_EXPR_BIT_NOT(a29), Z_EXPR_BIT_NOT(a28),        \
		Z_EXPR_BIT_NOT(a27), Z_EXPR_BIT_NOT(a26), Z_EXPR_BIT_NOT(a25),                     \
		Z_EXPR_BIT_NOT(a24), Z_EXPR_BIT_NOT(a23), Z_EXPR_BIT_NOT(a22),                     \
		Z_EXPR_BIT_NOT(a21), Z_EXPR_BIT_NOT(a20), Z_EXPR_BIT_NOT(a19),                     \
		Z_EXPR_BIT_NOT(a18), Z_EXPR_BIT_NOT(a17), Z_EXPR_BIT_NOT(a16),                     \
		Z_EXPR_BIT_NOT(a15), Z_EXPR_BIT_NOT(a14), Z_EXPR_BIT_NOT(a13),                     \
		Z_EXPR_BIT_NOT(a12), Z_EXPR_BIT_NOT(a11), Z_EXPR_BIT_NOT(a10),                     \
		Z_EXPR_BIT_NOT(a09), Z_EXPR_BIT_NOT(a08), Z_EXPR_BIT_NOT(a07),                     \
		Z_EXPR_BIT_NOT(a06), Z_EXPR_BIT_NOT(a05), Z_EXPR_BIT_NOT(a04),                     \
		Z_EXPR_BIT_NOT(a03), Z_EXPR_BIT_NOT(a02), Z_EXPR_BIT_NOT(a01), Z_EXPR_BIT_NOT(a00)

#define Z_EXPR_LSHIFT_N(...) GET_ARGS_LESS_N(1, __VA_ARGS__), 0

#define Z_EXPR_LSHIFT_0(...)  __VA_ARGS__
#define Z_EXPR_LSHIFT_1(...)  Z_EXPR_LSHIFT_0(Z_EXPR_LSHIFT_N(__VA_ARGS__))
#define Z_EXPR_LSHIFT_2(...)  Z_EXPR_LSHIFT_1(Z_EXPR_LSHIFT_N(__VA_ARGS__))
#define Z_EXPR_LSHIFT_3(...)  Z_EXPR_LSHIFT_2(Z_EXPR_LSHIFT_N(__VA_ARGS__))
#define Z_EXPR_LSHIFT_4(...)  Z_EXPR_LSHIFT_3(Z_EXPR_LSHIFT_N(__VA_ARGS__))
#define Z_EXPR_LSHIFT_5(...)  Z_EXPR_LSHIFT_4(Z_EXPR_LSHIFT_N(__VA_ARGS__))
#define Z_EXPR_LSHIFT_6(...)  Z_EXPR_LSHIFT_5(Z_EXPR_LSHIFT_N(__VA_ARGS__))
#define Z_EXPR_LSHIFT_7(...)  Z_EXPR_LSHIFT_6(Z_EXPR_LSHIFT_N(__VA_ARGS__))
#define Z_EXPR_LSHIFT_8(...)  Z_EXPR_LSHIFT_7(Z_EXPR_LSHIFT_N(__VA_ARGS__))
#define Z_EXPR_LSHIFT_9(...)  Z_EXPR_LSHIFT_8(Z_EXPR_LSHIFT_N(__VA_ARGS__))
#define Z_EXPR_LSHIFT_10(...) Z_EXPR_LSHIFT_9(Z_EXPR_LSHIFT_N(__VA_ARGS__))
#define Z_EXPR_LSHIFT_11(...) Z_EXPR_LSHIFT_10(Z_EXPR_LSHIFT_N(__VA_ARGS__))
#define Z_EXPR_LSHIFT_12(...) Z_EXPR_LSHIFT_11(Z_EXPR_LSHIFT_N(__VA_ARGS__))
#define Z_EXPR_LSHIFT_13(...) Z_EXPR_LSHIFT_12(Z_EXPR_LSHIFT_N(__VA_ARGS__))
#define Z_EXPR_LSHIFT_14(...) Z_EXPR_LSHIFT_13(Z_EXPR_LSHIFT_N(__VA_ARGS__))
#define Z_EXPR_LSHIFT_15(...) Z_EXPR_LSHIFT_14(Z_EXPR_LSHIFT_N(__VA_ARGS__))
#define Z_EXPR_LSHIFT_16(...) Z_EXPR_LSHIFT_15(Z_EXPR_LSHIFT_N(__VA_ARGS__))
#define Z_EXPR_LSHIFT_17(...) Z_EXPR_LSHIFT_16(Z_EXPR_LSHIFT_N(__VA_ARGS__))
#define Z_EXPR_LSHIFT_18(...) Z_EXPR_LSHIFT_17(Z_EXPR_LSHIFT_N(__VA_ARGS__))
#define Z_EXPR_LSHIFT_19(...) Z_EXPR_LSHIFT_18(Z_EXPR_LSHIFT_N(__VA_ARGS__))
#define Z_EXPR_LSHIFT_20(...) Z_EXPR_LSHIFT_19(Z_EXPR_LSHIFT_N(__VA_ARGS__))
#define Z_EXPR_LSHIFT_21(...) Z_EXPR_LSHIFT_20(Z_EXPR_LSHIFT_N(__VA_ARGS__))
#define Z_EXPR_LSHIFT_22(...) Z_EXPR_LSHIFT_21(Z_EXPR_LSHIFT_N(__VA_ARGS__))
#define Z_EXPR_LSHIFT_23(...) Z_EXPR_LSHIFT_22(Z_EXPR_LSHIFT_N(__VA_ARGS__))
#define Z_EXPR_LSHIFT_24(...) Z_EXPR_LSHIFT_23(Z_EXPR_LSHIFT_N(__VA_ARGS__))
#define Z_EXPR_LSHIFT_25(...) Z_EXPR_LSHIFT_24(Z_EXPR_LSHIFT_N(__VA_ARGS__))
#define Z_EXPR_LSHIFT_26(...) Z_EXPR_LSHIFT_25(Z_EXPR_LSHIFT_N(__VA_ARGS__))
#define Z_EXPR_LSHIFT_27(...) Z_EXPR_LSHIFT_26(Z_EXPR_LSHIFT_N(__VA_ARGS__))
#define Z_EXPR_LSHIFT_28(...) Z_EXPR_LSHIFT_27(Z_EXPR_LSHIFT_N(__VA_ARGS__))
#define Z_EXPR_LSHIFT_29(...) Z_EXPR_LSHIFT_28(Z_EXPR_LSHIFT_N(__VA_ARGS__))
#define Z_EXPR_LSHIFT_30(...) Z_EXPR_LSHIFT_29(Z_EXPR_LSHIFT_N(__VA_ARGS__))
#define Z_EXPR_LSHIFT_31(...) Z_EXPR_LSHIFT_30(Z_EXPR_LSHIFT_N(__VA_ARGS__))

/**
 * Performs a left-shift operation.
 * The first 32 arguments are the number expressed in bit-args format to be shifted.
 * The remaining 32 are the shift amounts, also expressed in bit-args.
 */
#define Z_EXPR_LSHIFT(...)                                                                         \
	COND_CODE_1(Z_EXPR_IS_TRUE_N(27, GET_ARGS_LESS_N(32, __VA_ARGS__)),                        \
		    (Z_EXPR_NUM_0),                                                                \
		    (_CONCAT_6(Z_EXPR_LSHIFT_,                                                     \
			       Z_EXPR_BIN2DEC_5(GET_ARGS_LESS_N(59 /* 64 - 5 */, __VA_ARGS__)))(   \
			       GET_ARGS_FIRST_N(32, __VA_ARGS__))))

#define Z_EXPR_RSHIFT_N(...) GET_ARGS_FIRST_N(32, 0, __VA_ARGS__)

#define Z_EXPR_RSHIFT_0(...)  __VA_ARGS__
#define Z_EXPR_RSHIFT_1(...)  Z_EXPR_RSHIFT_0(Z_EXPR_RSHIFT_N(__VA_ARGS__))
#define Z_EXPR_RSHIFT_2(...)  Z_EXPR_RSHIFT_1(Z_EXPR_RSHIFT_N(__VA_ARGS__))
#define Z_EXPR_RSHIFT_3(...)  Z_EXPR_RSHIFT_2(Z_EXPR_RSHIFT_N(__VA_ARGS__))
#define Z_EXPR_RSHIFT_4(...)  Z_EXPR_RSHIFT_3(Z_EXPR_RSHIFT_N(__VA_ARGS__))
#define Z_EXPR_RSHIFT_5(...)  Z_EXPR_RSHIFT_4(Z_EXPR_RSHIFT_N(__VA_ARGS__))
#define Z_EXPR_RSHIFT_6(...)  Z_EXPR_RSHIFT_5(Z_EXPR_RSHIFT_N(__VA_ARGS__))
#define Z_EXPR_RSHIFT_7(...)  Z_EXPR_RSHIFT_6(Z_EXPR_RSHIFT_N(__VA_ARGS__))
#define Z_EXPR_RSHIFT_8(...)  Z_EXPR_RSHIFT_7(Z_EXPR_RSHIFT_N(__VA_ARGS__))
#define Z_EXPR_RSHIFT_9(...)  Z_EXPR_RSHIFT_8(Z_EXPR_RSHIFT_N(__VA_ARGS__))
#define Z_EXPR_RSHIFT_10(...) Z_EXPR_RSHIFT_9(Z_EXPR_RSHIFT_N(__VA_ARGS__))
#define Z_EXPR_RSHIFT_11(...) Z_EXPR_RSHIFT_10(Z_EXPR_RSHIFT_N(__VA_ARGS__))
#define Z_EXPR_RSHIFT_12(...) Z_EXPR_RSHIFT_11(Z_EXPR_RSHIFT_N(__VA_ARGS__))
#define Z_EXPR_RSHIFT_13(...) Z_EXPR_RSHIFT_12(Z_EXPR_RSHIFT_N(__VA_ARGS__))
#define Z_EXPR_RSHIFT_14(...) Z_EXPR_RSHIFT_13(Z_EXPR_RSHIFT_N(__VA_ARGS__))
#define Z_EXPR_RSHIFT_15(...) Z_EXPR_RSHIFT_14(Z_EXPR_RSHIFT_N(__VA_ARGS__))
#define Z_EXPR_RSHIFT_16(...) Z_EXPR_RSHIFT_15(Z_EXPR_RSHIFT_N(__VA_ARGS__))
#define Z_EXPR_RSHIFT_17(...) Z_EXPR_RSHIFT_16(Z_EXPR_RSHIFT_N(__VA_ARGS__))
#define Z_EXPR_RSHIFT_18(...) Z_EXPR_RSHIFT_17(Z_EXPR_RSHIFT_N(__VA_ARGS__))
#define Z_EXPR_RSHIFT_19(...) Z_EXPR_RSHIFT_18(Z_EXPR_RSHIFT_N(__VA_ARGS__))
#define Z_EXPR_RSHIFT_20(...) Z_EXPR_RSHIFT_19(Z_EXPR_RSHIFT_N(__VA_ARGS__))
#define Z_EXPR_RSHIFT_21(...) Z_EXPR_RSHIFT_20(Z_EXPR_RSHIFT_N(__VA_ARGS__))
#define Z_EXPR_RSHIFT_22(...) Z_EXPR_RSHIFT_21(Z_EXPR_RSHIFT_N(__VA_ARGS__))
#define Z_EXPR_RSHIFT_23(...) Z_EXPR_RSHIFT_22(Z_EXPR_RSHIFT_N(__VA_ARGS__))
#define Z_EXPR_RSHIFT_24(...) Z_EXPR_RSHIFT_23(Z_EXPR_RSHIFT_N(__VA_ARGS__))
#define Z_EXPR_RSHIFT_25(...) Z_EXPR_RSHIFT_24(Z_EXPR_RSHIFT_N(__VA_ARGS__))
#define Z_EXPR_RSHIFT_26(...) Z_EXPR_RSHIFT_25(Z_EXPR_RSHIFT_N(__VA_ARGS__))
#define Z_EXPR_RSHIFT_27(...) Z_EXPR_RSHIFT_26(Z_EXPR_RSHIFT_N(__VA_ARGS__))
#define Z_EXPR_RSHIFT_28(...) Z_EXPR_RSHIFT_27(Z_EXPR_RSHIFT_N(__VA_ARGS__))
#define Z_EXPR_RSHIFT_29(...) Z_EXPR_RSHIFT_28(Z_EXPR_RSHIFT_N(__VA_ARGS__))
#define Z_EXPR_RSHIFT_30(...) Z_EXPR_RSHIFT_29(Z_EXPR_RSHIFT_N(__VA_ARGS__))
#define Z_EXPR_RSHIFT_31(...) Z_EXPR_RSHIFT_30(Z_EXPR_RSHIFT_N(__VA_ARGS__))

/**
 * Performs a right-shift operation.
 * The first 32 arguments are the number expressed in bit-args format to be shifted.
 * The remaining 32 are the shift amounts also expressed in bit-args.
 */
#define Z_EXPR_RSHIFT(...)                                                                         \
	COND_CODE_1(Z_EXPR_IS_TRUE_N(27, GET_ARGS_LESS_N(32, __VA_ARGS__)),                        \
		    (Z_EXPR_NUM_0),                                                                \
		    (_CONCAT_6(Z_EXPR_RSHIFT_,                                                     \
			       Z_EXPR_BIN2DEC_5(GET_ARGS_LESS_N(59 /* 64 - 5 */, __VA_ARGS__)))(   \
			       GET_ARGS_FIRST_N(32, __VA_ARGS__))))

#define Z_EXPR_CMP_RESULT_EQ(...) Z_GET_ARG_1(__VA_ARGS__)
#define Z_EXPR_CMP_RESULT_LT(...) Z_GET_ARG_2(__VA_ARGS__)
#define Z_EXPR_CMP_RESULT_GT(...) Z_GET_ARG_3(__VA_ARGS__)

#define Z_EXPR_CMP_EQ(x, y) Z_EXPR_BIT_NOT(Z_EXPR_BIT_XOR(x, y))
#define Z_EXPR_CMP_GT(x, y) Z_EXPR_BIT_AND(x, Z_EXPR_BIT_NOT(y))
#define Z_EXPR_CMP_LT(x, y) Z_EXPR_BIT_AND(Z_EXPR_BIT_NOT(x), y)

/*
 * The following recurrence formula expresses whether bit-args are equal,
 * less than, or greater than.
 *
 * E_i = E_{i - 1} && (A_i = B_i)
 * L_i = L_{i - 1} || (E_{i - 1} &&  A_i && ~B_i)
 * G_i = G_{i - 1} || (E_{i - 1} && ~A_i &&  B_i)
 *
 * @param e E_{i-1} in the recurrence formula
 * @param l L_{i-1} in the recurrence formula
 * @param g G_{i-1} in the recurrence formula
 * @param a A_i in the recurrence formula
 * @param b B_i in the recurrence formula
 *
 * @return A bit-args, which are e, l, and g values, are replaced by E_i, L_i, and G_i,
 * and the parameters a and b are deleted.
 */
#define Z_EXPR_CMP_N(e, l, g, a, b, ...)                                                           \
	Z_EXPR_BIT_AND(e, Z_EXPR_CMP_EQ(a, b)),                                                    \
		Z_EXPR_BIT_OR(l, Z_EXPR_BIT_AND(e, Z_EXPR_CMP_LT(a, b))),                          \
		Z_EXPR_BIT_OR(g, Z_EXPR_BIT_AND(e, Z_EXPR_CMP_GT(a, b))), __VA_ARGS__

#define Z_EXPR_CMP_0(...)  Z_EXPR_CMP_N(__VA_ARGS__)
#define Z_EXPR_CMP_1(...)  Z_EXPR_CMP_0(Z_EXPR_CMP_N(__VA_ARGS__))
#define Z_EXPR_CMP_2(...)  Z_EXPR_CMP_1(Z_EXPR_CMP_N(__VA_ARGS__))
#define Z_EXPR_CMP_3(...)  Z_EXPR_CMP_2(Z_EXPR_CMP_N(__VA_ARGS__))
#define Z_EXPR_CMP_4(...)  Z_EXPR_CMP_3(Z_EXPR_CMP_N(__VA_ARGS__))
#define Z_EXPR_CMP_5(...)  Z_EXPR_CMP_4(Z_EXPR_CMP_N(__VA_ARGS__))
#define Z_EXPR_CMP_6(...)  Z_EXPR_CMP_5(Z_EXPR_CMP_N(__VA_ARGS__))
#define Z_EXPR_CMP_7(...)  Z_EXPR_CMP_6(Z_EXPR_CMP_N(__VA_ARGS__))
#define Z_EXPR_CMP_8(...)  Z_EXPR_CMP_7(Z_EXPR_CMP_N(__VA_ARGS__))
#define Z_EXPR_CMP_9(...)  Z_EXPR_CMP_8(Z_EXPR_CMP_N(__VA_ARGS__))
#define Z_EXPR_CMP_10(...) Z_EXPR_CMP_9(Z_EXPR_CMP_N(__VA_ARGS__))
#define Z_EXPR_CMP_11(...) Z_EXPR_CMP_10(Z_EXPR_CMP_N(__VA_ARGS__))
#define Z_EXPR_CMP_12(...) Z_EXPR_CMP_11(Z_EXPR_CMP_N(__VA_ARGS__))
#define Z_EXPR_CMP_13(...) Z_EXPR_CMP_12(Z_EXPR_CMP_N(__VA_ARGS__))
#define Z_EXPR_CMP_14(...) Z_EXPR_CMP_13(Z_EXPR_CMP_N(__VA_ARGS__))
#define Z_EXPR_CMP_15(...) Z_EXPR_CMP_14(Z_EXPR_CMP_N(__VA_ARGS__))
#define Z_EXPR_CMP_16(...) Z_EXPR_CMP_15(Z_EXPR_CMP_N(__VA_ARGS__))
#define Z_EXPR_CMP_17(...) Z_EXPR_CMP_16(Z_EXPR_CMP_N(__VA_ARGS__))
#define Z_EXPR_CMP_18(...) Z_EXPR_CMP_17(Z_EXPR_CMP_N(__VA_ARGS__))
#define Z_EXPR_CMP_19(...) Z_EXPR_CMP_18(Z_EXPR_CMP_N(__VA_ARGS__))
#define Z_EXPR_CMP_20(...) Z_EXPR_CMP_19(Z_EXPR_CMP_N(__VA_ARGS__))
#define Z_EXPR_CMP_21(...) Z_EXPR_CMP_20(Z_EXPR_CMP_N(__VA_ARGS__))
#define Z_EXPR_CMP_22(...) Z_EXPR_CMP_21(Z_EXPR_CMP_N(__VA_ARGS__))
#define Z_EXPR_CMP_23(...) Z_EXPR_CMP_22(Z_EXPR_CMP_N(__VA_ARGS__))
#define Z_EXPR_CMP_24(...) Z_EXPR_CMP_23(Z_EXPR_CMP_N(__VA_ARGS__))
#define Z_EXPR_CMP_25(...) Z_EXPR_CMP_24(Z_EXPR_CMP_N(__VA_ARGS__))
#define Z_EXPR_CMP_26(...) Z_EXPR_CMP_25(Z_EXPR_CMP_N(__VA_ARGS__))
#define Z_EXPR_CMP_27(...) Z_EXPR_CMP_26(Z_EXPR_CMP_N(__VA_ARGS__))
#define Z_EXPR_CMP_28(...) Z_EXPR_CMP_27(Z_EXPR_CMP_N(__VA_ARGS__))
#define Z_EXPR_CMP_29(...) Z_EXPR_CMP_28(Z_EXPR_CMP_N(__VA_ARGS__))
#define Z_EXPR_CMP_30(...) Z_EXPR_CMP_29(Z_EXPR_CMP_N(__VA_ARGS__))
#define Z_EXPR_CMP_31(...) Z_EXPR_CMP_30(Z_EXPR_CMP_N(__VA_ARGS__))

/*
 * Set the initial conditions E_0=1, L_0=0, G_0=0 and recursively operate
 * on the list of bit-pairs of arguments.
 */
#define Z_EXPR_CMP(...) Z_EXPR_CMP_31(1, 0, 0, Z_EXPR_TUPLIFY(__VA_ARGS__))

#define Z_EXPR_IS_EQ(a31, a30, a29, a28, a27, a26, a25, a24, a23, a22, a21, a20, a19, a18, a17,    \
		     a16, a15, a14, a13, a12, a11, a10, a09, a08, a07, a06, a05, a04, a03, a02,    \
		     a01, a00, b31, b30, b29, b28, b27, b26, b25, b24, b23, b22, b21, b20, b19,    \
		     b18, b17, b16, b15, b14, b13, b12, b11, b10, b09, b08, b07, b06, b05, b04,    \
		     b03, b02, b01, b00)                                                           \
	Z_EXPR_BIT_NOT(Z_EXPR_IS_TRUE_N(                                                           \
		32, Z_EXPR_BIT_XOR(a31, b31), Z_EXPR_BIT_XOR(a30, b30), Z_EXPR_BIT_XOR(a29, b29),  \
		Z_EXPR_BIT_XOR(a28, b28), Z_EXPR_BIT_XOR(a27, b27), Z_EXPR_BIT_XOR(a26, b26),      \
		Z_EXPR_BIT_XOR(a25, b25), Z_EXPR_BIT_XOR(a24, b24), Z_EXPR_BIT_XOR(a23, b23),      \
		Z_EXPR_BIT_XOR(a22, b22), Z_EXPR_BIT_XOR(a21, b21), Z_EXPR_BIT_XOR(a20, b20),      \
		Z_EXPR_BIT_XOR(a19, b19), Z_EXPR_BIT_XOR(a18, b18), Z_EXPR_BIT_XOR(a17, b17),      \
		Z_EXPR_BIT_XOR(a16, b16), Z_EXPR_BIT_XOR(a15, b15), Z_EXPR_BIT_XOR(a14, b14),      \
		Z_EXPR_BIT_XOR(a13, b13), Z_EXPR_BIT_XOR(a12, b12), Z_EXPR_BIT_XOR(a11, b11),      \
		Z_EXPR_BIT_XOR(a10, b10), Z_EXPR_BIT_XOR(a09, b09), Z_EXPR_BIT_XOR(a08, b08),      \
		Z_EXPR_BIT_XOR(a07, b07), Z_EXPR_BIT_XOR(a06, b06), Z_EXPR_BIT_XOR(a05, b05),      \
		Z_EXPR_BIT_XOR(a04, b04), Z_EXPR_BIT_XOR(a03, b03), Z_EXPR_BIT_XOR(a02, b02),      \
		Z_EXPR_BIT_XOR(a01, b01), Z_EXPR_BIT_XOR(a00, b00)))

#define Z_EXPR_IS_LT(...)  Z_EXPR_CMP_RESULT_LT(Z_EXPR_CMP(__VA_ARGS__))
#define Z_EXPR_IS_GT(...)  Z_EXPR_CMP_RESULT_GT(Z_EXPR_CMP(__VA_ARGS__))
#define Z_EXPR_IS_LTE(...) Z_EXPR_BIT_NOT(Z_EXPR_CMP_RESULT_GT(Z_EXPR_CMP(__VA_ARGS__)))
#define Z_EXPR_IS_GTE(...) Z_EXPR_BIT_NOT(Z_EXPR_CMP_RESULT_LT(Z_EXPR_CMP(__VA_ARGS__)))

#define Z_EXPR_EQ(...)  COND_CODE_1(Z_EXPR_IS_EQ(__VA_ARGS__), (Z_EXPR_NUM_1), (Z_EXPR_NUM_0))
#define Z_EXPR_LT(...)  COND_CODE_1(Z_EXPR_IS_LT(__VA_ARGS__), (Z_EXPR_NUM_1), (Z_EXPR_NUM_0))
#define Z_EXPR_GT(...)  COND_CODE_1(Z_EXPR_IS_GT(__VA_ARGS__), (Z_EXPR_NUM_1), (Z_EXPR_NUM_0))
#define Z_EXPR_LTE(...) COND_CODE_1(Z_EXPR_IS_LTE(__VA_ARGS__), (Z_EXPR_NUM_1), (Z_EXPR_NUM_0))
#define Z_EXPR_GTE(...) COND_CODE_1(Z_EXPR_IS_GTE(__VA_ARGS__), (Z_EXPR_NUM_1), (Z_EXPR_NUM_0))

#define Z_EXPR_ADD_SUM_N(x, y, c) Z_EXPR_BIT_XOR(Z_EXPR_BIT_XOR(x, y), c)
#define Z_EXPR_ADD_CARRY_OUT_N(x, y, c)                                                            \
	Z_EXPR_BIT_OR(Z_EXPR_BIT_AND(x, y), Z_EXPR_BIT_AND(Z_EXPR_BIT_XOR(x, y), c))

/**
 * Addition is expressed by the following recurrence formula:
 *
 * S_n = A_n XOR B_n XOR C_{n-1}
 * C_n = (A_n AND B_n) OR ((A_n XOR B_n) AND C_{n-1})
 *
 * This macro expects to be expanded from the end of the list.
 * That is, if there is (3, 2, 1, 0), it is expected to be expanded in the order (3, (2, 1, 0)).
 *
 * @param x First input bit.
 * @param y Second input bit.
 * @param ... The first bit is the carry bit, and the rest is the calculated value.
 * @return A list; the first is the carry bit, and the rest are the already calculated values
 */
#define Z_EXPR_ADD_N(x, y, ...)                                                                    \
	Z_EXPR_ADD_CARRY_OUT_N(x, y, GET_ARGS_FIRST_N(1, __VA_ARGS__)),                            \
		Z_EXPR_ADD_SUM_N(x, y, GET_ARGS_FIRST_N(1, __VA_ARGS__)),                          \
		Z_GET_ARGS_LESS_1(__VA_ARGS__)

#define Z_EXPR_ADD_0(x, y, ...)  Z_EXPR_ADD_N(x, y, __VA_ARGS__)
#define Z_EXPR_ADD_1(x, y, ...)  Z_EXPR_ADD_N(x, y, Z_EXPR_ADD_0(__VA_ARGS__))
#define Z_EXPR_ADD_2(x, y, ...)  Z_EXPR_ADD_N(x, y, Z_EXPR_ADD_1(__VA_ARGS__))
#define Z_EXPR_ADD_3(x, y, ...)  Z_EXPR_ADD_N(x, y, Z_EXPR_ADD_2(__VA_ARGS__))
#define Z_EXPR_ADD_4(x, y, ...)  Z_EXPR_ADD_N(x, y, Z_EXPR_ADD_3(__VA_ARGS__))
#define Z_EXPR_ADD_5(x, y, ...)  Z_EXPR_ADD_N(x, y, Z_EXPR_ADD_4(__VA_ARGS__))
#define Z_EXPR_ADD_6(x, y, ...)  Z_EXPR_ADD_N(x, y, Z_EXPR_ADD_5(__VA_ARGS__))
#define Z_EXPR_ADD_7(x, y, ...)  Z_EXPR_ADD_N(x, y, Z_EXPR_ADD_6(__VA_ARGS__))
#define Z_EXPR_ADD_8(x, y, ...)  Z_EXPR_ADD_N(x, y, Z_EXPR_ADD_7(__VA_ARGS__))
#define Z_EXPR_ADD_9(x, y, ...)  Z_EXPR_ADD_N(x, y, Z_EXPR_ADD_8(__VA_ARGS__))
#define Z_EXPR_ADD_10(x, y, ...) Z_EXPR_ADD_N(x, y, Z_EXPR_ADD_9(__VA_ARGS__))
#define Z_EXPR_ADD_11(x, y, ...) Z_EXPR_ADD_N(x, y, Z_EXPR_ADD_10(__VA_ARGS__))
#define Z_EXPR_ADD_12(x, y, ...) Z_EXPR_ADD_N(x, y, Z_EXPR_ADD_11(__VA_ARGS__))
#define Z_EXPR_ADD_13(x, y, ...) Z_EXPR_ADD_N(x, y, Z_EXPR_ADD_12(__VA_ARGS__))
#define Z_EXPR_ADD_14(x, y, ...) Z_EXPR_ADD_N(x, y, Z_EXPR_ADD_13(__VA_ARGS__))
#define Z_EXPR_ADD_15(x, y, ...) Z_EXPR_ADD_N(x, y, Z_EXPR_ADD_14(__VA_ARGS__))
#define Z_EXPR_ADD_16(x, y, ...) Z_EXPR_ADD_N(x, y, Z_EXPR_ADD_15(__VA_ARGS__))
#define Z_EXPR_ADD_17(x, y, ...) Z_EXPR_ADD_N(x, y, Z_EXPR_ADD_16(__VA_ARGS__))
#define Z_EXPR_ADD_18(x, y, ...) Z_EXPR_ADD_N(x, y, Z_EXPR_ADD_17(__VA_ARGS__))
#define Z_EXPR_ADD_19(x, y, ...) Z_EXPR_ADD_N(x, y, Z_EXPR_ADD_18(__VA_ARGS__))
#define Z_EXPR_ADD_20(x, y, ...) Z_EXPR_ADD_N(x, y, Z_EXPR_ADD_19(__VA_ARGS__))
#define Z_EXPR_ADD_21(x, y, ...) Z_EXPR_ADD_N(x, y, Z_EXPR_ADD_20(__VA_ARGS__))
#define Z_EXPR_ADD_22(x, y, ...) Z_EXPR_ADD_N(x, y, Z_EXPR_ADD_21(__VA_ARGS__))
#define Z_EXPR_ADD_23(x, y, ...) Z_EXPR_ADD_N(x, y, Z_EXPR_ADD_22(__VA_ARGS__))
#define Z_EXPR_ADD_24(x, y, ...) Z_EXPR_ADD_N(x, y, Z_EXPR_ADD_23(__VA_ARGS__))
#define Z_EXPR_ADD_25(x, y, ...) Z_EXPR_ADD_N(x, y, Z_EXPR_ADD_24(__VA_ARGS__))
#define Z_EXPR_ADD_26(x, y, ...) Z_EXPR_ADD_N(x, y, Z_EXPR_ADD_25(__VA_ARGS__))
#define Z_EXPR_ADD_27(x, y, ...) Z_EXPR_ADD_N(x, y, Z_EXPR_ADD_26(__VA_ARGS__))
#define Z_EXPR_ADD_28(x, y, ...) Z_EXPR_ADD_N(x, y, Z_EXPR_ADD_27(__VA_ARGS__))
#define Z_EXPR_ADD_29(x, y, ...) Z_EXPR_ADD_N(x, y, Z_EXPR_ADD_28(__VA_ARGS__))
#define Z_EXPR_ADD_30(x, y, ...) Z_EXPR_ADD_N(x, y, Z_EXPR_ADD_29(__VA_ARGS__))
#define Z_EXPR_ADD_31(x, y, ...) Z_EXPR_ADD_N(x, y, Z_EXPR_ADD_30(__VA_ARGS__))

#define Z_EXPR_ADD_X(...) GET_ARGS_FIRST_N(32, GET_ARGS_LESS_N(1, Z_EXPR_ADD_31(__VA_ARGS__)))

/**
 * Adds two 32-bit numbers in bit-args format.
 *
 * Set the initial conditions C_0=0 and recursively operate
 * on the list of bit-pairs of arguments.
 */
#define Z_EXPR_ADD(...) Z_EXPR_ADD_X(Z_EXPR_TUPLIFY(__VA_ARGS__), 0)

/**
 * Subtracts two 32-bit numbers in bit-args format.
 *
 * Subtraction is performed by calculating the two's complement and then adding the results.
 */
#define Z_EXPR_SUB(a31, a30, a29, a28, a27, a26, a25, a24, a23, a22, a21, a20, a19, a18, a17, a16, \
		   a15, a14, a13, a12, a11, a10, a09, a08, a07, a06, a05, a04, a03, a02, a01, a00, \
		   ...)                                                                            \
	Z_EXPR_ADD(a31, a30, a29, a28, a27, a26, a25, a24, a23, a22, a21, a20, a19, a18, a17, a16, \
		   a15, a14, a13, a12, a11, a10, a09, a08, a07, a06, a05, a04, a03, a02, a01, a00, \
		   Z_EXPR_ADD(Z_EXPR_NOT(__VA_ARGS__), Z_EXPR_NUM_1))

/**
 * Calculates the partial product of a 32-bit number and a single bit.
 *
 * This macro generates a partial product for a multiplication operation by
 * performing a bitwise AND operation between each bit of a 32-bit number and
 * a single-bit multiplier.
 *
 * @param a31,...,a00 32-bit number to multiply.
 * @param b Single-bit multiplier.
 * @return 32-bit partial product as bit-args.
 */
#define Z_EXPR_MUL_PARTIAL_PRODUCT(a31, a30, a29, a28, a27, a26, a25, a24, a23, a22, a21, a20,     \
				   a19, a18, a17, a16, a15, a14, a13, a12, a11, a10, a09, a08,     \
				   a07, a06, a05, a04, a03, a02, a01, a00, b)                      \
	Z_EXPR_BIT_AND(a31, b), Z_EXPR_BIT_AND(a30, b), Z_EXPR_BIT_AND(a29, b),                    \
		Z_EXPR_BIT_AND(a28, b), Z_EXPR_BIT_AND(a27, b), Z_EXPR_BIT_AND(a26, b),            \
		Z_EXPR_BIT_AND(a25, b), Z_EXPR_BIT_AND(a24, b), Z_EXPR_BIT_AND(a23, b),            \
		Z_EXPR_BIT_AND(a22, b), Z_EXPR_BIT_AND(a21, b), Z_EXPR_BIT_AND(a20, b),            \
		Z_EXPR_BIT_AND(a19, b), Z_EXPR_BIT_AND(a18, b), Z_EXPR_BIT_AND(a17, b),            \
		Z_EXPR_BIT_AND(a16, b), Z_EXPR_BIT_AND(a15, b), Z_EXPR_BIT_AND(a14, b),            \
		Z_EXPR_BIT_AND(a13, b), Z_EXPR_BIT_AND(a12, b), Z_EXPR_BIT_AND(a11, b),            \
		Z_EXPR_BIT_AND(a10, b), Z_EXPR_BIT_AND(a09, b), Z_EXPR_BIT_AND(a08, b),            \
		Z_EXPR_BIT_AND(a07, b), Z_EXPR_BIT_AND(a06, b), Z_EXPR_BIT_AND(a05, b),            \
		Z_EXPR_BIT_AND(a04, b), Z_EXPR_BIT_AND(a03, b), Z_EXPR_BIT_AND(a02, b),            \
		Z_EXPR_BIT_AND(a01, b), Z_EXPR_BIT_AND(a00, b)

/**
 * @brief Computes the product of two 32-bit numbers through iterative accumulation of partial
 * products.
 *
 * This macro represents one iteration in the multiplication process, where the product is updated
 * by adding the shifted partial product of the multiplicand and a bit from the multiplier. This
 * approach mimics the traditional "shift-and-add" multiplication algorithm.
 *
 * @param n The current bit position being evaluated (from 31 to 0).
 * @param p31,...,p00 The current accumulated product in bit-args from previous steps.
 * @param a31,...,a00 Bit-args of the multiplicand.
 * @param b31,...,b00 Bit-args of the multiplier.
 * @return A tuple containing:
 *         - Decremented counter (`n - 1`) for the next iteration.
 *         - Updated accumulated product bit-args after adding the shifted partial product.
 *         - Bits of the multiplicand and multiplier remain unchanged for subsequent steps.
 */
#define Z_EXPR_MUL_N(n, p31, p30, p29, p28, p27, p26, p25, p24, p23, p22, p21, p20, p19, p18, p17, \
		     p16, p15, p14, p13, p12, p11, p10, p09, p08, p07, p06, p05, p04, p03, p02,    \
		     p01, p00, a31, a30, a29, a28, a27, a26, a25, a24, a23, a22, a21, a20, a19,    \
		     a18, a17, a16, a15, a14, a13, a12, a11, a10, a09, a08, a07, a06, a05, a04,    \
		     a03, a02, a01, a00, b31, b30, b29, b28, b27, b26, b25, b24, b23, b22, b21,    \
		     b20, b19, b18, b17, b16, b15, b14, b13, b12, b11, b10, b09, b08, b07, b06,    \
		     b05, b04, b03, b02, b01, b00)                                                 \
	/* Decrement the counter (move to the next bit position). */                               \
	Z_UTIL_DEC(n), /* Update the accumulated product by adding the shifted partial product. */ \
		Z_EXPR_ADD(p31, p30, p29, p28, p27, p26, p25, p24, p23, p22, p21, p20, p19, p18,   \
			   p17, p16, p15, p14, p13, p12, p11, p10, p09, p08, p07, p06, p05, p04,   \
			   p03, p02, p01, p00,                                                     \
			   UTIL_CAT(Z_EXPR_LSHIFT_, n)(Z_EXPR_MUL_PARTIAL_PRODUCT(                 \
				   a31, a30, a29, a28, a27, a26, a25, a24, a23, a22, a21, a20,     \
				   a19, a18, a17, a16, a15, a14, a13, a12, a11, a10, a09, a08,     \
				   a07, a06, a05, a04, a03, a02, a01, a00,                         \
				   UTIL_CAT(Z_GET_ARG_, UTIL_CAT(Z_UTIL_INC_, n))(                 \
					   b00, b01, b02, b03, b04, b05, b06, b07, b08, b09, b10,  \
					   b11, b12, b13, b14, b15, b16, b17, b18, b19, b20, b21,  \
					   b22, b23, b24, b25, b26, b27, b28, b29, b30,            \
					   b31)))), /* Retain the multiplicand and multiplier for  \
						       the next iteration. */                      \
		a31, a30, a29, a28, a27, a26, a25, a24, a23, a22, a21, a20, a19, a18, a17, a16,    \
		a15, a14, a13, a12, a11, a10, a09, a08, a07, a06, a05, a04, a03, a02, a01, a00,    \
		b31, b30, b29, b28, b27, b26, b25, b24, b23, b22, b21, b20, b19, b18, b17, b16,    \
		b15, b14, b13, b12, b11, b10, b09, b08, b07, b06, b05, b04, b03, b02, b01, b00

#define Z_EXPR_MUL_0(...)  Z_EXPR_MUL_N(__VA_ARGS__)
#define Z_EXPR_MUL_1(...)  Z_EXPR_MUL_0(Z_EXPR_MUL_N(__VA_ARGS__))
#define Z_EXPR_MUL_2(...)  Z_EXPR_MUL_1(Z_EXPR_MUL_N(__VA_ARGS__))
#define Z_EXPR_MUL_3(...)  Z_EXPR_MUL_2(Z_EXPR_MUL_N(__VA_ARGS__))
#define Z_EXPR_MUL_4(...)  Z_EXPR_MUL_3(Z_EXPR_MUL_N(__VA_ARGS__))
#define Z_EXPR_MUL_5(...)  Z_EXPR_MUL_4(Z_EXPR_MUL_N(__VA_ARGS__))
#define Z_EXPR_MUL_6(...)  Z_EXPR_MUL_5(Z_EXPR_MUL_N(__VA_ARGS__))
#define Z_EXPR_MUL_7(...)  Z_EXPR_MUL_6(Z_EXPR_MUL_N(__VA_ARGS__))
#define Z_EXPR_MUL_8(...)  Z_EXPR_MUL_7(Z_EXPR_MUL_N(__VA_ARGS__))
#define Z_EXPR_MUL_9(...)  Z_EXPR_MUL_8(Z_EXPR_MUL_N(__VA_ARGS__))
#define Z_EXPR_MUL_10(...) Z_EXPR_MUL_9(Z_EXPR_MUL_N(__VA_ARGS__))
#define Z_EXPR_MUL_11(...) Z_EXPR_MUL_10(Z_EXPR_MUL_N(__VA_ARGS__))
#define Z_EXPR_MUL_12(...) Z_EXPR_MUL_11(Z_EXPR_MUL_N(__VA_ARGS__))
#define Z_EXPR_MUL_13(...) Z_EXPR_MUL_12(Z_EXPR_MUL_N(__VA_ARGS__))
#define Z_EXPR_MUL_14(...) Z_EXPR_MUL_13(Z_EXPR_MUL_N(__VA_ARGS__))
#define Z_EXPR_MUL_15(...) Z_EXPR_MUL_14(Z_EXPR_MUL_N(__VA_ARGS__))
#define Z_EXPR_MUL_16(...) Z_EXPR_MUL_15(Z_EXPR_MUL_N(__VA_ARGS__))
#define Z_EXPR_MUL_17(...) Z_EXPR_MUL_16(Z_EXPR_MUL_N(__VA_ARGS__))
#define Z_EXPR_MUL_18(...) Z_EXPR_MUL_17(Z_EXPR_MUL_N(__VA_ARGS__))
#define Z_EXPR_MUL_19(...) Z_EXPR_MUL_18(Z_EXPR_MUL_N(__VA_ARGS__))
#define Z_EXPR_MUL_20(...) Z_EXPR_MUL_19(Z_EXPR_MUL_N(__VA_ARGS__))
#define Z_EXPR_MUL_21(...) Z_EXPR_MUL_20(Z_EXPR_MUL_N(__VA_ARGS__))
#define Z_EXPR_MUL_22(...) Z_EXPR_MUL_21(Z_EXPR_MUL_N(__VA_ARGS__))
#define Z_EXPR_MUL_23(...) Z_EXPR_MUL_22(Z_EXPR_MUL_N(__VA_ARGS__))
#define Z_EXPR_MUL_24(...) Z_EXPR_MUL_23(Z_EXPR_MUL_N(__VA_ARGS__))
#define Z_EXPR_MUL_25(...) Z_EXPR_MUL_24(Z_EXPR_MUL_N(__VA_ARGS__))
#define Z_EXPR_MUL_26(...) Z_EXPR_MUL_25(Z_EXPR_MUL_N(__VA_ARGS__))
#define Z_EXPR_MUL_27(...) Z_EXPR_MUL_26(Z_EXPR_MUL_N(__VA_ARGS__))
#define Z_EXPR_MUL_28(...) Z_EXPR_MUL_27(Z_EXPR_MUL_N(__VA_ARGS__))
#define Z_EXPR_MUL_29(...) Z_EXPR_MUL_28(Z_EXPR_MUL_N(__VA_ARGS__))
#define Z_EXPR_MUL_30(...) Z_EXPR_MUL_29(Z_EXPR_MUL_N(__VA_ARGS__))
#define Z_EXPR_MUL_31(...) Z_EXPR_MUL_30(Z_EXPR_MUL_N(__VA_ARGS__))

/**
 * Multiplies two 32-bit numbers in bit args format.
 */
#define Z_EXPR_MUL(...)                                                                            \
	GET_ARGS_FIRST_N(32, GET_ARGS_LESS_N(1, Z_EXPR_MUL_31(31, Z_EXPR_NUM_0, __VA_ARGS__)))

#define Z_EXPR_GET_ARGS_LESS_X(...) GET_ARGS_LESS_N(__VA_ARGS__)

#define Z_EXPR_SET_BIT_REVERSED(n, ...)                                                            \
	REVERSE_ARGS(GET_ARGS_FIRST_N(                                                             \
		32, COND_CODE_1(Z_EXPR_IS_ZERO(n),                                                 \
				(1, Z_EXPR_GET_ARGS_LESS_X(1, __VA_ARGS__)),                       \
				(GET_ARGS_FIRST_N(n, __VA_ARGS__), 1,                              \
				 Z_EXPR_GET_ARGS_LESS_X(Z_UTIL_INC(n), __VA_ARGS__)))))

#define Z_EXPR_SUB_EXPAND(...) Z_EXPR_SUB(__VA_ARGS__)

/**
 * @brief Updates the quotient and the remainder during a single step of long division
 * for 32-bit numbers.
 *
 * This macro represents an iteration of the long division process. It evaluates
 * whether the current partial remainder is greater than or equal to the shifted divisor.
 * Based on this comparison, it conditionally updates the quotient by setting the
 * appropriate bit and adjusts the remainder by subtracting the divisor if necessary.
 *
 * @param n The current bit position being evaluated (from 31 to 0).
 * @param q31,...,q00 The current bit-args of the quotient, arranged in reverse order.
 * @param ... The first 32 arguments represent the current remainder bit-args, the next
 *            32 arguments represent the left-shifted divisor, and the remaining
 *            arguments represent the divisor bit-args.
 * @return A tuple containing:
 *         - Updated counter (`n - 1`).
 *         - Updated quotient bit-args (if the condition is met).
 *         - Updated remainder bit-args (if the condition is met).
 *         - Original or updated divisor bit-args, based on the current iteration.
 */
#define Z_EXPR_DIV_UPDATE_X_(n, q31, q30, q29, q28, q27, q26, q25, q24, q23, q22, q21, q20, q19,   \
			     q18, q17, q16, q15, q14, q13, q12, q11, q10, q09, q08, q07, q06, q05, \
			     q04, q03, q02, q01, q00, ...)                                         \
	/* Decrement the counter (move to the next bit position). */                               \
	Z_UTIL_DEC(n),                                                                             \
	COND_CODE_1(Z_EXPR_IS_GTE(GET_ARGS_FIRST_N(64, __VA_ARGS__)),                              \
	/* Update the quotient and the remainder if the current digit is 1. */                     \
		   /* Set the corresponding bit in the quotient. */                                \
		   (Z_EXPR_SET_BIT_REVERSED(n, q00, q01, q02, q03, q04, q05, q06, q07, q08, q09,   \
					       q10, q11, q12, q13, q14, q15, q16, q17, q18, q19,   \
					       q20, q21, q22, q23, q24, q25, q26, q27, q28, q29,   \
					       q30, q31),                                          \
		    /* Subtract the divisor from the remainder. */                                 \
		    Z_EXPR_SUB_EXPAND(GET_ARGS_FIRST_N(64, __VA_ARGS__)),                          \
		    /* Keep the original divisor bit-args. */                                      \
		    GET_ARGS_LESS_N(64, __VA_ARGS__)),                                             \
		    /* Retain the current quotient and remainder if the condition is unmet. */     \
		   (q31, q30, q29, q28, q27, q26, q25, q24, q23, q22, q21, q20, q19, q18, q17,     \
		    q16, q15, q14, q13, q12, q11, q10, q09, q08, q07, q06, q05, q04, q03, q02,     \
		    q01, q00,                                                                      \
		    /* Retain the original remainder and divisor. */                               \
		    GET_ARGS_FIRST_N(32, __VA_ARGS__), GET_ARGS_LESS_N(64, __VA_ARGS__)))

#define Z_EXPR_DIV_UPDATE_X(...) Z_EXPR_DIV_UPDATE_X_(__VA_ARGS__)

#define Z_EXPR_DIVMOD_N(n, q31, q30, q29, q28, q27, q26, q25, q24, q23, q22, q21, q20, q19, q18,   \
			q17, q16, q15, q14, q13, q12, q11, q10, q09, q08, q07, q06, q05, q04, q03, \
			q02, q01, q00, r31, r30, r29, r28, r27, r26, r25, r24, r23, r22, r21, r20, \
			r19, r18, r17, r16, r15, r14, r13, r12, r11, r10, r09, r08, r07, r06, r05, \
			r04, r03, r02, r01, r00, ...)                                              \
	Z_EXPR_DIV_UPDATE_X(n, q31, q30, q29, q28, q27, q26, q25, q24, q23, q22, q21, q20, q19,    \
			    q18, q17, q16, q15, q14, q13, q12, q11, q10, q09, q08, q07, q06, q05,  \
			    q04, q03, q02, q01, q00, r31, r30, r29, r28, r27, r26, r25, r24, r23,  \
			    r22, r21, r20, r19, r18, r17, r16, r15, r14, r13, r12, r11, r10, r09,  \
			    r08, r07, r06, r05, r04, r03, r02, r01, r00,                           \
			    UTIL_CAT(Z_EXPR_LSHIFT_, n)(__VA_ARGS__), __VA_ARGS__)

#define Z_EXPR_DIVMOD_0(...)  Z_EXPR_DIVMOD_N(__VA_ARGS__)
#define Z_EXPR_DIVMOD_1(...)  Z_EXPR_DIVMOD_0(Z_EXPR_DIVMOD_N(__VA_ARGS__))
#define Z_EXPR_DIVMOD_2(...)  Z_EXPR_DIVMOD_1(Z_EXPR_DIVMOD_N(__VA_ARGS__))
#define Z_EXPR_DIVMOD_3(...)  Z_EXPR_DIVMOD_2(Z_EXPR_DIVMOD_N(__VA_ARGS__))
#define Z_EXPR_DIVMOD_4(...)  Z_EXPR_DIVMOD_3(Z_EXPR_DIVMOD_N(__VA_ARGS__))
#define Z_EXPR_DIVMOD_5(...)  Z_EXPR_DIVMOD_4(Z_EXPR_DIVMOD_N(__VA_ARGS__))
#define Z_EXPR_DIVMOD_6(...)  Z_EXPR_DIVMOD_5(Z_EXPR_DIVMOD_N(__VA_ARGS__))
#define Z_EXPR_DIVMOD_7(...)  Z_EXPR_DIVMOD_6(Z_EXPR_DIVMOD_N(__VA_ARGS__))
#define Z_EXPR_DIVMOD_8(...)  Z_EXPR_DIVMOD_7(Z_EXPR_DIVMOD_N(__VA_ARGS__))
#define Z_EXPR_DIVMOD_9(...)  Z_EXPR_DIVMOD_8(Z_EXPR_DIVMOD_N(__VA_ARGS__))
#define Z_EXPR_DIVMOD_10(...) Z_EXPR_DIVMOD_9(Z_EXPR_DIVMOD_N(__VA_ARGS__))
#define Z_EXPR_DIVMOD_11(...) Z_EXPR_DIVMOD_10(Z_EXPR_DIVMOD_N(__VA_ARGS__))
#define Z_EXPR_DIVMOD_12(...) Z_EXPR_DIVMOD_11(Z_EXPR_DIVMOD_N(__VA_ARGS__))
#define Z_EXPR_DIVMOD_13(...) Z_EXPR_DIVMOD_12(Z_EXPR_DIVMOD_N(__VA_ARGS__))
#define Z_EXPR_DIVMOD_14(...) Z_EXPR_DIVMOD_13(Z_EXPR_DIVMOD_N(__VA_ARGS__))
#define Z_EXPR_DIVMOD_15(...) Z_EXPR_DIVMOD_14(Z_EXPR_DIVMOD_N(__VA_ARGS__))
#define Z_EXPR_DIVMOD_16(...) Z_EXPR_DIVMOD_15(Z_EXPR_DIVMOD_N(__VA_ARGS__))
#define Z_EXPR_DIVMOD_17(...) Z_EXPR_DIVMOD_16(Z_EXPR_DIVMOD_N(__VA_ARGS__))
#define Z_EXPR_DIVMOD_18(...) Z_EXPR_DIVMOD_17(Z_EXPR_DIVMOD_N(__VA_ARGS__))
#define Z_EXPR_DIVMOD_19(...) Z_EXPR_DIVMOD_18(Z_EXPR_DIVMOD_N(__VA_ARGS__))
#define Z_EXPR_DIVMOD_20(...) Z_EXPR_DIVMOD_19(Z_EXPR_DIVMOD_N(__VA_ARGS__))
#define Z_EXPR_DIVMOD_21(...) Z_EXPR_DIVMOD_20(Z_EXPR_DIVMOD_N(__VA_ARGS__))
#define Z_EXPR_DIVMOD_22(...) Z_EXPR_DIVMOD_21(Z_EXPR_DIVMOD_N(__VA_ARGS__))
#define Z_EXPR_DIVMOD_23(...) Z_EXPR_DIVMOD_22(Z_EXPR_DIVMOD_N(__VA_ARGS__))
#define Z_EXPR_DIVMOD_24(...) Z_EXPR_DIVMOD_23(Z_EXPR_DIVMOD_N(__VA_ARGS__))
#define Z_EXPR_DIVMOD_25(...) Z_EXPR_DIVMOD_24(Z_EXPR_DIVMOD_N(__VA_ARGS__))
#define Z_EXPR_DIVMOD_26(...) Z_EXPR_DIVMOD_25(Z_EXPR_DIVMOD_N(__VA_ARGS__))
#define Z_EXPR_DIVMOD_27(...) Z_EXPR_DIVMOD_26(Z_EXPR_DIVMOD_N(__VA_ARGS__))
#define Z_EXPR_DIVMOD_28(...) Z_EXPR_DIVMOD_27(Z_EXPR_DIVMOD_N(__VA_ARGS__))
#define Z_EXPR_DIVMOD_29(...) Z_EXPR_DIVMOD_28(Z_EXPR_DIVMOD_N(__VA_ARGS__))
#define Z_EXPR_DIVMOD_30(...) Z_EXPR_DIVMOD_29(Z_EXPR_DIVMOD_N(__VA_ARGS__))
#define Z_EXPR_DIVMOD_31(...) Z_EXPR_DIVMOD_30(Z_EXPR_DIVMOD_N(__VA_ARGS__))

#define Z_EXPR_DIVMOD_START_DIGIT(...)                                                             \
	UTIL_SUB(GET_ARG_N(1, Z_EXPR_TRIM_LZERO(32, GET_ARGS_FIRST_N(32, __VA_ARGS__))),           \
		 GET_ARG_N(1, Z_EXPR_TRIM_LZERO(32, GET_ARGS_LESS_N(32, __VA_ARGS__))))

#define Z_EXPR_DIVMOD_X(n, ...) UTIL_CAT(Z_EXPR_DIVMOD_, n)(n, Z_EXPR_NUM_0, __VA_ARGS__)

#define Z_EXPR_DIVMOD(...)                                                                         \
	/* Take quotient-remainder part */                                                         \
	GET_ARGS_FIRST_N(                                                                          \
		64, /* Drop counter element at the first of list */                                \
		GET_ARGS_LESS_N(                                                                   \
			1, Z_EXPR_DIVMOD_X(Z_EXPR_DIVMOD_START_DIGIT(__VA_ARGS__), __VA_ARGS__)))
/**
 * Divides two 32-bit numbers in bit args format.
 */
#define Z_EXPR_DIV(...) GET_ARGS_FIRST_N(32, Z_EXPR_DIVMOD(__VA_ARGS__))

/**
 * Computes the remainder of the division between two 32-bit numbers in bit args format.
 */
#define Z_EXPR_MOD(...) GET_ARGS_LESS_N(32, Z_EXPR_DIVMOD(__VA_ARGS__))

#ifdef __cplusplus
}
#endif

#endif /*ZEPHYR_INCLUDE_SYS_UTIL_INTERNAL_UTIL_EXPR_H_*/
