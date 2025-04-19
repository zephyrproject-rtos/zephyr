/*
 * Copyright (c) 2024, Joel Hirsbrunner
 * Copyright (c) 2025, TOKITA Hiroshi
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This header defines internal macros for evaluating bitwise expressions.
 * It is used to perform operations such as bit concatenation, conversion from
 * binary to decimal, bit extraction, and bitwise arithmetic (NOT, AND, OR, XOR)
 * as well as left/right shift operations on 32-bit numbers represented as individual
 * bit arguments.
 */

#ifndef ZEPHYR_INCLUDE_SYS_UTIL_INTERNAL_EXPR_H_
#define ZEPHYR_INCLUDE_SYS_UTIL_INTERNAL_EXPR_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @cond INTERNAL_HIDDEN
 */

#include <zephyr/sys/util_macro.h>

#define Z_EXPR_ARGS_NEXT_32(...) GET_ARGS_FIRST_N(32, GET_ARGS_LESS_N(32, __VA_ARGS__))

#define Z_EXPR_IS_0_0   1
#define Z_EXPR_IS_0(x)  UTIL_CAT(Z_EXPR_IS_0_, x)
#define Z_EXPR_IS_32_32 1
#define Z_EXPR_IS_32(x) UTIL_CAT(Z_EXPR_IS_32_, x)

/*
 * Constructs a binary literal by concatenating 32 bit digits.
 * The arguments _31, _30, ..., _00 represent the individual bits.
 * For example, if provided with 0 and 1 values, it builds a binary constant.
 */
#define Z_EXPR_TO_NUM(_31, _30, _29, _28, _27, _26, _25, _24, _23, _22, _21, _20, _19, _18, _17,   \
		      _16, _15, _14, _13, _12, _11, _10, _09, _08, _07, _06, _05, _04, _03, _02,   \
		      _01, _00)                                                                    \
	0B##_31##_30##_29##_28##_27##_26##_25##_24##_23##_22##_21##_20##_19##_18##_17##_16         \
	  ##_15##_14##_13##_12##_11##_10##_09##_08##_07##_06##_05##_04##_03##_02##_01##_00

/*
 * The following macros define preset sequences of 32 bits.
 * They are used as reference bit patterns.
 */
#define Z_EXPR_BITS_10000U                                                                         \
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0,  \
		0, 0
#define Z_EXPR_BITS_100000U                                                                        \
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 0, 1, 0, 1, 0, 0, 0,  \
		0, 0
#define Z_EXPR_BITS_1000000U                                                                       \
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0,  \
		0, 0
#define Z_EXPR_BITS_10000000U                                                                      \
	0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0,  \
		0, 0
#define Z_EXPR_BITS_100000000U                                                                     \
	0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,  \
		0, 0
#define Z_EXPR_BITS_1000000000U                                                                    \
	0, 0, 1, 1, 1, 0, 1, 1, 1, 0, 0, 1, 1, 0, 1, 0, 1, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0,  \
		0, 0

/*
 * Concatenates the prefix "Z_EXPR_BITS" with the given number n.
 * For example, Z_EXPR_BITS(1) expands to Z_EXPR_BITS_1.
 */
#define Z_EXPR_STR(x) #x

#define Z_EXPR_BITS_U(n)                                                                           \
	COND_CODE_1(Z_EXPR_IS_32(NUM_VA_ARGS(_CONCAT_2(Z_EXPR_BITS_, n, U))),                      \
		    (_CONCAT_2(Z_EXPR_BITS_, n, U)), ("Not defined : Z_EXPR_BITS_" Z_EXPR_STR(n)))

#define Z_EXPR_BITS(n)                                                                             \
	COND_CODE_1(Z_EXPR_IS_32(NUM_VA_ARGS(UTIL_CAT(Z_EXPR_BITS_, n))),                          \
		    (_CONCAT_1(Z_EXPR_BITS_, n)), (Z_EXPR_BITS_U(n)))

/*
 * Converts a list of bit arguments into a decimal constant.
 * It first creates a binary literal with Z_EXPR_TO_NUM and then
 * concatenates with Z_EXPR_BIN_TO_DEC_ to obtain the decimal value.
 */
#define Z_EXPR_BITS_TO_DEC(...) UTIL_CAT(Z_EXPR_BIN_TO_DEC_, Z_EXPR_TO_NUM(__VA_ARGS__))

#define Z_EXPR_CAT_DEC_1(x, ...) COND_CODE_1(Z_EXPR_IS_0(x), (0), (x))
#define Z_EXPR_CAT_DEC_2(x, ...)                                                                   \
	COND_CODE_1(Z_EXPR_IS_0(x), (Z_EXPR_CAT_DEC_1(__VA_ARGS__)), (_CONCAT_2(x, __VA_ARGS__)))
#define Z_EXPR_CAT_DEC_3(x, ...)                                                                   \
	COND_CODE_1(Z_EXPR_IS_0(x), (Z_EXPR_CAT_DEC_2(__VA_ARGS__)), (_CONCAT_3(x, __VA_ARGS__)))
#define Z_EXPR_CAT_DEC_4(x, ...)                                                                   \
	COND_CODE_1(Z_EXPR_IS_0(x), (Z_EXPR_CAT_DEC_3(__VA_ARGS__)), (_CONCAT_4(x, __VA_ARGS__)))
#define Z_EXPR_CAT_DEC_5(x, ...)                                                                   \
	COND_CODE_1(Z_EXPR_IS_0(x), (Z_EXPR_CAT_DEC_4(__VA_ARGS__)), (_CONCAT_5(x, __VA_ARGS__)))
#define Z_EXPR_CAT_DEC_6(x, ...)                                                                   \
	COND_CODE_1(Z_EXPR_IS_0(x), (Z_EXPR_CAT_DEC_5(__VA_ARGS__)), (_CONCAT_6(x, __VA_ARGS__)))
#define Z_EXPR_CAT_DEC_7(x, ...)                                                                   \
	COND_CODE_1(Z_EXPR_IS_0(x), (Z_EXPR_CAT_DEC_6(__VA_ARGS__)), (_CONCAT_7(x, __VA_ARGS__)))
#define Z_EXPR_CAT_DEC_8(x, ...)                                                                   \
	COND_CODE_1(Z_EXPR_IS_0(x), (Z_EXPR_CAT_DEC_7(__VA_ARGS__)), (_CONCAT_8(x, __VA_ARGS__)))
#define Z_EXPR_CAT_DEC_9(x, ...)                                                                   \
	COND_CODE_1(Z_EXPR_IS_0(x), (Z_EXPR_CAT_DEC_8(__VA_ARGS__)), (_CONCAT_9(x, __VA_ARGS__)))
#define Z_EXPR_CAT_DEC_10(x, ...)                                                                  \
	COND_CODE_1(Z_EXPR_IS_0(x),                                                                \
		    (Z_EXPR_CAT_DEC_9(__VA_ARGS__)), (_CONCAT_10(x, __VA_ARGS__)))
#define Z_EXPR_CAT_DEC_11(x, ...)                                                                  \
	COND_CODE_1(Z_EXPR_IS_0(x),                                                                \
		    (Z_EXPR_CAT_DEC_10(__VA_ARGS__)), (_CONCAT_11(x, __VA_ARGS__)))
#define Z_EXPR_CAT_DEC(...) UTIL_CAT(Z_EXPR_CAT_DEC_, NUM_VA_ARGS(__VA_ARGS__))(__VA_ARGS__)

#define Z_EXPR_CALC_DEC_0(...)                                                                     \
	Z_EXPR_BITS_TO_DEC(GET_ARGS_FIRST_N(32, __VA_ARGS__)),                                     \
		Z_EXPR_BITS_TO_DEC(Z_EXPR_ARGS_NEXT_32(__VA_ARGS__))
#define Z_EXPR_CALC_DEC_1(...)                                                                     \
	Z_EXPR_BITS_TO_DEC(GET_ARGS_FIRST_N(32, __VA_ARGS__)),                                     \
		Z_EXPR_CALC_DEC_0(                                                                 \
			Z_EXPR_DIVMOD(Z_EXPR_ARGS_NEXT_32(__VA_ARGS__), Z_EXPR_BITS_10U))
#define Z_EXPR_CALC_DEC_2(...)                                                                     \
	Z_EXPR_BITS_TO_DEC(GET_ARGS_FIRST_N(32, __VA_ARGS__)),                                     \
		Z_EXPR_CALC_DEC_1(                                                                 \
			Z_EXPR_DIVMOD(Z_EXPR_ARGS_NEXT_32(__VA_ARGS__), Z_EXPR_BITS_100U))
#define Z_EXPR_CALC_DEC_3(...)                                                                     \
	Z_EXPR_BITS_TO_DEC(GET_ARGS_FIRST_N(32, __VA_ARGS__)),                                     \
		Z_EXPR_CALC_DEC_2(                                                                 \
			Z_EXPR_DIVMOD(Z_EXPR_ARGS_NEXT_32(__VA_ARGS__), Z_EXPR_BITS_1000U))
#define Z_EXPR_CALC_DEC_4(...)                                                                     \
	Z_EXPR_BITS_TO_DEC(GET_ARGS_FIRST_N(32, __VA_ARGS__)),                                     \
		Z_EXPR_CALC_DEC_3(                                                                 \
			Z_EXPR_DIVMOD(Z_EXPR_ARGS_NEXT_32(__VA_ARGS__), Z_EXPR_BITS_10000U))
#define Z_EXPR_CALC_DEC_5(...)                                                                     \
	Z_EXPR_BITS_TO_DEC(GET_ARGS_FIRST_N(32, __VA_ARGS__)),                                     \
		Z_EXPR_CALC_DEC_4(                                                                 \
			Z_EXPR_DIVMOD(Z_EXPR_ARGS_NEXT_32(__VA_ARGS__), Z_EXPR_BITS_100000U))
