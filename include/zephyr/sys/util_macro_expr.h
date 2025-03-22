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
#include <zephyr/sys/util_expr_bits.h>

/**
 * @brief Expression macros
 *
 * @defgroup expr-macros EXPR macros
 * @ingroup sys-util
 * @{
 */

/**
 * @brief Performs decimal addition.
 *
 * EXPR_ADD_DEC(...) calculates the addition of two decimal values.
 *
 * Each value should be convert with EXPR_BITS macro.
 * It means, the corresponding `Z_EXPR_BITS_[n]` must be defined.
 *
 * @return The resulting decimal literal. Since a LUT is used for the conversion,
 *         the corresponding `Z_EXPR_BN_TO_DEC_[binary_form_of_n]` must be defined.
 */
#define EXPR_ADD_DEC(...)                                                                          \
	Z_EXPR_BITS_TO_DEC(EXPR_ADD(EXPR_BITS(GET_ARG_N(1, __VA_ARGS__)),                          \
				    EXPR_BITS(GET_ARG_N(2, __VA_ARGS__))))

/**
 * @brief Performs decimal addition.
 *
 * EXPR_ADD_DEC(...) calculates the addition of two decimal values.
 *
 * Each value should be convert with EXPR_BITS macro.
 * It means, the corresponding `Z_EXPR_BITS_[n]` must be defined.
 *
 * @return The resulting decimal literal. Since a LUT is not used for the conversion,
 *         the calculation takes time.
 */
#define EXPR_ADD_BIGDEC(...)                                                                       \
	Z_EXPR_ENCODE_DEC(EXPR_ADD(EXPR_BITS(GET_ARG_N(1, __VA_ARGS__)),                           \
				   EXPR_BITS(GET_ARG_N(2, __VA_ARGS__))))

/**
 * @brief Perform a - b
 *
 * @see EXPR_ADD_DEC(...)
 */
#define EXPR_SUB_DEC(...)                                                                          \
	Z_EXPR_BITS_TO_DEC(EXPR_SUB(EXPR_BITS(GET_ARG_N(1, __VA_ARGS__)),                          \
				    EXPR_BITS(GET_ARG_N(2, __VA_ARGS__))))

/**
 * @brief Perform a - b
 *
 * @see EXPR_ADD_BIGDEC(...)
 */
#define EXPR_SUB_BIGDEC(...)                                                                       \
	Z_EXPR_ENCODE_DEC(EXPR_SUB(EXPR_BITS(GET_ARG_N(1, __VA_ARGS__)),                           \
				   EXPR_BITS(GET_ARG_N(2, __VA_ARGS__))))

/**
 * @brief Perform a * b
 *
 * @see EXPR_ADD_DEC(...)
 */
#define EXPR_MUL_DEC(...)                                                                          \
	Z_EXPR_BITS_TO_DEC(EXPR_MUL(EXPR_BITS(GET_ARG_N(1, __VA_ARGS__)),                          \
				    EXPR_BITS(GET_ARG_N(2, __VA_ARGS__))))

/**
 * @brief Perform a * b
 *
 * @see EXPR_ADD_BIGDEC(...)
 */
#define EXPR_MUL_BIGDEC(...)                                                                       \
	Z_EXPR_ENCODE_DEC(EXPR_MUL(EXPR_BITS(GET_ARG_N(1, __VA_ARGS__)),                           \
				   EXPR_BITS(GET_ARG_N(2, __VA_ARGS__))))

/**
 * @brief Perform a / b
 *
 * @see EXPR_ADD_DEC(...)
 */
#define EXPR_DIV_DEC(...)                                                                          \
	Z_EXPR_BITS_TO_DEC(EXPR_DIV(EXPR_BITS(GET_ARG_N(1, __VA_ARGS__)),                          \
				    EXPR_BITS(GET_ARG_N(2, __VA_ARGS__))))

/**
 * @brief Perform a / b
 *
 * @see EXPR_ADD_BIGDEC(...)
 */
#define EXPR_DIV_BIGDEC(...)                                                                       \
	Z_EXPR_ENCODE_DEC(EXPR_DIV(EXPR_BITS(GET_ARG_N(1, __VA_ARGS__)),                           \
				   EXPR_BITS(GET_ARG_N(2, __VA_ARGS__))))

/**
 * @brief Perform a % b
 *
 * @see EXPR_ADD_DEC(...)
 */
#define EXPR_MOD_DEC(...)                                                                          \
	Z_EXPR_BITS_TO_DEC(EXPR_MOD(EXPR_BITS(GET_ARG_N(1, __VA_ARGS__)),                          \
				    EXPR_BITS(GET_ARG_N(2, __VA_ARGS__))))

/**
 * @brief Perform a % b
 *
 * @see EXPR_ADD_BIGDEC(...)
 */
