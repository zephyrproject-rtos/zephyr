/*
 * Copyright (c) 2014-2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Variables needed for system clock
 *
 *
 * Declare variables used by both system timer device driver and kernel
 * components that use timer functionality.
 */

#ifndef ZEPHYR_INCLUDE_SYS_CLOCK_H_
#define ZEPHYR_INCLUDE_SYS_CLOCK_H_

#include <zephyr/sys/util.h>
#include <zephyr/sys/dlist.h>

#include <zephyr/toolchain.h>
#include <zephyr/types.h>

#include <zephyr/sys/time_units.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup clock_apis
 * @{
 */

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

/** number of seconds per hour */
#define SEC_PER_HOUR 3600U

/** number of seconds per day */
#define SEC_PER_DAY 86400U

/** number of minutes per hour */
#define MIN_PER_HOUR 60U

/** number of hours per day */
#define HOUR_PER_DAY 24U

/** number of microseconds per second */
#define USEC_PER_SEC ((USEC_PER_MSEC) * (MSEC_PER_SEC))

/** number of nanoseconds per second */
#define NSEC_PER_SEC ((NSEC_PER_USEC) * (USEC_PER_MSEC) * (MSEC_PER_SEC))

/** @} */

/** @cond INTERNAL_HIDDEN */
#define Z_TIMEOUT_NO_WAIT_INIT {0}
#define Z_TIMEOUT_NO_WAIT ((k_timeout_t) Z_TIMEOUT_NO_WAIT_INIT)
#if defined(__cplusplus) && ((__cplusplus - 0) < 202002L)
#define Z_TIMEOUT_TICKS_INIT(t) { (t) }
#else
#define Z_TIMEOUT_TICKS_INIT(t) { .ticks = (t) }
#endif
#define Z_TIMEOUT_TICKS(t) ((k_timeout_t) Z_TIMEOUT_TICKS_INIT(t))
#define Z_FOREVER Z_TIMEOUT_TICKS(K_TICKS_FOREVER)

#ifdef CONFIG_TIMEOUT_64BIT
# define Z_TIMEOUT_MS(t) Z_TIMEOUT_TICKS((k_ticks_t)k_ms_to_ticks_ceil64(MAX(t, 0)))
# define Z_TIMEOUT_US(t) Z_TIMEOUT_TICKS((k_ticks_t)k_us_to_ticks_ceil64(MAX(t, 0)))
# define Z_TIMEOUT_NS(t) Z_TIMEOUT_TICKS((k_ticks_t)k_ns_to_ticks_ceil64(MAX(t, 0)))
# define Z_TIMEOUT_CYC(t) Z_TIMEOUT_TICKS((k_ticks_t)k_cyc_to_ticks_ceil64(MAX(t, 0)))
# define Z_TIMEOUT_MS_TICKS(t) ((k_ticks_t)k_ms_to_ticks_ceil64(MAX(t, 0)))
#else
# define Z_TIMEOUT_MS(t) Z_TIMEOUT_TICKS((k_ticks_t)k_ms_to_ticks_ceil32(MAX(t, 0)))
# define Z_TIMEOUT_US(t) Z_TIMEOUT_TICKS((k_ticks_t)k_us_to_ticks_ceil32(MAX(t, 0)))
# define Z_TIMEOUT_NS(t) Z_TIMEOUT_TICKS((k_ticks_t)k_ns_to_ticks_ceil32(MAX(t, 0)))
# define Z_TIMEOUT_CYC(t) Z_TIMEOUT_TICKS((k_ticks_t)k_cyc_to_ticks_ceil32(MAX(t, 0)))
# define Z_TIMEOUT_MS_TICKS(t) ((k_ticks_t)k_ms_to_ticks_ceil32(MAX(t, 0)))
#endif

/* Converts between absolute timeout expiration values (packed into
 * the negative space below K_TICKS_FOREVER) and (non-negative) delta
 * timeout values.  If the result of Z_TICK_ABS(t) is >= 0, then the
 * value was an absolute timeout with the returned expiration time.
 * Note that this macro is bidirectional: Z_TICK_ABS(Z_TICK_ABS(t)) ==
 * t for all inputs, and that the representation of K_TICKS_FOREVER is
 * the same value in both spaces!  Clever, huh?
 */
#define Z_TICK_ABS(t) (K_TICKS_FOREVER - 1 - (t))

/* Test for relative timeout */
#if CONFIG_TIMEOUT_64BIT
#define Z_IS_TIMEOUT_RELATIVE(timeout)  (Z_TICK_ABS((timeout).ticks) < 0)
#else
#define Z_IS_TIMEOUT_RELATIVE(timeout)  true
#endif

/* added tick needed to account for tick in progress */
#define _TICK_ALIGN 1

/** @endcond */

#ifndef CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME
#if defined(CONFIG_SYS_CLOCK_EXISTS) && \
	(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC == 0)
#error "SYS_CLOCK_HW_CYCLES_PER_SEC must be non-zero!"
#endif
#endif /* CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME */

/* kernel clocks */

/*
 * We default to using 64-bit intermediates in timescale conversions,
 * but if the HW timer cycles/sec, ticks/sec and ms/sec are all known
 * to be nicely related, then we can cheat with 32 bits instead.
 */
/**
 * @addtogroup clock_apis
 * @{
 */

#ifdef CONFIG_SYS_CLOCK_EXISTS

#if defined(CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME) || \
	(MSEC_PER_SEC % CONFIG_SYS_CLOCK_TICKS_PER_SEC) || \
	(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC % CONFIG_SYS_CLOCK_TICKS_PER_SEC)
#define _NEED_PRECISE_TICK_MS_CONVERSION
#endif

#endif

/**
 * SYS_CLOCK_HW_CYCLES_TO_NS_AVG converts CPU clock cycles to nanoseconds
 * and calculates the average cycle time
 */
#define SYS_CLOCK_HW_CYCLES_TO_NS_AVG(X, NCYCLES) \
	(uint32_t)(k_cyc_to_ns_floor64(X) / NCYCLES)

/**
 *
 * @brief Return the lower part of the current system tick count
 *
 * @return the current system tick count
 *
 */
uint32_t sys_clock_tick_get_32(void);

/**
 *
 * @brief Return the current system tick count
 *
 * @return the current system tick count
 *
 */
int64_t sys_clock_tick_get(void);

#ifndef CONFIG_SYS_CLOCK_EXISTS
#define sys_clock_tick_get() (0)
#define sys_clock_tick_get_32() (0)
#endif

#ifdef CONFIG_SYS_CLOCK_EXISTS

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

#else

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

#endif

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

#endif /* ZEPHYR_INCLUDE_SYS_CLOCK_H_ */