#define Z_EXPR_CALC_DEC_6(...)                                                                     \
	Z_EXPR_BITS_TO_DEC(GET_ARGS_FIRST_N(32, __VA_ARGS__)),                                     \
		Z_EXPR_CALC_DEC_5(                                                                 \
			Z_EXPR_DIVMOD(Z_EXPR_ARGS_NEXT_32(__VA_ARGS__), Z_EXPR_BITS_1000000U))
#define Z_EXPR_CALC_DEC_7(...)                                                                     \
	Z_EXPR_BITS_TO_DEC(GET_ARGS_FIRST_N(32, __VA_ARGS__)),                                     \
		Z_EXPR_CALC_DEC_6(                                                                 \
			Z_EXPR_DIVMOD(Z_EXPR_ARGS_NEXT_32(__VA_ARGS__), Z_EXPR_BITS_10000000U))
#define Z_EXPR_CALC_DEC_8(...)                                                                     \
	Z_EXPR_BITS_TO_DEC(GET_ARGS_FIRST_N(32, __VA_ARGS__)),                                     \
		Z_EXPR_CALC_DEC_7(                                                                 \
			Z_EXPR_DIVMOD(Z_EXPR_ARGS_NEXT_32(__VA_ARGS__), Z_EXPR_BITS_100000000U))
#define Z_EXPR_CALC_DEC_9(...)                                                                     \
	Z_EXPR_BITS_TO_DEC(GET_ARGS_FIRST_N(32, __VA_ARGS__)),                                     \
		Z_EXPR_CALC_DEC_8(                                                                 \
			Z_EXPR_DIVMOD(Z_EXPR_ARGS_NEXT_32(__VA_ARGS__), Z_EXPR_BITS_1000000000U))

#define Z_EXPR_ENCODE_DEC(...) Z_EXPR_CAT_DEC(Z_EXPR_CALC_DEC_9(EXPR_BITS_0X(0), __VA_ARGS__))

#define Z_EXPR_PAD_ZERO_7
#define Z_EXPR_PAD_ZERO_6 0, 0, 0, 0,
#define Z_EXPR_PAD_ZERO_5 0, 0, 0, 0, 0, 0, 0, 0,
#define Z_EXPR_PAD_ZERO_4 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
#define Z_EXPR_PAD_ZERO_3 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
#define Z_EXPR_PAD_ZERO_2 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
#define Z_EXPR_PAD_ZERO_1 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
#define Z_EXPR_PAD_ZERO_0                                                                          \
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

#define Z_EXPR_HEX2BITS_0 0, 0, 0, 0
#define Z_EXPR_HEX2BITS_1 0, 0, 0, 1
#define Z_EXPR_HEX2BITS_2 0, 0, 1, 0
#define Z_EXPR_HEX2BITS_3 0, 0, 1, 1
#define Z_EXPR_HEX2BITS_4 0, 1, 0, 0
#define Z_EXPR_HEX2BITS_5 0, 1, 0, 1
#define Z_EXPR_HEX2BITS_6 0, 1, 1, 0
#define Z_EXPR_HEX2BITS_7 0, 1, 1, 1
#define Z_EXPR_HEX2BITS_8 1, 0, 0, 0
#define Z_EXPR_HEX2BITS_9 1, 0, 0, 1
#define Z_EXPR_HEX2BITS_a 1, 0, 1, 0
#define Z_EXPR_HEX2BITS_A 1, 0, 1, 0
#define Z_EXPR_HEX2BITS_b 1, 0, 1, 1
#define Z_EXPR_HEX2BITS_B 1, 0, 1, 1
#define Z_EXPR_HEX2BITS_c 1, 1, 0, 0
#define Z_EXPR_HEX2BITS_C 1, 1, 0, 0
#define Z_EXPR_HEX2BITS_d 1, 1, 0, 1
#define Z_EXPR_HEX2BITS_D 1, 1, 0, 1
#define Z_EXPR_HEX2BITS_e 1, 1, 1, 0
#define Z_EXPR_HEX2BITS_E 1, 1, 1, 0
#define Z_EXPR_HEX2BITS_f 1, 1, 1, 1
#define Z_EXPR_HEX2BITS_F 1, 1, 1, 1

#define Z_EXPR_BITS_0X_0(a)             UTIL_CAT(Z_EXPR_HEX2BITS_, a)
#define Z_EXPR_BITS_0X_1(a, b)          Z_EXPR_BITS_0X_0(a), UTIL_CAT(Z_EXPR_HEX2BITS_, b)
#define Z_EXPR_BITS_0X_2(a, b, c)       Z_EXPR_BITS_0X_1(a, b), UTIL_CAT(Z_EXPR_HEX2BITS_, c)
#define Z_EXPR_BITS_0X_3(a, b, c, d)    Z_EXPR_BITS_0X_2(a, b, c), UTIL_CAT(Z_EXPR_HEX2BITS_, d)
#define Z_EXPR_BITS_0X_4(a, b, c, d, e) Z_EXPR_BITS_0X_3(a, b, c, d), UTIL_CAT(Z_EXPR_HEX2BITS_, e)
#define Z_EXPR_BITS_0X_5(a, b, c, d, e, f)                                                         \
	Z_EXPR_BITS_0X_4(a, b, c, d, e), UTIL_CAT(Z_EXPR_HEX2BITS_, f)
#define Z_EXPR_BITS_0X_6(a, b, c, d, e, f, g)                                                      \
	Z_EXPR_BITS_0X_5(a, b, c, d, e, f), UTIL_CAT(Z_EXPR_HEX2BITS_, g)
#define Z_EXPR_BITS_0X_7(a, b, c, d, e, f, g, h)                                                   \
	Z_EXPR_BITS_0X_6(a, b, c, d, e, f, g), UTIL_CAT(Z_EXPR_HEX2BITS_, h)

#define Z_EXPR_DO_BITS_0X(N, ...)                                                                  \
	UTIL_CAT(Z_EXPR_PAD_ZERO_, N) UTIL_CAT(Z_EXPR_BITS_0X_, N)(__VA_ARGS__)
#define Z_EXPR_BITS_0X(...) Z_EXPR_DO_BITS_0X(NUM_VA_ARGS_LESS_1(__VA_ARGS__), __VA_ARGS__)

#define Z_EXPR_BIT_LEN_0(H, ...)  COND_CODE_1(H, (1), ())
#define Z_EXPR_BIT_LEN_1(H, ...)  COND_CODE_1(H, (H, __VA_ARGS__), (Z_EXPR_BIT_LEN_0(__VA_ARGS__)))
#define Z_EXPR_BIT_LEN_2(H, ...)  COND_CODE_1(H, (H, __VA_ARGS__), (Z_EXPR_BIT_LEN_1(__VA_ARGS__)))
#define Z_EXPR_BIT_LEN_3(H, ...)  COND_CODE_1(H, (H, __VA_ARGS__), (Z_EXPR_BIT_LEN_2(__VA_ARGS__)))
#define Z_EXPR_BIT_LEN_4(H, ...)  COND_CODE_1(H, (H, __VA_ARGS__), (Z_EXPR_BIT_LEN_3(__VA_ARGS__)))
#define Z_EXPR_BIT_LEN_5(H, ...)  COND_CODE_1(H, (H, __VA_ARGS__), (Z_EXPR_BIT_LEN_4(__VA_ARGS__)))
#define Z_EXPR_BIT_LEN_6(H, ...)  COND_CODE_1(H, (H, __VA_ARGS__), (Z_EXPR_BIT_LEN_5(__VA_ARGS__)))
#define Z_EXPR_BIT_LEN_7(H, ...)  COND_CODE_1(H, (H, __VA_ARGS__), (Z_EXPR_BIT_LEN_6(__VA_ARGS__)))
#define Z_EXPR_BIT_LEN_8(H, ...)  COND_CODE_1(H, (H, __VA_ARGS__), (Z_EXPR_BIT_LEN_7(__VA_ARGS__)))
#define Z_EXPR_BIT_LEN_9(H, ...)  COND_CODE_1(H, (H, __VA_ARGS__), (Z_EXPR_BIT_LEN_8(__VA_ARGS__)))
#define Z_EXPR_BIT_LEN_10(H, ...) COND_CODE_1(H, (H, __VA_ARGS__), (Z_EXPR_BIT_LEN_9(__VA_ARGS__)))
#define Z_EXPR_BIT_LEN_11(H, ...) COND_CODE_1(H, (H, __VA_ARGS__), (Z_EXPR_BIT_LEN_10(__VA_ARGS__)))
#define Z_EXPR_BIT_LEN_12(H, ...) COND_CODE_1(H, (H, __VA_ARGS__), (Z_EXPR_BIT_LEN_11(__VA_ARGS__)))
#define Z_EXPR_BIT_LEN_13(H, ...) COND_CODE_1(H, (H, __VA_ARGS__), (Z_EXPR_BIT_LEN_12(__VA_ARGS__)))
#define Z_EXPR_BIT_LEN_14(H, ...) COND_CODE_1(H, (H, __VA_ARGS__), (Z_EXPR_BIT_LEN_13(__VA_ARGS__)))
#define Z_EXPR_BIT_LEN_15(H, ...) COND_CODE_1(H, (H, __VA_ARGS__), (Z_EXPR_BIT_LEN_14(__VA_ARGS__)))
#define Z_EXPR_BIT_LEN_16(H, ...) COND_CODE_1(H, (H, __VA_ARGS__), (Z_EXPR_BIT_LEN_15(__VA_ARGS__)))
#define Z_EXPR_BIT_LEN_17(H, ...) COND_CODE_1(H, (H, __VA_ARGS__), (Z_EXPR_BIT_LEN_16(__VA_ARGS__)))
#define Z_EXPR_BIT_LEN_18(H, ...) COND_CODE_1(H, (H, __VA_ARGS__), (Z_EXPR_BIT_LEN_17(__VA_ARGS__)))
#define Z_EXPR_BIT_LEN_19(H, ...) COND_CODE_1(H, (H, __VA_ARGS__), (Z_EXPR_BIT_LEN_18(__VA_ARGS__)))
#define Z_EXPR_BIT_LEN_20(H, ...) COND_CODE_1(H, (H, __VA_ARGS__), (Z_EXPR_BIT_LEN_19(__VA_ARGS__)))
#define Z_EXPR_BIT_LEN_21(H, ...) COND_CODE_1(H, (H, __VA_ARGS__), (Z_EXPR_BIT_LEN_20(__VA_ARGS__)))
#define Z_EXPR_BIT_LEN_22(H, ...) COND_CODE_1(H, (H, __VA_ARGS__), (Z_EXPR_BIT_LEN_21(__VA_ARGS__)))
#define Z_EXPR_BIT_LEN_23(H, ...) COND_CODE_1(H, (H, __VA_ARGS__), (Z_EXPR_BIT_LEN_22(__VA_ARGS__)))
#define Z_EXPR_BIT_LEN_24(H, ...) COND_CODE_1(H, (H, __VA_ARGS__), (Z_EXPR_BIT_LEN_23(__VA_ARGS__)))
#define Z_EXPR_BIT_LEN_25(H, ...) COND_CODE_1(H, (H, __VA_ARGS__), (Z_EXPR_BIT_LEN_24(__VA_ARGS__)))
#define Z_EXPR_BIT_LEN_26(H, ...) COND_CODE_1(H, (H, __VA_ARGS__), (Z_EXPR_BIT_LEN_25(__VA_ARGS__)))
#define Z_EXPR_BIT_LEN_27(H, ...) COND_CODE_1(H, (H, __VA_ARGS__), (Z_EXPR_BIT_LEN_26(__VA_ARGS__)))
#define Z_EXPR_BIT_LEN_28(H, ...) COND_CODE_1(H, (H, __VA_ARGS__), (Z_EXPR_BIT_LEN_27(__VA_ARGS__)))
#define Z_EXPR_BIT_LEN_29(H, ...) COND_CODE_1(H, (H, __VA_ARGS__), (Z_EXPR_BIT_LEN_28(__VA_ARGS__)))
#define Z_EXPR_BIT_LEN_30(H, ...) COND_CODE_1(H, (H, __VA_ARGS__), (Z_EXPR_BIT_LEN_29(__VA_ARGS__)))
#define Z_EXPR_BIT_LEN_31(H, ...) COND_CODE_1(H, (H, __VA_ARGS__), (Z_EXPR_BIT_LEN_30(__VA_ARGS__)))