#define EXPR_MOD_BIGDEC(...)                                                                       \
	Z_EXPR_ENCODE_DEC(EXPR_MOD(EXPR_BITS(GET_ARG_N(1, __VA_ARGS__)),                           \
				   EXPR_BITS(GET_ARG_N(2, __VA_ARGS__))))

/**
 * @brief Perform a << b
 *
 * @see EXPR_ADD_DEC(...)
 */
#define EXPR_LSH_DEC(...)                                                                          \
	Z_EXPR_BITS_TO_DEC(EXPR_LSH(EXPR_BITS(GET_ARG_N(1, __VA_ARGS__)),                          \
				    EXPR_BITS(GET_ARG_N(2, __VA_ARGS__))))

/**
 * @brief Perform a << b
 *
 * @see EXPR_ADD_BIGDEC(...)
 */
#define EXPR_LSH_BIGDEC(...)                                                                       \
	Z_EXPR_ENCODE_DEC(EXPR_LSH(EXPR_BITS(GET_ARG_N(1, __VA_ARGS__)),                           \
				   EXPR_BITS(GET_ARG_N(2, __VA_ARGS__))))

/**
 * @brief Perform a >> b
 *
 * @see EXPR_ADD_DEC(...)
 */
#define EXPR_RSH_DEC(...)                                                                          \
	Z_EXPR_BITS_TO_DEC(EXPR_RSH(EXPR_BITS(GET_ARG_N(1, __VA_ARGS__)),                          \
				    EXPR_BITS(GET_ARG_N(2, __VA_ARGS__))))

/**
 * @brief Perform a >> b
 *
 * @see EXPR_ADD_BIGDEC(...)
 */
#define EXPR_RSH_BIGDEC(...)                                                                       \
	Z_EXPR_ENCODE_DEC(EXPR_RSH(EXPR_BITS(GET_ARG_N(1, __VA_ARGS__)),                           \
				   EXPR_BITS(GET_ARG_N(2, __VA_ARGS__))))

/**
 * @brief Perform a & b
 *
 * @see EXPR_ADD_DEC(...)
 */
#define EXPR_AND_DEC(...)                                                                          \
	Z_EXPR_BITS_TO_DEC(EXPR_AND(EXPR_BITS(GET_ARG_N(1, __VA_ARGS__)),                          \
				    EXPR_BITS(GET_ARG_N(2, __VA_ARGS__))))

/**
 * @brief Perform a & b
 *
 * @see EXPR_ADD_BIGDEC(...)
 */
#define EXPR_AND_BIGDEC(...)                                                                       \
	Z_EXPR_ENCODE_DEC(EXPR_AND(EXPR_BITS(GET_ARG_N(1, __VA_ARGS__)),                           \
				   EXPR_BITS(GET_ARG_N(2, __VA_ARGS__))))

/**
 * @brief Perform a | b
 *
 * @see EXPR_ADD_DEC(...)
 */
#define EXPR_OR_DEC(...)                                                                           \
	Z_EXPR_BITS_TO_DEC(EXPR_OR(EXPR_BITS(GET_ARG_N(1, __VA_ARGS__)),                           \
				   EXPR_BITS(GET_ARG_N(2, __VA_ARGS__))))

/**
 * @brief Perform a | b
 *
 * @see EXPR_ADD_BIGDEC(...)
 */
#define EXPR_OR_BIGDEC(...)                                                                        \
	Z_EXPR_ENCODE_DEC(EXPR_OR(EXPR_BITS(GET_ARG_N(1, __VA_ARGS__)),                            \
				  EXPR_BITS(GET_ARG_N(2, __VA_ARGS__))))

/**
 * @brief Perform a ^ b
 *
 * @see EXPR_ADD_DEC(...)
 */
#define EXPR_XOR_DEC(...)                                                                          \
	Z_EXPR_BITS_TO_DEC(EXPR_XOR(EXPR_BITS(GET_ARG_N(1, __VA_ARGS__)),                          \
				    EXPR_BITS(GET_ARG_N(2, __VA_ARGS__))))

/**
 * @brief Perform a ^ b
 *
 * @see EXPR_ADD_BIGDEC(...)
 */
#define EXPR_XOR_BIGDEC(...)                                                                       \
	Z_EXPR_ENCODE_DEC(EXPR_XOR(EXPR_BITS(GET_ARG_N(1, __VA_ARGS__)),                           \
				   EXPR_BITS(GET_ARG_N(2, __VA_ARGS__))))

/**
 * @brief Perform ~a
 *
 * @see EXPR_ADD_DEC(...)
 */
#define EXPR_NOT_DEC(...) Z_EXPR_BITS_TO_DEC(EXPR_NOT(EXPR_BITS(GET_ARG_N(1, __VA_ARGS__))))

/**
 * @brief Perform ~a
 *
 * @see EXPR_ADD_BIGDEC(...)
 */
