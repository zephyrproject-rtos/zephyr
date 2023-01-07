/*
 * Copyright (c) 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Next-Highest Power-of-Two Utilities
 *
 * Macros and static inline functions for dealing with next-highest power-of-two (NHPOT)
 * calculatinos.
 */

#ifndef ZEPHYR_INCLUDE_SYS_NHPOT_H_
#define ZEPHYR_INCLUDE_SYS_NHPOT_H_

#include <stdint.h>

#include <zephyr/sys/util_macro.h>

#if defined(__cplusplus) && __cplusplus >= 201402L
#define NHPOT_CONSTEXPR constexpr
#else
#define NHPOT_CONSTEXPR
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ingroup sys-util
 * @{
 */

/**
 * @brief Get the next highest 32-bit power of two for @p val
 *
 * This macro rounds-up @p val to the next-highest power of two, as opposed to @ref ROUND_UP which
 * rounds up a certain value to the next multiple of some alignment.
 *
 * For example,
 * ```
 * NHPOT(0) => 1
 * NHPOT(1) => 1
 * NHPOT(2) => 2
 * NHPOT(3) => 4
 * NHPOT(4) => 4
 * NHPOT(5) => 8
 * ...
 * NHPOT(4294967295UL) => 0
 * ```
 *
 * @note This macro should only be used for compile-time constant expressions. The runtime
 * equivalent of this is @ref nhpot.
 *
 * @param val the value for which to calculate the next highest power of two
 *
 * @retval the next highest power of two for @p val
 * @retval zero, if the next higher power of two for @p val would saturate 32-bits
 *
 * @see See @ref NHPOT64 for the 64-bit variant.
 *
 */
/* clang-format off */
#define NHPOT(val) \
	((val) <= BIT(0) ? BIT(0) : ((val) <= BIT(1) ? BIT(1) : ((val) <= BIT(2) ? BIT(2) : \
	((val) <= BIT(3) ? BIT(3) : ((val) <= BIT(4) ? BIT(4) : ((val) <= BIT(5) ? BIT(5) : \
	((val) <= BIT(6) ? BIT(6) : ((val) <= BIT(7) ? BIT(7) : ((val) <= BIT(8) ? BIT(8) : \
	((val) <= BIT(9) ? BIT(9) : ((val) <= BIT(10) ? BIT(10) : ((val) <= BIT(11) ? BIT(11) : \
	((val) <= BIT(12) ? BIT(12) : ((val) <= BIT(13) ? BIT(13) : ((val) <= BIT(14) ? BIT(14) : \
	((val) <= BIT(15) ? BIT(15) : ((val) <= BIT(16) ? BIT(16) : ((val) <= BIT(17) ? BIT(17) : \
	((val) <= BIT(18) ? BIT(18) : ((val) <= BIT(19) ? BIT(19) : ((val) <= BIT(20) ? BIT(20) : \
	((val) <= BIT(21) ? BIT(21) : ((val) <= BIT(22) ? BIT(22) : ((val) <= BIT(23) ? BIT(23) : \
	((val) <= BIT(24) ? BIT(24) : ((val) <= BIT(25) ? BIT(25) : ((val) <= BIT(26) ? BIT(26) : \
	((val) <= BIT(27) ? BIT(27) : ((val) <= BIT(28) ? BIT(28) : ((val) <= BIT(29) ? BIT(29) : \
	((val) <= BIT(30) ? BIT(30) : ((val) <= BIT(31) ? BIT(31) : \
	0))))))))))))))))))))))))))))))))
/* clang-format on */

/**
 * @brief Get the next highest 64-bit power of two for @p val
 *
 * This macro rounds-up @p val to the next-highest power of two, as opposed to @ref ROUND_UP which
 * rounds up a certain value to the next multiple of some alignment.
 *
 * For example,
 * ```
 * NHPOT64(0) => 1
 * NHPOT64(1) => 1
 * NHPOT64(2) => 2
 * NHPOT64(3) => 4
 * NHPOT64(4) => 4
 * NHPOT64(5) => 8
 * ...
 * NHPOT(0xffffffffffffffffULL) => 0
 * ```
 *
 * @note This macro should only be used for compile-time constant expressions. The runtime
 * equivalent of this is @ref nhpot64.
 *
 * @param val the value for which to calculate the next highest power of two
 *
 * @retval the next highest power of two for @p val
 * @retval zero, if the next higher power of two for @p val would saturate 64-bits
 *
 * @see See @ref NHPOT for the 32-bit variant.
 */
/* clang-format off */
#define NHPOT64(val) \
	((NHPOT(val) != 0) ? NHPOT(val) : \
	((val) <= BIT64(32) ? BIT64(32) : ((val) <= BIT64(33) ? BIT64(33) : \
	((val) <= BIT64(34) ? BIT64(34) : ((val) <= BIT64(35) ? BIT64(35) : \
	((val) <= BIT64(36) ? BIT64(36) : ((val) <= BIT64(37) ? BIT64(37) : \
	((val) <= BIT64(38) ? BIT64(38) : ((val) <= BIT64(39) ? BIT64(39) : \
	((val) <= BIT64(40) ? BIT64(40) : ((val) <= BIT64(41) ? BIT64(41) : \
	((val) <= BIT64(42) ? BIT64(42) : ((val) <= BIT64(43) ? BIT64(43) : \
	((val) <= BIT64(44) ? BIT64(44) : ((val) <= BIT64(45) ? BIT64(45) : \
	((val) <= BIT64(46) ? BIT64(46) : ((val) <= BIT64(47) ? BIT64(47) : \
	((val) <= BIT64(48) ? BIT64(48) : ((val) <= BIT64(49) ? BIT64(49) : \
	((val) <= BIT64(50) ? BIT64(50) : ((val) <= BIT64(51) ? BIT64(51) : \
	((val) <= BIT64(52) ? BIT64(52) : ((val) <= BIT64(53) ? BIT64(53) : \
	((val) <= BIT64(54) ? BIT64(54) : ((val) <= BIT64(55) ? BIT64(55) : \
	((val) <= BIT64(56) ? BIT64(56) : ((val) <= BIT64(57) ? BIT64(57) : \
	((val) <= BIT64(58) ? BIT64(58) : ((val) <= BIT64(59) ? BIT64(59) : \
	((val) <= BIT64(60) ? BIT64(60) : ((val) <= BIT64(61) ? BIT64(61) : \
	((val) <= BIT64(62) ? BIT64(62) : ((val) <= BIT64(63) ? BIT64(63) : 0 \
	)))))))))))))))))))))))))))))))))
/* clang-format on */

/**
 * @brief Get the next highest 32-bit power of two for @p val
 *
 * This macro rounds-up @p val to the next-highest power of two, as opposed to @ref ROUND_UP which
 * rounds up a certain value to the next multiple of some alignment.
 *
 * @note This function should be used with runtime C or C++ expressions and compile-time
 * constant C++ expressions.
 *
 * @param val the value for which to calculate the next highest power of two
 *
 * @retval the next highest power of two for @p val
 * @retval zero, if the next higher power of two for @p val would saturate 32-bits
 *
 * @see See @ref NHPOT for a compile-time constant C macro
 * @see See @ref nhpot64 for the 64-bit variant of this function
 */
static NHPOT_CONSTEXPR inline uint32_t nhpot(uint32_t val)
{
	uint32_t val2 = val;

	val2--;
	val2 |= val2 >> 1;
	val2 |= val2 >> 2;
	val2 |= val2 >> 4;
	val2 |= val2 >> 8;
	val2 |= val2 >> 16;
	val2++;

	return (val == 0) * 1 + (val != 0) * val2;
}

/**
 * @brief Get the next highest 64-bit power of two for @p val
 *
 * This macro rounds-up @p val to the next-highest power of two, as opposed to @ref ROUND_UP which
 * rounds up a certain value to the next multiple of some alignment.
 *
 * @note This function should be used with runtime C or C++ expressions and compile-time
 * constant C++ expressions.
 *
 * @param val the value for which to calculate the next highest power of two
 *
 * @retval the next highest power of two for @p val
 * @retval zero, if the next higher power of two for @p val would saturate 64-bits
 *
 * @see See @ref NHPOT64 for a compile-time constant C macro
 * @see See @ref nhpot for the 32-bit variant of this function
 */
static NHPOT_CONSTEXPR inline uint64_t nhpot64(uint64_t val)
{
	uint64_t val2 = val;

	val2--;
	val2 |= val2 >> 1;
	val2 |= val2 >> 2;
	val2 |= val2 >> 4;
	val2 |= val2 >> 8;
	val2 |= val2 >> 16;
	val2 |= val2 >> 32;
	val2++;

	return (val == 0) * 1ULL + (val != 0) * val2;
}

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_NHPOT_H_ */