#define Z_EXPR_BIT_LEN(...)                                                                        \
	NUM_VA_ARGS_LESS_1(UTIL_CAT(Z_EXPR_BIT_LEN_, NUM_VA_ARGS_LESS_1(__VA_ARGS__))(__VA_ARGS__))

/*
 * Bitwise operation
 */

#define Z_EXPR_BIT_NOT_0 1
#define Z_EXPR_BIT_NOT_1 0

#define Z_EXPR_BIT_AND_00 0
#define Z_EXPR_BIT_AND_01 0
#define Z_EXPR_BIT_AND_10 0
#define Z_EXPR_BIT_AND_11 1

#define Z_EXPR_BIT_OR_00 0
#define Z_EXPR_BIT_OR_01 1
#define Z_EXPR_BIT_OR_10 1
#define Z_EXPR_BIT_OR_11 1

#define Z_EXPR_BIT_XOR_00 0
#define Z_EXPR_BIT_XOR_01 1
#define Z_EXPR_BIT_XOR_10 1
#define Z_EXPR_BIT_XOR_11 0

/**
 * Performs bitwise OR on two 32-bit numbers
 */
#define Z_EXPR_DO_OR(a31, a30, a29, a28, a27, a26, a25, a24, a23, a22, a21, a20, a19, a18, a17,    \
		     a16, a15, a14, a13, a12, a11, a10, a09, a08, a07, a06, a05, a04, a03, a02,    \
		     a01, a00, b31, b30, b29, b28, b27, b26, b25, b24, b23, b22, b21, b20, b19,    \
		     b18, b17, b16, b15, b14, b13, b12, b11, b10, b09, b08, b07, b06, b05, b04,    \
		     b03, b02, b01, b00)                                                           \
	Z_EXPR_BIT_OR_##a31##b31, Z_EXPR_BIT_OR_##a30##b30, Z_EXPR_BIT_OR_##a29##b29,              \
		Z_EXPR_BIT_OR_##a28##b28, Z_EXPR_BIT_OR_##a27##b27, Z_EXPR_BIT_OR_##a26##b26,      \
		Z_EXPR_BIT_OR_##a25##b25, Z_EXPR_BIT_OR_##a24##b24, Z_EXPR_BIT_OR_##a23##b23,      \
		Z_EXPR_BIT_OR_##a22##b22, Z_EXPR_BIT_OR_##a21##b21, Z_EXPR_BIT_OR_##a20##b20,      \
		Z_EXPR_BIT_OR_##a19##b19, Z_EXPR_BIT_OR_##a18##b18, Z_EXPR_BIT_OR_##a17##b17,      \
		Z_EXPR_BIT_OR_##a16##b16, Z_EXPR_BIT_OR_##a15##b15, Z_EXPR_BIT_OR_##a14##b14,      \
		Z_EXPR_BIT_OR_##a13##b13, Z_EXPR_BIT_OR_##a12##b12, Z_EXPR_BIT_OR_##a11##b11,      \
		Z_EXPR_BIT_OR_##a10##b10, Z_EXPR_BIT_OR_##a09##b09, Z_EXPR_BIT_OR_##a08##b08,      \
		Z_EXPR_BIT_OR_##a07##b07, Z_EXPR_BIT_OR_##a06##b06, Z_EXPR_BIT_OR_##a05##b05,      \
		Z_EXPR_BIT_OR_##a04##b04, Z_EXPR_BIT_OR_##a03##b03, Z_EXPR_BIT_OR_##a02##b02,      \
		Z_EXPR_BIT_OR_##a01##b01, Z_EXPR_BIT_OR_##a00##b00

/**
 * Performs bitwise AND on two 32-bit numbers
 */
#define Z_EXPR_DO_AND(a31, a30, a29, a28, a27, a26, a25, a24, a23, a22, a21, a20, a19, a18, a17,   \
		      a16, a15, a14, a13, a12, a11, a10, a09, a08, a07, a06, a05, a04, a03, a02,   \
		      a01, a00, b31, b30, b29, b28, b27, b26, b25, b24, b23, b22, b21, b20, b19,   \
		      b18, b17, b16, b15, b14, b13, b12, b11, b10, b09, b08, b07, b06, b05, b04,   \
		      b03, b02, b01, b00)                                                          \
	Z_EXPR_BIT_AND_##a31##b31, Z_EXPR_BIT_AND_##a30##b30, Z_EXPR_BIT_AND_##a29##b29,           \
		Z_EXPR_BIT_AND_##a28##b28, Z_EXPR_BIT_AND_##a27##b27, Z_EXPR_BIT_AND_##a26##b26,   \
		Z_EXPR_BIT_AND_##a25##b25, Z_EXPR_BIT_AND_##a24##b24, Z_EXPR_BIT_AND_##a23##b23,   \
		Z_EXPR_BIT_AND_##a22##b22, Z_EXPR_BIT_AND_##a21##b21, Z_EXPR_BIT_AND_##a20##b20,   \
		Z_EXPR_BIT_AND_##a19##b19, Z_EXPR_BIT_AND_##a18##b18, Z_EXPR_BIT_AND_##a17##b17,   \
		Z_EXPR_BIT_AND_##a16##b16, Z_EXPR_BIT_AND_##a15##b15, Z_EXPR_BIT_AND_##a14##b14,   \
		Z_EXPR_BIT_AND_##a13##b13, Z_EXPR_BIT_AND_##a12##b12, Z_EXPR_BIT_AND_##a11##b11,   \
		Z_EXPR_BIT_AND_##a10##b10, Z_EXPR_BIT_AND_##a09##b09, Z_EXPR_BIT_AND_##a08##b08,   \
		Z_EXPR_BIT_AND_##a07##b07, Z_EXPR_BIT_AND_##a06##b06, Z_EXPR_BIT_AND_##a05##b05,   \
		Z_EXPR_BIT_AND_##a04##b04, Z_EXPR_BIT_AND_##a03##b03, Z_EXPR_BIT_AND_##a02##b02,   \
		Z_EXPR_BIT_AND_##a01##b01, Z_EXPR_BIT_AND_##a00##b00

/**
 * Performs bitwise XOR on two 32-bit numbers in bit-args format.
 */
