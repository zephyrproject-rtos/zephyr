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

/**
 * @brief Tick precision used in timeout APIs
 *
 * This type defines the word size of the timeout values used in
 * k_timeout_t objects, and thus defines an upper bound on maximum
 * timeout length (or equivalently minimum tick duration).  Note that
 * this does not affect the size of the system uptime counter, which
 * is always a 64 bit count of ticks.
 */
#ifdef CONFIG_TIMEOUT_64BIT
typedef int64_t k_ticks_t;
#else
typedef uint32_t k_ticks_t;
#endif

#define K_TICKS_FOREVER ((k_ticks_t) -1)

/**
 * @brief Kernel timeout type
 *
 * Timeout arguments presented to kernel APIs are stored in this
 * opaque type, which is capable of representing times in various
 * formats and units.  It should be constructed from application data
 * using one of the macros defined for this purpose (e.g. `K_MSEC()`,
 * `K_TIMEOUT_ABS_TICKS()`, etc...), or be one of the two constants
 * K_NO_WAIT or K_FOREVER.  Applications should not inspect the
 * internal data once constructed.  Timeout values may be compared for
 * equality with the `K_TIMEOUT_EQ()` macro.
 */
typedef struct {
	k_ticks_t ticks;
} k_timeout_t;

/**
 * @brief Compare timeouts for equality
 *
 * The k_timeout_t object is an opaque struct that should not be
 * inspected by application code.  This macro exists so that users can
 * test timeout objects for equality with known constants
 * (e.g. K_NO_WAIT and K_FOREVER) when implementing their own APIs in
 * terms of Zephyr timeout constants.
 *
 * @return True if the timeout objects are identical
 */
#define K_TIMEOUT_EQ(a, b) ((a).ticks == (b).ticks)

/** number of nanoseconds per microsecond */
#define NSEC_PER_USEC 1000U

/** number of nanoseconds per millisecond */
#define NSEC_PER_MSEC 1000000U

/** number of microseconds per millisecond */
#define USEC_PER_MSEC 1000U

/** number of milliseconds per second */
#define MSEC_PER_SEC 1000U

/** number of seconds per minute */
#define SEC_PER_MIN 60U

/** number of minutes per hour */
#define MIN_PER_HOUR 60U

/** number of hours per day */
#define HOUR_PER_DAY 24U

/** number of microseconds per second */
#define USEC_PER_SEC ((USEC_PER_MSEC) * (MSEC_PER_SEC))

/** number of nanoseconds per second */
#define NSEC_PER_SEC ((NSEC_PER_USEC) * (USEC_PER_MSEC) * (MSEC_PER_SEC))

/** @cond INTERNAL_HIDDEN */
#define Z_TIMEOUT_NO_WAIT ((k_timeout_t) {0})
#if defined(__cplusplus) && ((__cplusplus - 0) < 202002L)
#define Z_TIMEOUT_TICKS(t) ((k_timeout_t) { (t) })
#else
#define Z_TIMEOUT_TICKS(t) ((k_timeout_t) { .ticks = (t) })
#endif
#define Z_FOREVER Z_TIMEOUT_TICKS(K_TICKS_FOREVER)

/* Converts between absolute timeout expiration values (packed into
 * the negative space below K_TICKS_FOREVER) and (non-negative) delta
 * timeout values.  If the result of Z_TICK_ABS(t) is >= 0, then the
 * value was an absolute timeout with the returned expiration time.
 * Note that this macro is bidirectional: Z_TICK_ABS(Z_TICK_ABS(t)) ==
 * t for all inputs, and that the representation of K_TICKS_FOREVER is
 * the same value in both spaces!  Clever, huh?
 */
#define Z_TICK_ABS(t) (K_TICKS_FOREVER - 1 - (t))

/* added tick needed to account for tick in progress */
#define _TICK_ALIGN 1
/** INTERNAL_HIDDEN @endcond */

#ifdef CONFIG_TIMEOUT_QUEUE

/**
 * @brief Kernel timepoint type
 *
 * Absolute timepoints are stored in this opaque type.
 * It is best not to inspect its content directly.
 *
 * @see sys_timepoint_calc()
 * @see sys_timepoint_timeout()
 * @see sys_timepoint_expired()
 */
typedef struct { uint64_t tick; } k_timepoint_t;

/**
 * @brief Calculate a timepoint value
 *
 * Returns a timepoint corresponding to the expiration (relative to an
 * unlocked "now"!) of a timeout object.  When used correctly, this should
 * be called once, synchronously with the user passing a new timeout value.
 * It should not be used iteratively to adjust a timeout (see
 * `sys_timepoint_timeout()` for that purpose).
 *
 * @param timeout Timeout value relative to current time (may also be
 *                `K_FOREVER` or `K_NO_WAIT`).
 * @retval Timepoint value corresponding to given timeout
 *
 * @see sys_timepoint_timeout()
 * @see sys_timepoint_expired()
 */
k_timepoint_t sys_timepoint_calc(k_timeout_t timeout);

/**
 * @brief Remaining time to given timepoint
 *
 * Returns the timeout interval between current time and provided timepoint.
 * If the timepoint is now in the past or if it was created with `K_NO_WAIT`
 * then `K_NO_WAIT` is returned. If it was created with `K_FOREVER` then
 * `K_FOREVER` is returned.
 *
 * @param timepoint Timepoint for which a timeout value is wanted.
 * @retval Corresponding timeout value.
 *
 * @see sys_timepoint_calc()
 */
k_timeout_t sys_timepoint_timeout(k_timepoint_t timepoint);

/**
 * @brief Compare two timepoint values.
 *
 * This function is used to compare two timepoint values.
 *
 * @param a Timepoint to compare
 * @param b Timepoint to compare against.
 * @return zero if both timepoints are the same. Negative value if timepoint @a a is before
 * timepoint @a b, positive otherwise.
 */
static inline int sys_timepoint_cmp(k_timepoint_t a, k_timepoint_t b)
{
	if (a.tick == b.tick) {
		return 0;
	}
	return a.tick < b.tick ? -1 : 1;
}

#else /* CONFIG_TIMEOUT_QUEUE */

/*
 * When timers are configured out, timepoints can't relate to anything.
 * The best we can do is to preserve whether or not they are derived from
 * K_NO_WAIT. Anything else will translate back to K_FOREVER.
 */
typedef struct { bool wait; } k_timepoint_t;

static inline k_timepoint_t sys_timepoint_calc(k_timeout_t timeout)
{
	k_timepoint_t timepoint;

	timepoint.wait = !K_TIMEOUT_EQ(timeout, Z_TIMEOUT_NO_WAIT);
	return timepoint;
}

static inline k_timeout_t sys_timepoint_timeout(k_timepoint_t timepoint)
{
	return timepoint.wait ? Z_FOREVER : Z_TIMEOUT_NO_WAIT;
}

static inline int sys_timepoint_cmp(k_timepoint_t a, k_timepoint_t b)
{
	if (a.wait == b.wait) {
		return 0;
	}
	return b.wait ? -1 : 1;
}

#endif /* CONFIG_TIMEOUT_QUEUE */

/**
 * @brief Indicates if timepoint is expired
 *
 * @param timepoint Timepoint to evaluate
 * @retval true if the timepoint is in the past, false otherwise
 *
 * @see sys_timepoint_calc()
 */
static inline bool sys_timepoint_expired(k_timepoint_t timepoint)
{
	return K_TIMEOUT_EQ(sys_timepoint_timeout(timepoint), Z_TIMEOUT_NO_WAIT);
}

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_TIME_H_ */
