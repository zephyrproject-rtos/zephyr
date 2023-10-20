/*
 * Copyright (c) 2014-2015 Wind River Systems, Inc.
 * Copyright (c) 2023 Florian Grandel, Zephyr Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_TIME_H_
#define ZEPHYR_INCLUDE_SYS_TIME_H_

#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * @brief Types needed to represent and convert time in general.
 *
 * @defgroup timeutil_general_time_apis General Time Representation and Conversion Helpers
 * @ingroup timeutil_apis
 *
 * @brief Various helper APIs for representing and converting between time
 * units.
 *
 * @{
 */

/** @cond INTERNAL_HIDDEN */

/**
 * Macro determines if fast conversion algorithm can be used. It checks if
 * maximum timeout represented in source frequency domain and multiplied by
 * target frequency fits in 64 bits.
 *
 * @param from_hz Source frequency.
 * @param to_hz Target frequency.
 *
 * @retval true Use faster algorithm.
 * @retval false Use algorithm preventing overflow of intermediate value.
 */
#define z_tmcvt_use_fast_algo(from_hz, to_hz) \
	((DIV_ROUND_UP(CONFIG_SYS_CLOCK_MAX_TIMEOUT_DAYS * 24ULL * 3600ULL * from_hz, \
			   UINT32_MAX) * to_hz) <= UINT32_MAX)

/* true if the conversion is the identity */
#define z_tmcvt_is_identity(__from_hz, __to_hz) \
	((__to_hz) == (__from_hz))

/* true if the conversion requires a simple integer multiply */
#define z_tmcvt_is_int_mul(__from_hz, __to_hz) \
	((__to_hz) > (__from_hz) && (__to_hz) % (__from_hz) == 0U)

/* true if the conversion requires a simple integer division */
#define z_tmcvt_is_int_div(__from_hz, __to_hz) \
	((__from_hz) > (__to_hz) && (__from_hz) % (__to_hz) == 0U)

/* Compute the offset needed to round the result correctly when
 * the conversion requires a simple integer division
 */
#define z_tmcvt_off_div(__from_hz, __to_hz, __round_up, __round_off)	\
	((__round_off) ? ((__from_hz) / (__to_hz)) / 2 :		\
	 (__round_up) ? ((__from_hz) / (__to_hz)) - 1 :			\
	 0)

/* Clang emits a divide-by-zero warning even though the int_div macro
 * results are only used when the divisor will not be zero. Work
 * around this by substituting 1 to make the compiler happy.
 */
#ifdef __clang__
#define z_tmcvt_divisor(a, b) ((a) / (b) ?: 1)
#else
#define z_tmcvt_divisor(a, b) ((a) / (b))
#endif

/* Compute the offset needed to round the result correctly when
 * the conversion requires a full mul/div
 */
#define z_tmcvt_off_gen(__from_hz, __to_hz, __round_up, __round_off)	\
	((__round_off) ? (__from_hz) / 2 :				\
	 (__round_up) ? (__from_hz) - 1 :				\
	 0)

/* Integer division 32-bit conversion */
#define z_tmcvt_int_div_32(__t, __from_hz, __to_hz, __round_up, __round_off) \
	((uint64_t) (__t) <= 0xffffffffU -				\
	 z_tmcvt_off_div(__from_hz, __to_hz, __round_up, __round_off) ?	\
	 ((uint32_t)((__t) +						\
		     z_tmcvt_off_div(__from_hz, __to_hz,		\
				     __round_up, __round_off)) /	\
	  z_tmcvt_divisor(__from_hz, __to_hz))				\
	 :								\
	 (uint32_t) (((uint64_t) (__t) +				\
		      z_tmcvt_off_div(__from_hz, __to_hz,		\
				      __round_up, __round_off)) /	\
		     z_tmcvt_divisor(__from_hz, __to_hz))		\
		)

/* Integer multiplication 32-bit conversion */
#define z_tmcvt_int_mul_32(__t, __from_hz, __to_hz)	\
	(uint32_t) (__t)*((__to_hz) / (__from_hz))

/* General 32-bit conversion */
#define z_tmcvt_gen_32(__t, __from_hz, __to_hz, __round_up, __round_off) \
	((uint32_t) (((uint64_t) (__t)*(__to_hz) +			\
		      z_tmcvt_off_gen(__from_hz, __to_hz, __round_up, __round_off)) / (__from_hz)))

/* Integer division 64-bit conversion */
#define z_tmcvt_int_div_64(__t, __from_hz, __to_hz, __round_up, __round_off) \
	(((uint64_t) (__t) + z_tmcvt_off_div(__from_hz, __to_hz,	\
					     __round_up, __round_off)) / \
	z_tmcvt_divisor(__from_hz, __to_hz))

/* Integer multiplcation 64-bit conversion */
#define z_tmcvt_int_mul_64(__t, __from_hz, __to_hz)	\
	(uint64_t) (__t)*((__to_hz) / (__from_hz))

/* Fast 64-bit conversion. This relies on the multiply not overflowing */
#define z_tmcvt_gen_64_fast(__t, __from_hz, __to_hz, __round_up, __round_off) \
	(((uint64_t) (__t)*(__to_hz) + \
	  z_tmcvt_off_gen(__from_hz, __to_hz, __round_up, __round_off)) / (__from_hz))