#define Z_EXPR_DO_XOR(a31, a30, a29, a28, a27, a26, a25, a24, a23, a22, a21, a20, a19, a18, a17,   \
		      a16, a15, a14, a13, a12, a11, a10, a09, a08, a07, a06, a05, a04, a03, a02,   \
		      a01, a00, b31, b30, b29, b28, b27, b26, b25, b24, b23, b22, b21, b20, b19,   \
		      b18, b17, b16, b15, b14, b13, b12, b11, b10, b09, b08, b07, b06, b05, b04,   \
		      b03, b02, b01, b00)                                                          \
	Z_EXPR_BIT_XOR_##a31##b31, Z_EXPR_BIT_XOR_##a30##b30, Z_EXPR_BIT_XOR_##a29##b29,           \
		Z_EXPR_BIT_XOR_##a28##b28, Z_EXPR_BIT_XOR_##a27##b27, Z_EXPR_BIT_XOR_##a26##b26,   \
		Z_EXPR_BIT_XOR_##a25##b25, Z_EXPR_BIT_XOR_##a24##b24, Z_EXPR_BIT_XOR_##a23##b23,   \
		Z_EXPR_BIT_XOR_##a22##b22, Z_EXPR_BIT_XOR_##a21##b21, Z_EXPR_BIT_XOR_##a20##b20,   \
		Z_EXPR_BIT_XOR_##a19##b19, Z_EXPR_BIT_XOR_##a18##b18, Z_EXPR_BIT_XOR_##a17##b17,   \
		Z_EXPR_BIT_XOR_##a16##b16, Z_EXPR_BIT_XOR_##a15##b15, Z_EXPR_BIT_XOR_##a14##b14,   \
		Z_EXPR_BIT_XOR_##a13##b13, Z_EXPR_BIT_XOR_##a12##b12, Z_EXPR_BIT_XOR_##a11##b11,   \
		Z_EXPR_BIT_XOR_##a10##b10, Z_EXPR_BIT_XOR_##a09##b09, Z_EXPR_BIT_XOR_##a08##b08,   \
		Z_EXPR_BIT_XOR_##a07##b07, Z_EXPR_BIT_XOR_##a06##b06, Z_EXPR_BIT_XOR_##a05##b05,   \
		Z_EXPR_BIT_XOR_##a04##b04, Z_EXPR_BIT_XOR_##a03##b03, Z_EXPR_BIT_XOR_##a02##b02,   \
		Z_EXPR_BIT_XOR_##a01##b01, Z_EXPR_BIT_XOR_##a00##b00

/**
 * Performs bitwise NOT on a 32-bit number in bit-args format.
 */
#define Z_EXPR_DO_NOT(a31, a30, a29, a28, a27, a26, a25, a24, a23, a22, a21, a20, a19, a18, a17,   \
		      a16, a15, a14, a13, a12, a11, a10, a09, a08, a07, a06, a05, a04, a03, a02,   \
		      a01, a00)                                                                    \
	Z_EXPR_BIT_NOT_##a31, Z_EXPR_BIT_NOT_##a30, Z_EXPR_BIT_NOT_##a29, Z_EXPR_BIT_NOT_##a28,    \
		Z_EXPR_BIT_NOT_##a27, Z_EXPR_BIT_NOT_##a26, Z_EXPR_BIT_NOT_##a25,                  \
		Z_EXPR_BIT_NOT_##a24, Z_EXPR_BIT_NOT_##a23, Z_EXPR_BIT_NOT_##a22,                  \
		Z_EXPR_BIT_NOT_##a21, Z_EXPR_BIT_NOT_##a20, Z_EXPR_BIT_NOT_##a19,                  \
		Z_EXPR_BIT_NOT_##a18, Z_EXPR_BIT_NOT_##a17, Z_EXPR_BIT_NOT_##a16,                  \
		Z_EXPR_BIT_NOT_##a15, Z_EXPR_BIT_NOT_##a14, Z_EXPR_BIT_NOT_##a13,                  \
		Z_EXPR_BIT_NOT_##a12, Z_EXPR_BIT_NOT_##a11, Z_EXPR_BIT_NOT_##a10,                  \
		Z_EXPR_BIT_NOT_##a09, Z_EXPR_BIT_NOT_##a08, Z_EXPR_BIT_NOT_##a07,                  \
		Z_EXPR_BIT_NOT_##a06, Z_EXPR_BIT_NOT_##a05, Z_EXPR_BIT_NOT_##a04,                  \
		Z_EXPR_BIT_NOT_##a03, Z_EXPR_BIT_NOT_##a02, Z_EXPR_BIT_NOT_##a01,                  \
		Z_EXPR_BIT_NOT_##a00

/*
 * shift operations
 */

#define Z_EXPR_SHIFT_PAD_ZEROS_1  0
#define Z_EXPR_SHIFT_PAD_ZEROS_2  0, 0
#define Z_EXPR_SHIFT_PAD_ZEROS_3  0, 0, 0
#define Z_EXPR_SHIFT_PAD_ZEROS_4  0, 0, 0, 0
#define Z_EXPR_SHIFT_PAD_ZEROS_5  0, 0, 0, 0, 0
#define Z_EXPR_SHIFT_PAD_ZEROS_6  0, 0, 0, 0, 0, 0
#define Z_EXPR_SHIFT_PAD_ZEROS_7  0, 0, 0, 0, 0, 0, 0
#define Z_EXPR_SHIFT_PAD_ZEROS_8  0, 0, 0, 0, 0, 0, 0, 0
#define Z_EXPR_SHIFT_PAD_ZEROS_9  0, 0, 0, 0, 0, 0, 0, 0, 0
#define Z_EXPR_SHIFT_PAD_ZEROS_10 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
#define Z_EXPR_SHIFT_PAD_ZEROS_11 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
#define Z_EXPR_SHIFT_PAD_ZEROS_12 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
#define Z_EXPR_SHIFT_PAD_ZEROS_13 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
#define Z_EXPR_SHIFT_PAD_ZEROS_14 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
#define Z_EXPR_SHIFT_PAD_ZEROS_15 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
#define Z_EXPR_SHIFT_PAD_ZEROS_16 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
#define Z_EXPR_SHIFT_PAD_ZEROS_17 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
#define Z_EXPR_SHIFT_PAD_ZEROS_18 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
#define Z_EXPR_SHIFT_PAD_ZEROS_19 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
#define Z_EXPR_SHIFT_PAD_ZEROS_20 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
#define Z_EXPR_SHIFT_PAD_ZEROS_21 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
#define Z_EXPR_SHIFT_PAD_ZEROS_22 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
#define Z_EXPR_SHIFT_PAD_ZEROS_23                                                                  \
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
#define Z_EXPR_SHIFT_PAD_ZEROS_24                                                                  \
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
#define Z_EXPR_SHIFT_PAD_ZEROS_25                                                                  \
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
#define Z_EXPR_SHIFT_PAD_ZEROS_26                                                                  \
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
#define Z_EXPR_SHIFT_PAD_ZEROS_27                                                                  \
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
#define Z_EXPR_SHIFT_PAD_ZEROS_28                                                                  \
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
#define Z_EXPR_SHIFT_PAD_ZEROS_29                                                                  \
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
#define Z_EXPR_SHIFT_PAD_ZEROS_30                                                                  \
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
#define Z_EXPR_SHIFT_PAD_ZEROS_31                                                                  \
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0

#define Z_EXPR_LSH_0(...)    GET_ARGS_LESS_N(0, __VA_ARGS__)
#define Z_EXPR_LSH_1(...)    GET_ARGS_LESS_N(1, __VA_ARGS__), Z_EXPR_SHIFT_PAD_ZEROS_1
#define Z_EXPR_LSH_2(...)    GET_ARGS_LESS_N(2, __VA_ARGS__), Z_EXPR_SHIFT_PAD_ZEROS_2
#define Z_EXPR_LSH_3(...)    GET_ARGS_LESS_N(3, __VA_ARGS__), Z_EXPR_SHIFT_PAD_ZEROS_3
#define Z_EXPR_LSH_4(...)    GET_ARGS_LESS_N(4, __VA_ARGS__), Z_EXPR_SHIFT_PAD_ZEROS_4
#define Z_EXPR_LSH_5(...)    GET_ARGS_LESS_N(5, __VA_ARGS__), Z_EXPR_SHIFT_PAD_ZEROS_5
#define Z_EXPR_LSH_6(...)    GET_ARGS_LESS_N(6, __VA_ARGS__), Z_EXPR_SHIFT_PAD_ZEROS_6
#define Z_EXPR_LSH_7(...)    GET_ARGS_LESS_N(7, __VA_ARGS__), Z_EXPR_SHIFT_PAD_ZEROS_7
#define Z_EXPR_LSH_8(...)    GET_ARGS_LESS_N(8, __VA_ARGS__), Z_EXPR_SHIFT_PAD_ZEROS_8
#define Z_EXPR_LSH_9(...)    GET_ARGS_LESS_N(9, __VA_ARGS__), Z_EXPR_SHIFT_PAD_ZEROS_9
#define Z_EXPR_LSH_10(...)   GET_ARGS_LESS_N(10, __VA_ARGS__), Z_EXPR_SHIFT_PAD_ZEROS_10
#define Z_EXPR_LSH_11(...)   GET_ARGS_LESS_N(11, __VA_ARGS__), Z_EXPR_SHIFT_PAD_ZEROS_11
#define Z_EXPR_LSH_12(...)   GET_ARGS_LESS_N(12, __VA_ARGS__), Z_EXPR_SHIFT_PAD_ZEROS_12
#define Z_EXPR_LSH_13(...)   GET_ARGS_LESS_N(13, __VA_ARGS__), Z_EXPR_SHIFT_PAD_ZEROS_13
#define Z_EXPR_LSH_14(...)   GET_ARGS_LESS_N(14, __VA_ARGS__), Z_EXPR_SHIFT_PAD_ZEROS_14
#define Z_EXPR_LSH_15(...)   GET_ARGS_LESS_N(15, __VA_ARGS__), Z_EXPR_SHIFT_PAD_ZEROS_15
#define Z_EXPR_LSH_16(...)   GET_ARGS_LESS_N(16, __VA_ARGS__), Z_EXPR_SHIFT_PAD_ZEROS_16
#define Z_EXPR_LSH_17(...)   GET_ARGS_LESS_N(17, __VA_ARGS__), Z_EXPR_SHIFT_PAD_ZEROS_17
#define Z_EXPR_LSH_18(...)   GET_ARGS_LESS_N(18, __VA_ARGS__), Z_EXPR_SHIFT_PAD_ZEROS_18
#define Z_EXPR_LSH_19(...)   GET_ARGS_LESS_N(19, __VA_ARGS__), Z_EXPR_SHIFT_PAD_ZEROS_19
#define Z_EXPR_LSH_20(...)   GET_ARGS_LESS_N(20, __VA_ARGS__), Z_EXPR_SHIFT_PAD_ZEROS_20
#define Z_EXPR_LSH_21(...)   GET_ARGS_LESS_N(21, __VA_ARGS__), Z_EXPR_SHIFT_PAD_ZEROS_21
#define Z_EXPR_LSH_22(...)   GET_ARGS_LESS_N(22, __VA_ARGS__), Z_EXPR_SHIFT_PAD_ZEROS_22
#define Z_EXPR_LSH_23(...)   GET_ARGS_LESS_N(23, __VA_ARGS__), Z_EXPR_SHIFT_PAD_ZEROS_23
#define Z_EXPR_LSH_24(...)   GET_ARGS_LESS_N(24, __VA_ARGS__), Z_EXPR_SHIFT_PAD_ZEROS_24
#define Z_EXPR_LSH_25(...)   GET_ARGS_LESS_N(25, __VA_ARGS__), Z_EXPR_SHIFT_PAD_ZEROS_25
#define Z_EXPR_LSH_26(...)   GET_ARGS_LESS_N(26, __VA_ARGS__), Z_EXPR_SHIFT_PAD_ZEROS_26
#define Z_EXPR_LSH_27(...)   GET_ARGS_LESS_N(27, __VA_ARGS__), Z_EXPR_SHIFT_PAD_ZEROS_27
#define Z_EXPR_LSH_28(...)   GET_ARGS_LESS_N(28, __VA_ARGS__), Z_EXPR_SHIFT_PAD_ZEROS_28
#define Z_EXPR_LSH_29(...)   GET_ARGS_LESS_N(29, __VA_ARGS__), Z_EXPR_SHIFT_PAD_ZEROS_29
#define Z_EXPR_LSH_30(...)   GET_ARGS_LESS_N(30, __VA_ARGS__), Z_EXPR_SHIFT_PAD_ZEROS_30
#define Z_EXPR_LSH_31(...)   GET_ARGS_LESS_N(31, __VA_ARGS__), Z_EXPR_SHIFT_PAD_ZEROS_31
#define Z_EXPR_LSH_N(N, ...) UTIL_CAT(Z_EXPR_LSH_, N)(__VA_ARGS__)

