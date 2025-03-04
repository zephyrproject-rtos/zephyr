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
#define UTIL_ADD(a, b)  EXPR_TO_NUM(EXPR_ADD(EXPR_BITS(a), EXPR_BITS(b)))

#define UTIL_DO_SUB(...)                                                                           \
	EXPR_TO_NUM(COND_CODE_1(GET_ARG_N(1, __VA_ARGS__), \
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
#define UTIL_X2(x)     EXPR_TO_NUM(Z_EXPR_LSH_1(EXPR_BITS(x)))

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_UTIL_MACRO_EXPR_H_ */