/* Slow 64-bit conversion. This avoids overflowing the multiply */
#define z_tmcvt_gen_64_slow(__t, __from_hz, __to_hz, __round_up, __round_off) \
	(((uint64_t) (__t) / (__from_hz))*(__to_hz) +			\
	 (((uint64_t) (__t) % (__from_hz))*(__to_hz) +		\
	  z_tmcvt_off_gen(__from_hz, __to_hz, __round_up, __round_off)) / (__from_hz))

/* General 64-bit conversion. Uses one of the two above macros */
#define z_tmcvt_gen_64(__t, __from_hz, __to_hz, __round_up, __round_off) \
	(z_tmcvt_use_fast_algo(__from_hz, __to_hz) ?			\
	 z_tmcvt_gen_64_fast(__t, __from_hz, __to_hz, __round_up, __round_off) : \
	 z_tmcvt_gen_64_slow(__t, __from_hz, __to_hz, __round_up, __round_off))

/* Convert, generating a 32-bit result */
#define z_tmcvt_32(__t, __from_hz, __to_hz, __const_hz, __round_up, __round_off) \
	((__const_hz) ?							\
	 (								\
		 z_tmcvt_is_identity(__from_hz, __to_hz) ?		\
		 (uint32_t) (__t)					\
		 :							\
		 z_tmcvt_is_int_div(__from_hz, __to_hz) ?		\
		 z_tmcvt_int_div_32(__t, __from_hz, __to_hz, __round_up, __round_off) \
		 :							\
		 z_tmcvt_is_int_mul(__from_hz, __to_hz) ?		\
		 z_tmcvt_int_mul_32(__t, __from_hz, __to_hz)		\
		 :							\
		 z_tmcvt_gen_32(__t, __from_hz, __to_hz, __round_up, __round_off) \
		 )							\
	 :								\
	 z_tmcvt_gen_32(__t, __from_hz, __to_hz, __round_up, __round_off) \
		)

/* Convert, generating a 64-bit result */
#define z_tmcvt_64(__t, __from_hz, __to_hz, __const_hz, __round_up, __round_off) \
	((__const_hz) ?							\
	 (								\
		 z_tmcvt_is_identity(__from_hz, __to_hz) ?		\
		 (uint64_t) (__t)					\
		 :							\
		 z_tmcvt_is_int_div(__from_hz, __to_hz) ?		\
		 z_tmcvt_int_div_64(__t, __from_hz, __to_hz, __round_up, __round_off) \
		 :							\
		 z_tmcvt_is_int_mul(__from_hz, __to_hz) ?		\
		 z_tmcvt_int_mul_64(__t, __from_hz, __to_hz)		\
		 :							\
		 z_tmcvt_gen_64(__t, __from_hz, __to_hz, __round_up, __round_off) \
		 )							\
	 :								\
	 z_tmcvt_gen_64_slow(__t, __from_hz, __to_hz, __round_up, __round_off) \
		)

/**
 * Time converter generator gadget. Selects from a range of optimized conversion
 * algorithms: ones that take advantage when the frequencies are an integer
 * ratio (in either direction), or a full precision conversion. Clever use of
 * extra arguments causes all the selection logic to be optimized out, and the
 * generated code even reduces to 32 bit only if a ratio conversion is available
 * and the result is 32 bits.
 *
 * This isn't intended to be used directly, instead being wrapped appropriately
 * in a user-facing API.
 *
 * @param t source (uint64_t or uint32_t) time in milliseconds
 * @param from_hz (uint32_t) frequency in Hz from which to convert the given
 * source time
 * @param to_hz (uint32_t) frequency in Hz to which to convert the given source
 * time
 * @param const_hz (bool) The hz arguments are known to be compile-time constants
 * (because otherwise the modulus test would have to be done at runtime).
 * @param result32 (bool) The result will be truncated to 32 bits on use.
 * @param round_up (bool) Return the ceiling of the resulting fraction.
 * @param round_off (bool) Return the nearest value to the resulting fraction (pass
 * both round_up/off as false to get "round_down").
 *
 * All of this must be implemented as expressions so that, when constant, the
 * results may be used to initialize global variables.
 */
#define z_tmcvt(__t, __from_hz, __to_hz, __const_hz, __result32, __round_up, __round_off) \
	((__result32) ?							\
	 z_tmcvt_32(__t, __from_hz, __to_hz, __const_hz, __round_up, __round_off) : \
	 z_tmcvt_64(__t, __from_hz, __to_hz, __const_hz, __round_up, __round_off))

/** INTERNAL_HIDDEN @endcond */


/**
 * @brief System-wide macro to denote "forever" in milliseconds
 *
 * Usage of this macro is limited to APIs that want to expose a timeout value
 * that can optionally be unlimited, or "forever".  This macro can not be fed
 * into kernel functions or macros directly. Use @ref SYS_TIMEOUT_MS instead.
 */
#define SYS_FOREVER_MS (-1)

/**
 * @brief System-wide macro to denote "forever" in microseconds
 *
 * See @ref SYS_FOREVER_MS.
 */
#define SYS_FOREVER_US (-1)

/** @brief System-wide macro to convert milliseconds to kernel timeouts */
#define SYS_TIMEOUT_MS(ms) Z_TIMEOUT_TICKS((ms) == SYS_FOREVER_MS ? \
					   K_TICKS_FOREVER : Z_TIMEOUT_MS_TICKS(ms))

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_TIME_H_ */
