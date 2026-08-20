/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SLEEP_H_
#define ZEPHYR_INCLUDE_SLEEP_H_

/**
 * @file
 * @brief Thread sleep APIs
 */

#include <stdint.h>

#include <zephyr/sys/clock.h>
#include <zephyr/sys/time_units.h>
#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup thread_apis
 * @{
 */

/**
 * @brief Put the current thread to sleep, with tick resolution.
 *
 * This routine puts the current thread to sleep for @a duration,
 * specified as a k_timeout_t object.
 *
 * This is the primitive that k_sleep(), k_msleep() and k_usleep() are built
 * upon.  It reports the time left to sleep in ticks, the unit the kernel
 * works in, so it needs no unit conversion and is the cheapest of the four.
 *
 * @param timeout Desired duration of sleep.
 *
 * @return Zero if the requested time has elapsed, or the time left to sleep
 * in ticks (e.g. if the thread was awoken by the \ref k_wakeup call).  If
 * @a timeout is K_FOREVER and the thread is woken early via k_wakeup(),
 * K_TICKS_FOREVER is returned.
 */
__syscall k_ticks_t k_sleep_ticks(k_timeout_t timeout);

/** @cond INTERNAL_HIDDEN */

/*
 * k_sleep() and k_usleep() below are inlined on top of k_sleep_ticks()
 * rather than being separate out of line entry points.  That way the compiler
 * sees both whether the requested duration is constant and whether the caller
 * actually cares about the returned value, and it can fold or discard the
 * unit conversions accordingly.  Those conversions are the sole reason many
 * small builds pull in the 64 bit division helper.
 *
 * For the callers that do use the returned value, the two converters below
 * keep the arithmetic 32 bit wide.  They first clamp in the tick domain,
 * against a bound computed at compile time, which is what makes the narrower
 * arithmetic safe.  It pays off because a compiler strength reduces a 32 bit
 * division by a constant into a multiply, yet calls the 64 bit division
 * helper for that same constant divisor in 64 bit.
 *
 * Where the tick rate makes the conversion a single division or a single
 * multiplication, that holds on any target.  The split forms are different:
 * they trade one division for two or three, which only wins where the one
 * they replace would have been a call to a helper.  A 64 bit target divides
 * in hardware, so it keeps the plain converter for those.
 *
 * The tick count handed to these converters always lies in the
 * [0, INT32_MAX] range, since k_sleep_ticks() derives it from a 32 bit
 * subtraction, so narrowing k_ticks_t to a uint32_t loses nothing.
 */

#define Z_SLEEP_TICK_HZ ((uint32_t)CONFIG_SYS_CLOCK_TICKS_PER_SEC)

/* True when the conversion reduces to a single division or multiplication,
 * needing none of the splits.
 */
#define Z_SLEEP_IS_SIMPLE(to_hz) \
	(Z_SLEEP_TICK_HZ % (to_hz) == 0 || (to_hz) % Z_SLEEP_TICK_HZ == 0)

/* Largest tick count whose value in to_hz units still fits in an int32_t. */
#define Z_SLEEP_MAX_TICKS(to_hz) ((uint64_t)INT32_MAX * Z_SLEEP_TICK_HZ / (to_hz))

/* True when (rem * to_hz + Z_SLEEP_TICK_HZ - 1) cannot overflow 32 bits,
 * knowing that rem is a remainder modulo Z_SLEEP_TICK_HZ.
 */
#define Z_SLEEP_NO_OVERFLOW(to_hz) \
	((uint64_t)Z_SLEEP_TICK_HZ * ((to_hz) + 1U) <= UINT32_MAX)

/* Past the clamp, ceil(t * to_hz / hz) without exceeding 32 bits: split the
 * division so the whole seconds are scaled separately from the remainder.
 */
#define Z_SLEEP_SPLIT(t, to_hz)                                             \
	(((t) / Z_SLEEP_TICK_HZ) * (to_hz) +                                \
	 DIV_ROUND_UP(((t) % Z_SLEEP_TICK_HZ) * (to_hz), Z_SLEEP_TICK_HZ))

static inline int32_t z_sleep_ticks_to_int32_ms(k_ticks_t ticks)
{
	uint32_t t = (uint32_t)ticks;

	/* a split is not worth narrowing for where 64 bit division is cheap */
	if (IS_ENABLED(CONFIG_64BIT) && !Z_SLEEP_IS_SIMPLE(MSEC_PER_SEC)) {
		return (int32_t)MIN(k_ticks_to_ms_ceil64(t), (uint64_t)INT32_MAX);
	}

	if (Z_SLEEP_MAX_TICKS(MSEC_PER_SEC) < (uint64_t)INT32_MAX &&
	    t > (uint32_t)Z_SLEEP_MAX_TICKS(MSEC_PER_SEC)) {
		return INT32_MAX;
	}

	/* a whole number of ticks per millisecond, 1:1 included */
	if (Z_SLEEP_TICK_HZ % MSEC_PER_SEC == 0) {
		return (int32_t)DIV_ROUND_UP(t, z_tmcvt_divisor(Z_SLEEP_TICK_HZ, MSEC_PER_SEC));
	}

	/* a whole number of milliseconds per tick */
	if (MSEC_PER_SEC % Z_SLEEP_TICK_HZ == 0) {
		return (int32_t)(t * z_tmcvt_divisor(MSEC_PER_SEC, Z_SLEEP_TICK_HZ));
	}

	if (Z_SLEEP_NO_OVERFLOW(MSEC_PER_SEC)) {
		return (int32_t)Z_SLEEP_SPLIT(t, MSEC_PER_SEC);
	}

	/* tick rate too high to be scaled in 32 bits at all */
	return (int32_t)MIN(k_ticks_to_ms_ceil64(t), (uint64_t)INT32_MAX);
}

/* Microseconds need one step more than milliseconds: a remainder times
 * USEC_PER_SEC overflows 32 bits for any tick rate above 4294 Hz, so scale
 * the remainder by USEC_PER_MSEC twice, carrying the first quotient over.
 * With A = rem * 1000 = q * hz + r, ceil(A * 1000 / hz) is exactly
 * q * 1000 + ceil(r * 1000 / hz), and r < hz keeps the second step in range.
 */
static inline int32_t z_sleep_ticks_to_int32_us_split(uint32_t t)
{
	uint32_t sec = t / Z_SLEEP_TICK_HZ;
	uint32_t rem = (t % Z_SLEEP_TICK_HZ) * USEC_PER_MSEC;
	uint32_t q = rem / Z_SLEEP_TICK_HZ;
	uint32_t r = rem % Z_SLEEP_TICK_HZ;

	return (int32_t)(sec * USEC_PER_SEC + q * USEC_PER_MSEC +
			 DIV_ROUND_UP(r * USEC_PER_MSEC, Z_SLEEP_TICK_HZ));
}

static inline int32_t z_sleep_ticks_to_int32_us(k_ticks_t ticks)
{
	uint32_t t = (uint32_t)ticks;

	/* a split is not worth narrowing for where 64 bit division is cheap */
	if (IS_ENABLED(CONFIG_64BIT) && !Z_SLEEP_IS_SIMPLE(USEC_PER_SEC)) {
		return (int32_t)MIN(k_ticks_to_us_ceil64(t), (uint64_t)INT32_MAX);
	}

	if (Z_SLEEP_MAX_TICKS(USEC_PER_SEC) < (uint64_t)INT32_MAX &&
	    t > (uint32_t)Z_SLEEP_MAX_TICKS(USEC_PER_SEC)) {
		return INT32_MAX;
	}

	/* a whole number of ticks per microsecond, 1:1 included */
	if (Z_SLEEP_TICK_HZ % USEC_PER_SEC == 0) {
		return (int32_t)DIV_ROUND_UP(t, z_tmcvt_divisor(Z_SLEEP_TICK_HZ, USEC_PER_SEC));
	}

	/* a whole number of microseconds per tick */
	if (USEC_PER_SEC % Z_SLEEP_TICK_HZ == 0) {
		return (int32_t)(t * z_tmcvt_divisor(USEC_PER_SEC, Z_SLEEP_TICK_HZ));
	}

	if (Z_SLEEP_NO_OVERFLOW(USEC_PER_SEC)) {
		return (int32_t)Z_SLEEP_SPLIT(t, USEC_PER_SEC);
	}

	if (Z_SLEEP_NO_OVERFLOW(USEC_PER_MSEC)) {
		return z_sleep_ticks_to_int32_us_split(t);
	}

	/* tick rate too high to be scaled in 32 bits at all */
	return (int32_t)MIN(k_ticks_to_us_ceil64(t), (uint64_t)INT32_MAX);
}

/** @endcond */

/**
 * @brief Put the current thread to sleep.
 *
 * This routine puts the current thread to sleep for @a duration,
 * specified as a k_timeout_t object.
 *
 * @param timeout Desired duration of sleep.
 *
 * @return Zero if the requested time has elapsed or the time left to
 * sleep rounded up to the nearest millisecond (e.g. if the thread was
 * awoken by the \ref k_wakeup call).  Will be clamped to INT_MAX in
 * the case where the remaining time is unrepresentable in an int32_t.
 * If @a timeout is K_FOREVER and the thread is woken early via
 * k_wakeup(), -1 is returned.
 */
static inline int32_t k_sleep(k_timeout_t timeout)
{
	k_ticks_t ticks = k_sleep_ticks(timeout);

	/* k_sleep() still returns 32 bit milliseconds for compatibility */
	if (K_TIMEOUT_EQ(timeout, Z_FOREVER)) {
		return (int32_t)K_TICKS_FOREVER;
	}

	return z_sleep_ticks_to_int32_ms(ticks);
}

/**
 * @brief Put the current thread to sleep.
 *
 * This routine puts the current thread to sleep for @a duration milliseconds.
 *
 * @param ms Number of milliseconds to sleep.
 *
 * @return Zero if the requested time has elapsed or if the thread was woken up
 * by the \ref k_wakeup call, the time left to sleep rounded up to the nearest
 * millisecond.
 */
static inline int32_t k_msleep(int32_t ms)
{
	return k_sleep(Z_TIMEOUT_MS(ms));
}

/**
 * @brief Put the current thread to sleep with microsecond resolution.
 *
 * This function is unlikely to work as expected without kernel tuning.
 * In particular, because the lower bound on the duration of a sleep is
 * the duration of a tick, @kconfig{CONFIG_SYS_CLOCK_TICKS_PER_SEC} must be
 * adjusted to achieve the resolution desired. The implications of doing
 * this must be understood before attempting to use k_usleep(). Use with
 * caution.
 *
 * @param us Number of microseconds to sleep.
 *
 * @return Zero if the requested time has elapsed or if the thread was woken up
 * by the \ref k_wakeup call, the time left to sleep rounded up to the nearest
 * microsecond.
 */
static inline int32_t k_usleep(int32_t us)
{
	k_ticks_t ticks = k_sleep_ticks(Z_TIMEOUT_US(us));

	return z_sleep_ticks_to_int32_us(ticks);
}

/** @} */

#ifdef __cplusplus
}
#endif

#include <zephyr/syscalls/sleep.h>

#endif /* ZEPHYR_INCLUDE_SLEEP_H_ */
