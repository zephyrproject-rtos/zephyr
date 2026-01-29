/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_Q31_H_
#define ZEPHYR_INCLUDE_SYS_Q31_H_

#include <zephyr/kernel.h>
#include <zephyr/dsp/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */

#define Z_PRIV_SYS_Q31_RANGE_SCALE(_range_min, _range_max) \
	(((1LL * (_range_max)) - (1LL * (_range_min))) / 2)

#define Z_PRIV_SYS_Q31_RANGE_CENTER(_range_min, _range_max) \
	((_range_min) + Z_PRIV_SYS_Q31_RANGE_SCALE(_range_min, _range_max))

/** @endcond */

/**
 * @brief Convert nano value between 1000000000 and -1000000000 to Q31 notation
 *
 * @param _nano Value in 1/1000000000 (nano) to convert to Q31 notation
 *
 * @note Positive Q31 range is 0 to 2147483647, negative Q31 range is 0 to -2147483648
 *
 * @retval Value in Q31 notation
 */
#define SYS_Q31_NANO(_nano)									\
	(											\
		(_nano) == 0									\
			? 0									\
			: (_nano) < 0								\
				? ((_nano) * 0x80000000LL) / 1000000000LL			\
				: ((_nano) * 0x7FFFFFFFLL) / 1000000000LL			\
	)

/**
 * @brief Convert micro value between 1000000 and -1000000 to Q31 notation
 *
 * @param _micro Value in 1/1000000 (micro) to convert to Q31 notation
 *
 * @see SYS_Q31_NANO()
 *
 * @retval Value in Q31 notation
 */
#define SYS_Q31_MICRO(_micro) \
	SYS_Q31_NANO((_micro) * 1000LL)

/**
 * @brief Convert milli value between 1000 and -1000 to Q31 notation
 *
 * @param _milli Value in 1/1000 (milli) to convert to Q31 notation
 *
 * @see SYS_Q31_MICRO()
 *
 * @retval Value in Q31 notation
 */
#define SYS_Q31_MILLI(_milli) \
	SYS_Q31_MICRO((_milli) * 1000LL)

/**
 * @brief Convert decimal value between 1 and -1 to Q31 notation
 *
 * @param _dec Value in 1/1 (integer) to convert to Q31 notation
 *
 * @see SYS_Q31_MILLI()
 *
 * @retval Value in Q31 notation
 */
#define SYS_Q31_DEC(_dec) \
	SYS_Q31_MILLI((_dec) * 1000LL)

/**
 * @brief Invert value in Q31 notation (-1 to 1)
 *
 * @details
 *
 * Examples:
 *
 *   SYS_Q31_INVERT(INT32_MIN) -> INT32_MAX
 *   SYS_Q31_INVERT(0)         -> 0
 *   SYS_Q31_INVERT(INT32_MAX) -> INT32_MIN
 *
 * @param _value Value in Q31 notation [-2147483648, ..., 2147483647]
 *
 * @retval Inverted value in Q31 notation
 */
#define SYS_Q31_INVERT(_value)									\
	(											\
		(_value) == INT32_MAX								\
			? INT32_MIN								\
			: (_value) < 0								\
				? -((_value) + 1)						\
				: -(_value)							\
	)

/**
 * @brief Scale value in Q31 notation (-1 to 1) to specified integer scale
 *
 * @details
 *
 * Examples:
 *
 *   SYS_Q31_SCALE(SYS_Q31_DEC(1), 500)      ->  500
 *   SYS_Q31_SCALE(SYS_Q31_MILLI(500), 500)  ->  250
 *   SYS_Q31_SCALE(SYS_Q31_DEC(0), 500)      ->  0
 *   SYS_Q31_SCALE(SYS_Q31_MILLI(-500), 500) -> -250
 *   SYS_Q31_SCALE(SYS_Q31_DEC(-1), 500)     -> -500
 *
 * @param _value Fraction of integer scale in Q31 notation [-2147483648, ..., 2147483647]
 *
 * @retval Value scaled to scale
 */
#define SYS_Q31_SCALE(_value, _scale)								\
	(											\
		(_value) == 0									\
			? 0									\
			: (_value) < 0								\
				? ((1LL * (_value)) * (_scale)) >> 31				\
				: (((1LL * (_value)) + 1) * (_scale)) >> 31			\
	)

/**
 * @brief Scale value in Q31 notation (-1 to 1) to specified integer range
 *
 * @details
 *
 * Examples:
 *
 *   SYS_Q31_RANGE(SYS_Q31_DEC(1), 0, 500)      -> 500
 *   SYS_Q31_RANGE(SYS_Q31_MILLI(500), 0, 500)  -> 375
 *   SYS_Q31_RANGE(SYS_Q31_DEC(0), 0, 500)      -> 250
 *   SYS_Q31_RANGE(SYS_Q31_MILLI(-500), 0, 500) -> 125
 *   SYS_Q31_RANGE(SYS_Q31_DEC(-1), 0, 500)     -> 0
 *
 * @param _value Fraction of integer range in Q31 notation [-2147483648, ..., 2147483647]
 * @param _range_min integer range minimum/start [-2147483647, ..., 2147483646]
 * @param _range_max integer range maximum/end [-2147483646, ..., 2147483647]
 *
 * @warning @p _range_max must be larger than @p _range_min
 *
 * @retval Value scaled to range
 */
#define SYS_Q31_RANGE(_value, _range_min, _range_max)						\
	(											\
		SYS_Q31_SCALE(_value, Z_PRIV_SYS_Q31_RANGE_SCALE(_range_min, _range_max)) +	\
		Z_PRIV_SYS_Q31_RANGE_CENTER(_range_min, _range_max)				\
	)

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_Q31_H_ */
