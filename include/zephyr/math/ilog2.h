/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_MATH_ILOG2_H_
#define ZEPHYR_INCLUDE_MATH_ILOG2_H_

#include <stdint.h>

#include <zephyr/toolchain.h>
#include <zephyr/arch/common/ffs.h>
#include <zephyr/sys/util.h>

/**
 * @file
 * @brief Provide ilog2() function
 */

/**
 *
 * @brief Calculate the floor of log2 for compile time constant
 *
 * This calculates the floor of log2 (integer log2) for 32-bit
 * unsigned integer.
 *
 * @note This should only be used for compile time constant
 *       when value is known during preprocessing stage.
 *       DO NOT USE for runtime code due to the big tree of
 *       nested if-else blocks.
 *
 * @warning Will return 0 if input value is 0, which is
 *          invalid for log2.
 *
 * @param n Input value
 * @return Integer log2 of @n
 */
#define ilog2_compile_time_const_u32(n)			\
	(						\
		(n < 2) ? 0 :				\
		((n & BIT(31)) == BIT(31)) ? 31 :	\
		((n & BIT(30)) == BIT(30)) ? 30 :	\
		((n & BIT(29)) == BIT(29)) ? 29 :	\
		((n & BIT(28)) == BIT(28)) ? 28 :	\
		((n & BIT(27)) == BIT(27)) ? 27 :	\
		((n & BIT(26)) == BIT(26)) ? 26 :	\
		((n & BIT(25)) == BIT(25)) ? 25 :	\
		((n & BIT(24)) == BIT(24)) ? 24 :	\
		((n & BIT(23)) == BIT(23)) ? 23 :	\
		((n & BIT(22)) == BIT(22)) ? 22 :	\
		((n & BIT(21)) == BIT(21)) ? 21 :	\
		((n & BIT(20)) == BIT(20)) ? 20 :	\
		((n & BIT(19)) == BIT(19)) ? 19 :	\
		((n & BIT(18)) == BIT(18)) ? 18 :	\
		((n & BIT(17)) == BIT(17)) ? 17 :	\
		((n & BIT(16)) == BIT(16)) ? 16 :	\
		((n & BIT(15)) == BIT(15)) ? 15 :	\
		((n & BIT(14)) == BIT(14)) ? 14 :	\
		((n & BIT(13)) == BIT(13)) ? 13 :	\
		((n & BIT(12)) == BIT(12)) ? 12 :	\
		((n & BIT(11)) == BIT(11)) ? 11 :	\
		((n & BIT(10)) == BIT(10)) ? 10 :	\
		((n & BIT(9)) == BIT(9)) ? 9 :		\
		((n & BIT(8)) == BIT(8)) ? 8 :		\
		((n & BIT(7)) == BIT(7)) ? 7 :		\
		((n & BIT(6)) == BIT(6)) ? 6 :		\
		((n & BIT(5)) == BIT(5)) ? 5 :		\
		((n & BIT(4)) == BIT(4)) ? 4 :		\
		((n & BIT(3)) == BIT(3)) ? 3 :		\
		((n & BIT(2)) == BIT(2)) ? 2 :		\
		1					\
	)

/**
 *
 * @brief Calculate integer log2
 *
 * This calculates the floor of log2 (integer of log2).
 *
 * @warning Will return 0 if input value is 0, which is
 *          invalid for log2.
 *
 * @param n Input value
 * @return Integer log2 of @p n
 */
/*
 * This is in #define form as this needs to also work on
 * compile time constants. Doing this as a static inline
 * function will result in compiler complaining with
 * "initializer element is not constant".
 */
#define ilog2(n)					\
	(						\
		__builtin_constant_p(n) ?		\
		ilog2_compile_time_const_u32(n) :	\
		find_msb_set(n) - 1			\
	)

#endif /* ZEPHYR_INCLUDE_MATH_ILOG2_H_ */
