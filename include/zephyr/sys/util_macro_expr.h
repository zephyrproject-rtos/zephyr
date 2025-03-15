/*
 * Copyright (c) 2025 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_UTIL_MACRO_EXPR_H_
#define ZEPHYR_INCLUDE_SYS_UTIL_MACRO_EXPR_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/sys/util_expr.h>
#include <zephyr/sys/util_expr_nums.h>

#define UTIL_TRIM_LZERO_IS_ZERO_0 1

#define UTIL_TRIM_LZERO_1(x, ...) \
	COND_CODE_1(UTIL_CAT(UTIL_TRIM_LZERO_IS_ZERO_, x), (0), (x))
#define UTIL_TRIM_LZERO_2(x, ...) \
	COND_CODE_1(UTIL_CAT(UTIL_TRIM_LZERO_IS_ZERO_, x), (UTIL_TRIM_LZERO_1(__VA_ARGS__)), (_CONCAT_2(x, __VA_ARGS__)))
#define UTIL_TRIM_LZERO_3(x, ...) \
	COND_CODE_1(UTIL_CAT(UTIL_TRIM_LZERO_IS_ZERO_, x), (UTIL_TRIM_LZERO_2(__VA_ARGS__)), (_CONCAT_3(x, __VA_ARGS__)))
#define UTIL_TRIM_LZERO_4(x, ...) \
	COND_CODE_1(UTIL_CAT(UTIL_TRIM_LZERO_IS_ZERO_, x), (UTIL_TRIM_LZERO_3(__VA_ARGS__)), (_CONCAT_4(x, __VA_ARGS__)))
#define UTIL_TRIM_LZERO_5(x, ...) \
	COND_CODE_1(UTIL_CAT(UTIL_TRIM_LZERO_IS_ZERO_, x), (UTIL_TRIM_LZERO_4(__VA_ARGS__)), (_CONCAT_5(x, __VA_ARGS__)))
#define UTIL_TRIM_LZERO_6(x, ...) \
	COND_CODE_1(UTIL_CAT(UTIL_TRIM_LZERO_IS_ZERO_, x), (UTIL_TRIM_LZERO_5(__VA_ARGS__)), (_CONCAT_6(x, __VA_ARGS__)))
#define UTIL_TRIM_LZERO_7(x, ...) \
	COND_CODE_1(UTIL_CAT(UTIL_TRIM_LZERO_IS_ZERO_, x), (UTIL_TRIM_LZERO_6(__VA_ARGS__)), (_CONCAT_7(x, __VA_ARGS__)))
#define UTIL_TRIM_LZERO_8(x, ...) \
	COND_CODE_1(UTIL_CAT(UTIL_TRIM_LZERO_IS_ZERO_, x), (UTIL_TRIM_LZERO_7(__VA_ARGS__)), (_CONCAT_8(x, __VA_ARGS__)))
#define UTIL_TRIM_LZERO_9(x, ...) \
	COND_CODE_1(UTIL_CAT(UTIL_TRIM_LZERO_IS_ZERO_, x), (UTIL_TRIM_LZERO_8(__VA_ARGS__)), (_CONCAT_9(x, __VA_ARGS__)))
#define UTIL_TRIM_LZERO_10(x, ...) \
	COND_CODE_1(UTIL_CAT(UTIL_TRIM_LZERO_IS_ZERO_, x), (UTIL_TRIM_LZERO_9(__VA_ARGS__)), (_CONCAT_10(x, __VA_ARGS__)))
#define UTIL_TRIM_LZERO_11(x, ...) \
	COND_CODE_1(UTIL_CAT(UTIL_TRIM_LZERO_IS_ZERO_, x), (UTIL_TRIM_LZERO_10(__VA_ARGS__)), (_CONCAT_11(x, __VA_ARGS__)))
#define UTIL_TRIM_LZERO(...) \
	UTIL_CAT(UTIL_TRIM_LZERO_, NUM_VA_ARGS(__VA_ARGS__))(__VA_ARGS__)

#define Z_EXPR_BITS_10         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0
#define Z_EXPR_BITS_100        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0
#define Z_EXPR_BITS_1000       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 1, 0, 0, 0
#define Z_EXPR_BITS_10000      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0
#define Z_EXPR_BITS_100000     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0
#define Z_EXPR_BITS_1000000    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0
#define Z_EXPR_BITS_10000000   0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0
#define Z_EXPR_BITS_100000000  0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0
#define Z_EXPR_BITS_1000000000 0, 0, 1, 1, 1, 0, 1, 1, 1, 0, 0, 1, 1, 0, 1, 0, 1, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0

#define UTIL_TO_DEC_0(...) Z_EXPR_BITS_TO_DEC(GET_ARGS_FIRST_N(32, __VA_ARGS__)), Z_EXPR_BITS_TO_DEC(GET_ARGS_FIRST_N(32, GET_ARGS_LESS_N(32, __VA_ARGS__)))
#define UTIL_TO_DEC_1(...) Z_EXPR_BITS_TO_DEC(GET_ARGS_FIRST_N(32, __VA_ARGS__)), UTIL_TO_DEC_0(Z_EXPR_DIVMOD(GET_ARGS_FIRST_N(32, GET_ARGS_LESS_N(32, __VA_ARGS__)), Z_EXPR_BITS_10))
#define UTIL_TO_DEC_2(...) Z_EXPR_BITS_TO_DEC(GET_ARGS_FIRST_N(32, __VA_ARGS__)), UTIL_TO_DEC_1(Z_EXPR_DIVMOD(GET_ARGS_FIRST_N(32, GET_ARGS_LESS_N(32, __VA_ARGS__)), Z_EXPR_BITS_100))
#define UTIL_TO_DEC_3(...) Z_EXPR_BITS_TO_DEC(GET_ARGS_FIRST_N(32, __VA_ARGS__)), UTIL_TO_DEC_2(Z_EXPR_DIVMOD(GET_ARGS_FIRST_N(32, GET_ARGS_LESS_N(32, __VA_ARGS__)), Z_EXPR_BITS_1000))
#define UTIL_TO_DEC_4(...) Z_EXPR_BITS_TO_DEC(GET_ARGS_FIRST_N(32, __VA_ARGS__)), UTIL_TO_DEC_3(Z_EXPR_DIVMOD(GET_ARGS_FIRST_N(32, GET_ARGS_LESS_N(32, __VA_ARGS__)), Z_EXPR_BITS_10000))
#define UTIL_TO_DEC_5(...) Z_EXPR_BITS_TO_DEC(GET_ARGS_FIRST_N(32, __VA_ARGS__)), UTIL_TO_DEC_4(Z_EXPR_DIVMOD(GET_ARGS_FIRST_N(32, GET_ARGS_LESS_N(32, __VA_ARGS__)), Z_EXPR_BITS_100000))
#define UTIL_TO_DEC_6(...) Z_EXPR_BITS_TO_DEC(GET_ARGS_FIRST_N(32, __VA_ARGS__)), UTIL_TO_DEC_5(Z_EXPR_DIVMOD(GET_ARGS_FIRST_N(32, GET_ARGS_LESS_N(32, __VA_ARGS__)), Z_EXPR_BITS_1000000))
#define UTIL_TO_DEC_7(...) Z_EXPR_BITS_TO_DEC(GET_ARGS_FIRST_N(32, __VA_ARGS__)), UTIL_TO_DEC_6(Z_EXPR_DIVMOD(GET_ARGS_FIRST_N(32, GET_ARGS_LESS_N(32, __VA_ARGS__)), Z_EXPR_BITS_10000000))
#define UTIL_TO_DEC_8(...) Z_EXPR_BITS_TO_DEC(GET_ARGS_FIRST_N(32, __VA_ARGS__)), UTIL_TO_DEC_7(Z_EXPR_DIVMOD(GET_ARGS_FIRST_N(32, GET_ARGS_LESS_N(32, __VA_ARGS__)), Z_EXPR_BITS_100000000))
#define UTIL_TO_DEC_9(...) Z_EXPR_BITS_TO_DEC(GET_ARGS_FIRST_N(32, __VA_ARGS__)), UTIL_TO_DEC_8(Z_EXPR_DIVMOD(GET_ARGS_FIRST_N(32, GET_ARGS_LESS_N(32, __VA_ARGS__)), Z_EXPR_BITS_1000000000))
#define UTIL_TO_DEC(...) UTIL_TRIM_LZERO(UTIL_TO_DEC_9(EXPR_BITS_0X(0), __VA_ARGS__))


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
#define IS_EQ(a, b) EXPR_IS_EQ(EXPR_BITS(a), EXPR_BITS(b))

/**
 * @brief UTIL_ADD(x, n) executes n-times UTIL_INC on x to create the value x + n.
 *        Both the arguments and the result of the operation must be in the range
 *        of 0 to 4095.
 */
#define UTIL_ADD(a, b)  UTIL_TO_DEC(EXPR_ADD(EXPR_BITS(a), EXPR_BITS(b)))

#define UTIL_DO_SUB(...)                                                                           \
	UTIL_TO_DEC(COND_CODE_1(GET_ARG_N(1, __VA_ARGS__),                                         \
			        (Z_EXPR_BITS_0),                                                   \
				(GET_ARGS_FIRST_N(32, __VA_ARGS__))))

/**
 * @brief UTIL_SUB(x, n) executes n-times UTIL_DEC on x to create the value x - n.
 *        Both the arguments and the result of the operation must be in the range
 *        of 0 to 4095.
 */
#define UTIL_SUB(a, b) UTIL_DO_SUB(EXPR_SUB(EXPR_BITS(a), EXPR_BITS(b)), EXPR_BITS(a))

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
 * @brief UTIL_X2(y) for an integer literal y from 0 to 4095 expands to an
 * integer literal whose value is 2y.
 */
#define UTIL_X2(x)     UTIL_TO_DEC(Z_EXPR_LSH_1(EXPR_BITS(x)))

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_UTIL_MACRO_EXPR_H_ */