#define Z_EXPR_RSH_0(...)    GET_ARGS_FIRST_N(32, __VA_ARGS__)
#define Z_EXPR_RSH_1(...)    Z_EXPR_SHIFT_PAD_ZEROS_1, GET_ARGS_FIRST_N(31, __VA_ARGS__)
#define Z_EXPR_RSH_2(...)    Z_EXPR_SHIFT_PAD_ZEROS_2, GET_ARGS_FIRST_N(30, __VA_ARGS__)
#define Z_EXPR_RSH_3(...)    Z_EXPR_SHIFT_PAD_ZEROS_3, GET_ARGS_FIRST_N(29, __VA_ARGS__)
#define Z_EXPR_RSH_4(...)    Z_EXPR_SHIFT_PAD_ZEROS_4, GET_ARGS_FIRST_N(28, __VA_ARGS__)
#define Z_EXPR_RSH_5(...)    Z_EXPR_SHIFT_PAD_ZEROS_5, GET_ARGS_FIRST_N(27, __VA_ARGS__)
#define Z_EXPR_RSH_6(...)    Z_EXPR_SHIFT_PAD_ZEROS_6, GET_ARGS_FIRST_N(26, __VA_ARGS__)
#define Z_EXPR_RSH_7(...)    Z_EXPR_SHIFT_PAD_ZEROS_7, GET_ARGS_FIRST_N(25, __VA_ARGS__)
#define Z_EXPR_RSH_8(...)    Z_EXPR_SHIFT_PAD_ZEROS_8, GET_ARGS_FIRST_N(24, __VA_ARGS__)
#define Z_EXPR_RSH_9(...)    Z_EXPR_SHIFT_PAD_ZEROS_9, GET_ARGS_FIRST_N(23, __VA_ARGS__)
#define Z_EXPR_RSH_10(...)   Z_EXPR_SHIFT_PAD_ZEROS_10, GET_ARGS_FIRST_N(22, __VA_ARGS__)
#define Z_EXPR_RSH_11(...)   Z_EXPR_SHIFT_PAD_ZEROS_11, GET_ARGS_FIRST_N(21, __VA_ARGS__)
#define Z_EXPR_RSH_12(...)   Z_EXPR_SHIFT_PAD_ZEROS_12, GET_ARGS_FIRST_N(20, __VA_ARGS__)
#define Z_EXPR_RSH_13(...)   Z_EXPR_SHIFT_PAD_ZEROS_13, GET_ARGS_FIRST_N(19, __VA_ARGS__)
#define Z_EXPR_RSH_14(...)   Z_EXPR_SHIFT_PAD_ZEROS_14, GET_ARGS_FIRST_N(18, __VA_ARGS__)
#define Z_EXPR_RSH_15(...)   Z_EXPR_SHIFT_PAD_ZEROS_15, GET_ARGS_FIRST_N(17, __VA_ARGS__)
#define Z_EXPR_RSH_16(...)   Z_EXPR_SHIFT_PAD_ZEROS_16, GET_ARGS_FIRST_N(16, __VA_ARGS__)
#define Z_EXPR_RSH_17(...)   Z_EXPR_SHIFT_PAD_ZEROS_17, GET_ARGS_FIRST_N(15, __VA_ARGS__)
#define Z_EXPR_RSH_18(...)   Z_EXPR_SHIFT_PAD_ZEROS_18, GET_ARGS_FIRST_N(14, __VA_ARGS__)
#define Z_EXPR_RSH_19(...)   Z_EXPR_SHIFT_PAD_ZEROS_19, GET_ARGS_FIRST_N(13, __VA_ARGS__)
#define Z_EXPR_RSH_20(...)   Z_EXPR_SHIFT_PAD_ZEROS_20, GET_ARGS_FIRST_N(12, __VA_ARGS__)
#define Z_EXPR_RSH_21(...)   Z_EXPR_SHIFT_PAD_ZEROS_21, GET_ARGS_FIRST_N(11, __VA_ARGS__)
#define Z_EXPR_RSH_22(...)   Z_EXPR_SHIFT_PAD_ZEROS_22, GET_ARGS_FIRST_N(10, __VA_ARGS__)
#define Z_EXPR_RSH_23(...)   Z_EXPR_SHIFT_PAD_ZEROS_23, GET_ARGS_FIRST_N(9, __VA_ARGS__)
#define Z_EXPR_RSH_24(...)   Z_EXPR_SHIFT_PAD_ZEROS_24, GET_ARGS_FIRST_N(8, __VA_ARGS__)
#define Z_EXPR_RSH_25(...)   Z_EXPR_SHIFT_PAD_ZEROS_25, GET_ARGS_FIRST_N(7, __VA_ARGS__)
#define Z_EXPR_RSH_26(...)   Z_EXPR_SHIFT_PAD_ZEROS_26, GET_ARGS_FIRST_N(6, __VA_ARGS__)
#define Z_EXPR_RSH_27(...)   Z_EXPR_SHIFT_PAD_ZEROS_27, GET_ARGS_FIRST_N(5, __VA_ARGS__)
#define Z_EXPR_RSH_28(...)   Z_EXPR_SHIFT_PAD_ZEROS_28, GET_ARGS_FIRST_N(4, __VA_ARGS__)
#define Z_EXPR_RSH_29(...)   Z_EXPR_SHIFT_PAD_ZEROS_29, GET_ARGS_FIRST_N(3, __VA_ARGS__)
#define Z_EXPR_RSH_30(...)   Z_EXPR_SHIFT_PAD_ZEROS_30, GET_ARGS_FIRST_N(2, __VA_ARGS__)
#define Z_EXPR_RSH_31(...)   Z_EXPR_SHIFT_PAD_ZEROS_31, GET_ARGS_FIRST_N(1, __VA_ARGS__)
#define Z_EXPR_RSH_N(N, ...) UTIL_CAT(Z_EXPR_RSH_, N)(__VA_ARGS__)

/**
 * Perform left-shift operation
 */
#define Z_EXPR_LSH(...)                                                                            \
	COND_CODE_1(Z_EXPR_IS_LT(GET_ARGS_LESS_N(32, __VA_ARGS__), Z_EXPR_BITS_32U),               \
		    (Z_EXPR_LSH_N(Z_EXPR_BITS_TO_DEC(GET_ARGS_LESS_N(32, __VA_ARGS__)),            \
						      GET_ARGS_FIRST_N(32, __VA_ARGS__))),         \
		    (Z_EXPR_BITS_0U))

/**
 * Perform right-shift operation
 */
#define Z_EXPR_RSH(...)                                                                            \
	COND_CODE_1(Z_EXPR_IS_LT(GET_ARGS_LESS_N(32, __VA_ARGS__), Z_EXPR_BITS_32U),               \
		    (Z_EXPR_RSH_N(Z_EXPR_BITS_TO_DEC(GET_ARGS_LESS_N(32, __VA_ARGS__)),            \
						     GET_ARGS_FIRST_N(32, __VA_ARGS__))),          \
		    (Z_EXPR_BITS_0U))