#define EXPR_NOT_BIGDEC(...) Z_EXPR_ENCODE_DEC(EXPR_NOT(EXPR_BITS(GET_ARG_N(1, __VA_ARGS__))))

/**
 * @brief Like <tt>a == b</tt>, but does evaluation and
 * short-circuiting at C preprocessor time.
 *
 * Each value should be convert with EXPR_BITS macro.
 * It means, the corresponding Z_EXPR_BITS_[n]U must be defined.
 *
 * Examples:
 * @code
 *   IS_EQ(1, 1)   // 1
 *   IS_EQ(1U, 1U) // 1
 *   IS_EQ(1U, 1)  // 1
 *   IS_EQ(1, 1U)  // 1
 *   IS_EQ(1, 0)   // 0
 * @endcode
 * @param ... Two integer literals (can be with U suffix)
 */
#define IS_EQ(...)                                                                                 \
	EXPR_IS_EQ(EXPR_BITS(GET_ARG_N(1, __VA_ARGS__)), EXPR_BITS(GET_ARG_N(2, __VA_ARGS__)))

/**
 * @brief Calculate <tt>a < b</tt> at preprocessing.
 * @see IS_EQ(...)
 */
#define IS_LT(...)                                                                                 \
	EXPR_IS_LT(EXPR_BITS(GET_ARG_N(1, __VA_ARGS__)), EXPR_BITS(GET_ARG_N(2, __VA_ARGS__)))

/**
 * @brief Calculate <tt>a > b</tt> at preprocessing.
 * @see IS_EQ(...)
 */
#define IS_GT(...)                                                                                 \
	EXPR_IS_GT(EXPR_BITS(GET_ARG_N(1, __VA_ARGS__)), EXPR_BITS(GET_ARG_N(2, __VA_ARGS__)))

/**
 * @brief Calculate <tt>a <= b</tt> at preprocessing.
 * @see IS_EQ(...)
 */
#define IS_LE(...)                                                                                 \
	EXPR_IS_LE(EXPR_BITS(GET_ARG_N(1, __VA_ARGS__)), EXPR_BITS(GET_ARG_N(2, __VA_ARGS__)))

/**
 * @brief Calculate <tt>a >= b</tt> at preprocessing.
 * @see IS_EQ(...)
 */
#define IS_GE(...)                                                                                 \
	EXPR_IS_GE(EXPR_BITS(GET_ARG_N(1, __VA_ARGS__)), EXPR_BITS(GET_ARG_N(2, __VA_ARGS__)))

/**
 * @brief Calculates the incremented value.
 *
 * If the given value is 4294967295 (0xFFFFFFFF), no increment will be performed.
 *
 * Examples:
 * @code
 *   UTIL_INC(3)   // 4
 * @endcode
 */
#define UTIL_INC(...)                                                                              \
	COND_CODE_1(EXPR_IS_EQ(EXPR_BITS(__VA_ARGS__), EXPR_BITS_0X(f, f, f, f, f, f, f, f)),      \
		    (4294967295), (EXPR_ADD_DEC(__VA_ARGS__, 1)))

#define UTIL_INC_BIGDEC(...)                                                                       \
	COND_CODE_1(EXPR_IS_EQ(EXPR_BITS(__VA_ARGS__), EXPR_BITS_0X(f, f, f, f, f, f, f, f)),      \
		    (4294967295), (EXPR_ADD_BIGDEC(__VA_ARGS__, 1)))

/**
 * @brief Calculates the decremented value.
 *
 * If the given value is 0, no derement will be performed.
 *
 * Examples:
 * @code
 *   UTIL_DEC(3)   // 2
 * @endcode
 */
#define UTIL_DEC(...)                                                                              \
	COND_CODE_1(EXPR_IS_EQ(EXPR_BITS(__VA_ARGS__), EXPR_BITS_0X(0)),                           \
		    (0), (EXPR_SUB_DEC(__VA_ARGS__, 1)))

#define UTIL_BIGDEC(...)                                                                           \
	COND_CODE_1(EXPR_IS_EQ(EXPR_BITS(__VA_ARGS__), EXPR_BITS_0X(0)),                           \
		    (0), (EXPR_SUB_BIGDEC(__VA_ARGS__, 1)))

/**
 * @brief Calculate the doubled value
 *
 * Examples:
 *
 * @code
 *   UTIL_X2(3)   // 6
 * @endcode
 */
#define UTIL_X2(...) Z_EXPR_BITS_TO_DEC(Z_EXPR_LSH_1(EXPR_BITS(__VA_ARGS__)))

#define UTIL_X2_BIGDEC(...) Z_EXPR_ENCODE_DEC(Z_EXPR_LSH_1(EXPR_BITS(__VA_ARGS__)))
/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_UTIL_MACRO_EXPR_H_ */