/* Z_UTIL_INTERNAL_<OP>_<v1><v2><c>  c, r */
#define Z_EXPR_OP_ADD_000             0, 0
#define Z_EXPR_OP_ADD_001             0, 1
#define Z_EXPR_OP_ADD_010             0, 1
#define Z_EXPR_OP_ADD_011             1, 0
#define Z_EXPR_OP_ADD_100             0, 1
#define Z_EXPR_OP_ADD_101             1, 0
#define Z_EXPR_OP_ADD_110             1, 0
#define Z_EXPR_OP_ADD_111             1, 1
#define Z_EXPR_OP_ADD(v1, v2, c, ...) Z_EXPR_OP_ADD_##v1##v2##c, __VA_ARGS__

#define Z_EXPR_OP_SUB_000             0, 0
#define Z_EXPR_OP_SUB_001             1, 1
#define Z_EXPR_OP_SUB_010             1, 1
#define Z_EXPR_OP_SUB_011             1, 0
#define Z_EXPR_OP_SUB_100             0, 1
#define Z_EXPR_OP_SUB_101             0, 0
#define Z_EXPR_OP_SUB_110             0, 0
#define Z_EXPR_OP_SUB_111             1, 1
#define Z_EXPR_OP_SUB(v1, v2, c, ...) Z_EXPR_OP_SUB_##v1##v2##c, __VA_ARGS__

/**
 * @brief Execute `fn` for every 32 bit in `v1` and `v2`.
 * @param fn Macro to evoke on every iteration. It takes a bit of `v1` and
 *           `v2`, and a carry bit `c` as its input arguments. The return
 *           must be the result `r` and the carry bit `c` of the operation.
 *           The order must be `c`, `r`.
 * @param v1, v2 These are 12 bit numbers on which a bitwise operation is
 *               performed. Use Z_EXPR_BITS to convert from a literal
 *               integer to a 12 bit number.
 * @returns
 * A 12 bit number representing the result `r` of the bitwise operation.
 * Use Z_EXPR_BITS_TO_DEC to convert from a 12 bit number to an integer
 * literal. Additionally, the last carry bit `c` is also returned.
 * The order is `r`, `c`.
 */

#define Z_EXPR_OP_0(fn, v1, v2, ...)  fn(v1, v2, __VA_ARGS__)
#define Z_EXPR_OP_1(fn, v1, v2, ...)  fn(v1, v2, Z_EXPR_OP_0(fn, __VA_ARGS__))
#define Z_EXPR_OP_2(fn, v1, v2, ...)  fn(v1, v2, Z_EXPR_OP_1(fn, __VA_ARGS__))
#define Z_EXPR_OP_3(fn, v1, v2, ...)  fn(v1, v2, Z_EXPR_OP_2(fn, __VA_ARGS__))
#define Z_EXPR_OP_4(fn, v1, v2, ...)  fn(v1, v2, Z_EXPR_OP_3(fn, __VA_ARGS__))
#define Z_EXPR_OP_5(fn, v1, v2, ...)  fn(v1, v2, Z_EXPR_OP_4(fn, __VA_ARGS__))
#define Z_EXPR_OP_6(fn, v1, v2, ...)  fn(v1, v2, Z_EXPR_OP_5(fn, __VA_ARGS__))
#define Z_EXPR_OP_7(fn, v1, v2, ...)  fn(v1, v2, Z_EXPR_OP_6(fn, __VA_ARGS__))
#define Z_EXPR_OP_8(fn, v1, v2, ...)  fn(v1, v2, Z_EXPR_OP_7(fn, __VA_ARGS__))
#define Z_EXPR_OP_9(fn, v1, v2, ...)  fn(v1, v2, Z_EXPR_OP_8(fn, __VA_ARGS__))
#define Z_EXPR_OP_10(fn, v1, v2, ...) fn(v1, v2, Z_EXPR_OP_9(fn, __VA_ARGS__))
#define Z_EXPR_OP_11(fn, v1, v2, ...) fn(v1, v2, Z_EXPR_OP_10(fn, __VA_ARGS__))
#define Z_EXPR_OP_12(fn, v1, v2, ...) fn(v1, v2, Z_EXPR_OP_11(fn, __VA_ARGS__))
#define Z_EXPR_OP_13(fn, v1, v2, ...) fn(v1, v2, Z_EXPR_OP_12(fn, __VA_ARGS__))
#define Z_EXPR_OP_14(fn, v1, v2, ...) fn(v1, v2, Z_EXPR_OP_13(fn, __VA_ARGS__))
#define Z_EXPR_OP_15(fn, v1, v2, ...) fn(v1, v2, Z_EXPR_OP_14(fn, __VA_ARGS__))
#define Z_EXPR_OP_16(fn, v1, v2, ...) fn(v1, v2, Z_EXPR_OP_15(fn, __VA_ARGS__))
#define Z_EXPR_OP_17(fn, v1, v2, ...) fn(v1, v2, Z_EXPR_OP_16(fn, __VA_ARGS__))
#define Z_EXPR_OP_18(fn, v1, v2, ...) fn(v1, v2, Z_EXPR_OP_17(fn, __VA_ARGS__))
#define Z_EXPR_OP_19(fn, v1, v2, ...) fn(v1, v2, Z_EXPR_OP_18(fn, __VA_ARGS__))
#define Z_EXPR_OP_20(fn, v1, v2, ...) fn(v1, v2, Z_EXPR_OP_19(fn, __VA_ARGS__))
#define Z_EXPR_OP_21(fn, v1, v2, ...) fn(v1, v2, Z_EXPR_OP_20(fn, __VA_ARGS__))
#define Z_EXPR_OP_22(fn, v1, v2, ...) fn(v1, v2, Z_EXPR_OP_21(fn, __VA_ARGS__))
#define Z_EXPR_OP_23(fn, v1, v2, ...) fn(v1, v2, Z_EXPR_OP_22(fn, __VA_ARGS__))
#define Z_EXPR_OP_24(fn, v1, v2, ...) fn(v1, v2, Z_EXPR_OP_23(fn, __VA_ARGS__))
#define Z_EXPR_OP_25(fn, v1, v2, ...) fn(v1, v2, Z_EXPR_OP_24(fn, __VA_ARGS__))
#define Z_EXPR_OP_26(fn, v1, v2, ...) fn(v1, v2, Z_EXPR_OP_25(fn, __VA_ARGS__))
#define Z_EXPR_OP_27(fn, v1, v2, ...) fn(v1, v2, Z_EXPR_OP_26(fn, __VA_ARGS__))
#define Z_EXPR_OP_28(fn, v1, v2, ...) fn(v1, v2, Z_EXPR_OP_27(fn, __VA_ARGS__))
#define Z_EXPR_OP_29(fn, v1, v2, ...) fn(v1, v2, Z_EXPR_OP_28(fn, __VA_ARGS__))
#define Z_EXPR_OP_30(fn, v1, v2, ...) fn(v1, v2, Z_EXPR_OP_29(fn, __VA_ARGS__))
#define Z_EXPR_OP_31(fn, v1, v2, ...) fn(v1, v2, Z_EXPR_OP_30(fn, __VA_ARGS__))

#define Z_EXPR_DO_OP(...) Z_EXPR_OP_31(__VA_ARGS__)

/**
 * Creates a tuple (a31, b31, a30, b30, …) by interleaving two 32-bit sequences.
 *
 * @p ... The first 32 arguments represent the bits of the first number (a31..a00)
 *        and the next 32 represent the bits of the second number (b31..b00).
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

#define Z_EXPR_CARRY(...)  GET_ARG_N(1, __VA_ARGS__)
#define Z_EXPR_RESULT(...) GET_ARGS_FIRST_N(32, GET_ARGS_LESS_N(1, __VA_ARGS__))

#define Z_EXPR_ADD_N(...) Z_EXPR_OP_ADD(__VA_ARGS__)
#define Z_EXPR_ADD(...)   Z_EXPR_RESULT(Z_EXPR_DO_OP(Z_EXPR_ADD_N, Z_EXPR_TUPLIFY(__VA_ARGS__), 0))
#define Z_EXPR_SUB_N(...) Z_EXPR_OP_SUB(__VA_ARGS__)
#define Z_EXPR_SUB(...)   Z_EXPR_RESULT(Z_EXPR_DO_OP(Z_EXPR_SUB_N, Z_EXPR_TUPLIFY(__VA_ARGS__), 0))

/*
 * comparation
 */
#define Z_EXPR_DO_BIT_OR(v1, v2) Z_EXPR_BIT_OR_##v1##v2
#define Z_EXPR_BIT_OR(...)       Z_EXPR_DO_BIT_OR(__VA_ARGS__)
#define Z_EXPR_DO_BIT_NOT(v1)    Z_EXPR_BIT_NOT_##v1
#define Z_EXPR_BIT_NOT(...)      Z_EXPR_DO_BIT_NOT(__VA_ARGS__)

#define Z_EXPR_ALL_OR(a31, a30, a29, a28, a27, a26, a25, a24, a23, a22, a21, a20, a19, a18, a17,   \
		      a16, a15, a14, a13, a12, a11, a10, a09, a08, a07, a06, a05, a04, a03, a02,   \
		      a01, a00)                                                                    \
	Z_EXPR_BIT_OR(a31, Z_EXPR_BIT_OR(a30, Z_EXPR_BIT_OR(a29, Z_EXPR_BIT_OR(a28,                \
	Z_EXPR_BIT_OR(a27, Z_EXPR_BIT_OR(a26, Z_EXPR_BIT_OR(a25, Z_EXPR_BIT_OR(a24,                \
	Z_EXPR_BIT_OR(a23, Z_EXPR_BIT_OR(a22, Z_EXPR_BIT_OR(a21, Z_EXPR_BIT_OR(a20,                \
	Z_EXPR_BIT_OR(a19, Z_EXPR_BIT_OR(a18, Z_EXPR_BIT_OR(a17, Z_EXPR_BIT_OR(a16,                \
	Z_EXPR_BIT_OR(a15, Z_EXPR_BIT_OR(a14, Z_EXPR_BIT_OR(a13, Z_EXPR_BIT_OR(a12,                \
	Z_EXPR_BIT_OR(a11, Z_EXPR_BIT_OR(a10, Z_EXPR_BIT_OR(a09, Z_EXPR_BIT_OR(a08,                \
	Z_EXPR_BIT_OR(a07, Z_EXPR_BIT_OR(a06, Z_EXPR_BIT_OR(a05, Z_EXPR_BIT_OR(a04,                \
	Z_EXPR_BIT_OR(a03, Z_EXPR_BIT_OR(a02, Z_EXPR_BIT_OR(a01, a00)))))))))))))))))))))))))))))))

#define Z_EXPR_DO_IS_EQ(a31, a30, a29, a28, a27, a26, a25, a24, a23, a22, a21, a20, a19, a18, a17, \
			a16, a15, a14, a13, a12, a11, a10, a09, a08, a07, a06, a05, a04, a03, a02, \
			a01, a00, b31, b30, b29, b28, b27, b26, b25, b24, b23, b22, b21, b20, b19, \
			b18, b17, b16, b15, b14, b13, b12, b11, b10, b09, b08, b07, b06, b05, b04, \
			b03, b02, b01, b00)                                                        \
	Z_EXPR_BIT_NOT(Z_EXPR_ALL_OR(                                                              \
		Z_EXPR_BIT_XOR_##a31##b31, Z_EXPR_BIT_XOR_##a30##b30, Z_EXPR_BIT_XOR_##a29##b29,   \
		Z_EXPR_BIT_XOR_##a28##b28, Z_EXPR_BIT_XOR_##a27##b27, Z_EXPR_BIT_XOR_##a26##b26,   \
		Z_EXPR_BIT_XOR_##a25##b25, Z_EXPR_BIT_XOR_##a24##b24, Z_EXPR_BIT_XOR_##a23##b23,   \
		Z_EXPR_BIT_XOR_##a22##b22, Z_EXPR_BIT_XOR_##a21##b21, Z_EXPR_BIT_XOR_##a20##b20,   \
		Z_EXPR_BIT_XOR_##a19##b19, Z_EXPR_BIT_XOR_##a18##b18, Z_EXPR_BIT_XOR_##a17##b17,   \
		Z_EXPR_BIT_XOR_##a16##b16, Z_EXPR_BIT_XOR_##a15##b15, Z_EXPR_BIT_XOR_##a14##b14,   \
		Z_EXPR_BIT_XOR_##a13##b13, Z_EXPR_BIT_XOR_##a12##b12, Z_EXPR_BIT_XOR_##a11##b11,   \
		Z_EXPR_BIT_XOR_##a10##b10, Z_EXPR_BIT_XOR_##a09##b09, Z_EXPR_BIT_XOR_##a08##b08,   \
		Z_EXPR_BIT_XOR_##a07##b07, Z_EXPR_BIT_XOR_##a06##b06, Z_EXPR_BIT_XOR_##a05##b05,   \
		Z_EXPR_BIT_XOR_##a04##b04, Z_EXPR_BIT_XOR_##a03##b03, Z_EXPR_BIT_XOR_##a02##b02,   \
		Z_EXPR_BIT_XOR_##a01##b01, Z_EXPR_BIT_XOR_##a00##b00))

#define Z_EXPR_IS_LT(...) Z_EXPR_CARRY(Z_EXPR_DO_OP(Z_EXPR_SUB_N, Z_EXPR_TUPLIFY(__VA_ARGS__), 0))
#define Z_EXPR_IS_GT(...)                                                                          \
	Z_EXPR_IS_LT(GET_ARGS_LESS_N(32, __VA_ARGS__), GET_ARGS_FIRST_N(32, __VA_ARGS__))
#define Z_EXPR_IS_LE(...) Z_EXPR_BIT_NOT(Z_EXPR_IS_GT(__VA_ARGS__))
#define Z_EXPR_IS_GE(...) Z_EXPR_BIT_NOT(Z_EXPR_IS_LT(__VA_ARGS__))

#define Z_EXPR_EQ(...) COND_CODE_1(Z_EXPR_DO_IS_EQ(__VA_ARGS__), (Z_EXPR_BITS_1U), (Z_EXPR_BITS_0U))
#define Z_EXPR_LT(...) COND_CODE_1(Z_EXPR_IS_LT(__VA_ARGS__), (Z_EXPR_BITS_1U), (Z_EXPR_BITS_0U))
#define Z_EXPR_GT(...) COND_CODE_1(Z_EXPR_IS_GT(__VA_ARGS__), (Z_EXPR_BITS_1U), (Z_EXPR_BITS_0U))
#define Z_EXPR_LE(...) COND_CODE_1(Z_EXPR_IS_LE(__VA_ARGS__), (Z_EXPR_BITS_1U), (Z_EXPR_BITS_0U))
#define Z_EXPR_GE(...) COND_CODE_1(Z_EXPR_IS_GE(__VA_ARGS__), (Z_EXPR_BITS_1U), (Z_EXPR_BITS_0U))

/*
 * accumulator
 */

#define Z_EXPR_MAC_0(fn, ...)  fn(__VA_ARGS__)
#define Z_EXPR_MAC_1(fn, ...)  Z_EXPR_MAC_0(fn, fn(__VA_ARGS__))
#define Z_EXPR_MAC_2(fn, ...)  Z_EXPR_MAC_1(fn, fn(__VA_ARGS__))
#define Z_EXPR_MAC_3(fn, ...)  Z_EXPR_MAC_2(fn, fn(__VA_ARGS__))
#define Z_EXPR_MAC_4(fn, ...)  Z_EXPR_MAC_3(fn, fn(__VA_ARGS__))
#define Z_EXPR_MAC_5(fn, ...)  Z_EXPR_MAC_4(fn, fn(__VA_ARGS__))
#define Z_EXPR_MAC_6(fn, ...)  Z_EXPR_MAC_5(fn, fn(__VA_ARGS__))
#define Z_EXPR_MAC_7(fn, ...)  Z_EXPR_MAC_6(fn, fn(__VA_ARGS__))
#define Z_EXPR_MAC_8(fn, ...)  Z_EXPR_MAC_7(fn, fn(__VA_ARGS__))
#define Z_EXPR_MAC_9(fn, ...)  Z_EXPR_MAC_8(fn, fn(__VA_ARGS__))
#define Z_EXPR_MAC_10(fn, ...) Z_EXPR_MAC_9(fn, fn(__VA_ARGS__))
#define Z_EXPR_MAC_11(fn, ...) Z_EXPR_MAC_10(fn, fn(__VA_ARGS__))
#define Z_EXPR_MAC_12(fn, ...) Z_EXPR_MAC_11(fn, fn(__VA_ARGS__))
#define Z_EXPR_MAC_13(fn, ...) Z_EXPR_MAC_12(fn, fn(__VA_ARGS__))
#define Z_EXPR_MAC_14(fn, ...) Z_EXPR_MAC_13(fn, fn(__VA_ARGS__))
#define Z_EXPR_MAC_15(fn, ...) Z_EXPR_MAC_14(fn, fn(__VA_ARGS__))
#define Z_EXPR_MAC_16(fn, ...) Z_EXPR_MAC_15(fn, fn(__VA_ARGS__))
#define Z_EXPR_MAC_17(fn, ...) Z_EXPR_MAC_16(fn, fn(__VA_ARGS__))
#define Z_EXPR_MAC_18(fn, ...) Z_EXPR_MAC_17(fn, fn(__VA_ARGS__))
#define Z_EXPR_MAC_19(fn, ...) Z_EXPR_MAC_18(fn, fn(__VA_ARGS__))
#define Z_EXPR_MAC_20(fn, ...) Z_EXPR_MAC_19(fn, fn(__VA_ARGS__))
#define Z_EXPR_MAC_21(fn, ...) Z_EXPR_MAC_20(fn, fn(__VA_ARGS__))
#define Z_EXPR_MAC_22(fn, ...) Z_EXPR_MAC_21(fn, fn(__VA_ARGS__))
#define Z_EXPR_MAC_23(fn, ...) Z_EXPR_MAC_22(fn, fn(__VA_ARGS__))
#define Z_EXPR_MAC_24(fn, ...) Z_EXPR_MAC_23(fn, fn(__VA_ARGS__))
#define Z_EXPR_MAC_25(fn, ...) Z_EXPR_MAC_24(fn, fn(__VA_ARGS__))
#define Z_EXPR_MAC_26(fn, ...) Z_EXPR_MAC_25(fn, fn(__VA_ARGS__))
#define Z_EXPR_MAC_27(fn, ...) Z_EXPR_MAC_26(fn, fn(__VA_ARGS__))
#define Z_EXPR_MAC_28(fn, ...) Z_EXPR_MAC_27(fn, fn(__VA_ARGS__))
#define Z_EXPR_MAC_29(fn, ...) Z_EXPR_MAC_28(fn, fn(__VA_ARGS__))
#define Z_EXPR_MAC_30(fn, ...) Z_EXPR_MAC_29(fn, fn(__VA_ARGS__))
#define Z_EXPR_MAC_31(fn, ...) Z_EXPR_MAC_30(fn, fn(__VA_ARGS__))

/**
 * @brief Mathematical Accumulator for multiplication and division.
 *
 * @param fn Takes the current index, value `v1`, and value `v2` as an
 *           input argument. The macro must return a value that is added
 *           to the accumulator, the next index for the next iteration,
 *           and the values for `v1` and `v2`. `v1` and `v2` may be
 *           changed in `fn`.
 * @param v1, v2 These are integer literals, the values to process in `fn`.
 * @param i_start This is the start index that is given to `fn`.
 *
 * @returns
 * Returns  the result of the performed operation as an integer literal.
 */
#define Z_EXPR_MAC(fn, i_start, ...)                                                               \
	UTIL_CAT(Z_EXPR_MAC_, i_start)(fn, i_start, Z_EXPR_BITS_0U, __VA_ARGS__)

/**
 * Multiplication
 *
 * A bitwise multiplication is fairly simple. In terms of c-code, it is
 * implemented as follows.
 *
 * static uint16_t mult_32bit(uint32_t v1, uint32_t v2)
 * {
 *     uint16_t res = 0;
 *     for (int i = 0; i < 32; i++) {
 *         if (0 == (v1 & (1 << i))) {
 *             continue;
 *         }
 *         res += (v2 << i);
 *     }
 *     return res;
 * }
 *
 * @returns
 * `i`, `add`
 */

#define Z_EXPR_GET_ARG_N(...) GET_ARG_N(__VA_ARGS__)

#define Z_EXPR_MUL_N(N, ...)                                                                       \
	Z_UTIL_DEC(N),                                                                             \
	COND_CODE_1(Z_EXPR_GET_ARG_N(Z_UTIL_INC(N), GET_ARGS_LESS_N(64, __VA_ARGS__)),             \
		    (Z_EXPR_ADD(GET_ARGS_FIRST_N(32, __VA_ARGS__),                                 \
				Z_EXPR_LSH_N(N, Z_EXPR_ARGS_NEXT_32(__VA_ARGS__)))),               \
		    (GET_ARGS_FIRST_N(32, __VA_ARGS__))),                                          \
	GET_ARGS_LESS_N(32, __VA_ARGS__)

#define Z_EXPR_OR_EXPAND(...) Z_EXPR_DO_OR(__VA_ARGS__)

/**
 * Division
 *
 * A bitwise division is a bit more complicated. However, in the essence,
 * it's the same as with the multiplication. Following the c-code equialent.
 *
 * static uint16_t div_12bit(uint16_t v1, uint16_t v2)
 * {
 *     uint32_t res = 0;
 *     for (int i = 31 - count_leading_zeros(v1); i >= 0; i--) {
 *         if ((v1 >> i) < v2) {
 *             continue;
 *         }
 *         v1 -= v2 << i;
 *         res |= 1 << i;
 *     }
 *     return res;
 * }
 *
 * @returns
 * `31 - i`, `partial-quotient (32 args)`, `remainder (32 args)`, `divisor (32 args)`
 */
#define Z_EXPR_DIV_N(N, ...)                                                                       \
	Z_UTIL_DEC(N),                                                                             \
	COND_CODE_1(Z_EXPR_IS_LT(Z_EXPR_RSH_N(N, Z_EXPR_ARGS_NEXT_32(__VA_ARGS__)),                \
						 GET_ARGS_LESS_N(64, __VA_ARGS__)),                \
		    (GET_ARGS_FIRST_N(64, __VA_ARGS__)),                                           \
		    (Z_EXPR_OR_EXPAND(GET_ARGS_FIRST_N(32, __VA_ARGS__),                           \
				      Z_EXPR_LSH_N(N, Z_EXPR_BITS_1U)),                            \
		     Z_EXPR_SUB(Z_EXPR_ARGS_NEXT_32(__VA_ARGS__),                                  \
				Z_EXPR_LSH_N(N, GET_ARGS_LESS_N(64, __VA_ARGS__))))),              \
	GET_ARGS_LESS_N(64, __VA_ARGS__)

#define Z_EXPR_MUL(...)                                                                            \
	Z_EXPR_RESULT(Z_EXPR_MAC(Z_EXPR_MUL_N, Z_EXPR_BIT_LEN(GET_ARGS_LESS_N(32, __VA_ARGS__)),   \
				 GET_ARGS_FIRST_N(32, __VA_ARGS__),                                \
				 REVERSE_ARGS(GET_ARGS_LESS_N(32, __VA_ARGS__))))

#define Z_EXPR_HELPER_SUB_0(x)  x
#define Z_EXPR_HELPER_SUB_1(x)  Z_EXPR_HELPER_SUB_0(Z_UTIL_DEC(x))
#define Z_EXPR_HELPER_SUB_2(x)  Z_EXPR_HELPER_SUB_1(Z_UTIL_DEC(x))
#define Z_EXPR_HELPER_SUB_3(x)  Z_EXPR_HELPER_SUB_2(Z_UTIL_DEC(x))
#define Z_EXPR_HELPER_SUB_4(x)  Z_EXPR_HELPER_SUB_3(Z_UTIL_DEC(x))
#define Z_EXPR_HELPER_SUB_5(x)  Z_EXPR_HELPER_SUB_4(Z_UTIL_DEC(x))
#define Z_EXPR_HELPER_SUB_6(x)  Z_EXPR_HELPER_SUB_5(Z_UTIL_DEC(x))
#define Z_EXPR_HELPER_SUB_7(x)  Z_EXPR_HELPER_SUB_6(Z_UTIL_DEC(x))
#define Z_EXPR_HELPER_SUB_8(x)  Z_EXPR_HELPER_SUB_7(Z_UTIL_DEC(x))
#define Z_EXPR_HELPER_SUB_9(x)  Z_EXPR_HELPER_SUB_8(Z_UTIL_DEC(x))
#define Z_EXPR_HELPER_SUB_10(x) Z_EXPR_HELPER_SUB_9(Z_UTIL_DEC(x))
#define Z_EXPR_HELPER_SUB_11(x) Z_EXPR_HELPER_SUB_10(Z_UTIL_DEC(x))
#define Z_EXPR_HELPER_SUB_12(x) Z_EXPR_HELPER_SUB_11(Z_UTIL_DEC(x))
#define Z_EXPR_HELPER_SUB_13(x) Z_EXPR_HELPER_SUB_12(Z_UTIL_DEC(x))
#define Z_EXPR_HELPER_SUB_14(x) Z_EXPR_HELPER_SUB_13(Z_UTIL_DEC(x))
#define Z_EXPR_HELPER_SUB_15(x) Z_EXPR_HELPER_SUB_14(Z_UTIL_DEC(x))
#define Z_EXPR_HELPER_SUB_16(x) Z_EXPR_HELPER_SUB_15(Z_UTIL_DEC(x))
#define Z_EXPR_HELPER_SUB_17(x) Z_EXPR_HELPER_SUB_16(Z_UTIL_DEC(x))
#define Z_EXPR_HELPER_SUB_18(x) Z_EXPR_HELPER_SUB_17(Z_UTIL_DEC(x))
#define Z_EXPR_HELPER_SUB_19(x) Z_EXPR_HELPER_SUB_18(Z_UTIL_DEC(x))
#define Z_EXPR_HELPER_SUB_20(x) Z_EXPR_HELPER_SUB_19(Z_UTIL_DEC(x))
#define Z_EXPR_HELPER_SUB_21(x) Z_EXPR_HELPER_SUB_20(Z_UTIL_DEC(x))
#define Z_EXPR_HELPER_SUB_22(x) Z_EXPR_HELPER_SUB_21(Z_UTIL_DEC(x))
#define Z_EXPR_HELPER_SUB_23(x) Z_EXPR_HELPER_SUB_22(Z_UTIL_DEC(x))
#define Z_EXPR_HELPER_SUB_24(x) Z_EXPR_HELPER_SUB_23(Z_UTIL_DEC(x))
#define Z_EXPR_HELPER_SUB_25(x) Z_EXPR_HELPER_SUB_24(Z_UTIL_DEC(x))
#define Z_EXPR_HELPER_SUB_26(x) Z_EXPR_HELPER_SUB_25(Z_UTIL_DEC(x))
#define Z_EXPR_HELPER_SUB_27(x) Z_EXPR_HELPER_SUB_26(Z_UTIL_DEC(x))
#define Z_EXPR_HELPER_SUB_28(x) Z_EXPR_HELPER_SUB_27(Z_UTIL_DEC(x))
#define Z_EXPR_HELPER_SUB_29(x) Z_EXPR_HELPER_SUB_28(Z_UTIL_DEC(x))
#define Z_EXPR_HELPER_SUB_30(x) Z_EXPR_HELPER_SUB_29(Z_UTIL_DEC(x))
#define Z_EXPR_HELPER_SUB_31(x) Z_EXPR_HELPER_SUB_30(Z_UTIL_DEC(x))

#define Z_EXPR_HELPER_SUB(x, y) UTIL_CAT(Z_EXPR_HELPER_SUB_, y)(x)
#define Z_EXPR_HELPER_MAX(x, y) COND_CODE_1(Z_EXPR_IS_0(Z_EXPR_HELPER_SUB(x, y)), (y), (x))

#define Z_EXPR_DIV_START_DIGIT(...)                                                                \
	Z_EXPR_HELPER_MAX(Z_EXPR_BIT_LEN(GET_ARGS_FIRST_N(32, __VA_ARGS__)),                       \
			  Z_EXPR_BIT_LEN(Z_EXPR_ARGS_NEXT_32(__VA_ARGS__)))

#define Z_EXPR_DO_DIVMOD(...)                                                                      \
	Z_EXPR_MAC(Z_EXPR_DIV_N, Z_EXPR_DIV_START_DIGIT(__VA_ARGS__), __VA_ARGS__)

#define Z_EXPR_MODULO(...) GET_ARGS_FIRST_N(32, GET_ARGS_LESS_N(33, __VA_ARGS__))
#define Z_EXPR_DIVMOD(...) GET_ARGS_LESS_N(1, Z_EXPR_DO_DIVMOD(__VA_ARGS__))

/**
 * INTERNAL_HIDDEN @endcond
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_UTIL_INTERNAL_UTIL_EXPR_H_ */
