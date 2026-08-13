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

/*
 * k_sleep() and k_usleep() below are inlined on top of k_sleep_ticks()
 * rather than being separate out of line entry points.  That way the compiler
 * sees both whether the requested duration is constant and whether the caller
 * actually cares about the returned value, and it can fold or discard the
 * unit conversions accordingly.
 */

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

	return (int32_t)MIN(k_ticks_to_ms_ceil64(ticks), (uint64_t)INT32_MAX);
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
	k_ticks_t ticks = k_sleep_ticks(Z_TIMEOUT_TICKS(k_us_to_ticks_ceil64(MAX(us, 0))));

	return (int32_t)MIN(k_ticks_to_us_ceil64(ticks), (uint64_t)INT32_MAX);
}

/** @} */

#ifdef __cplusplus
}
#endif

#include <zephyr/syscalls/sleep.h>

#endif /* ZEPHYR_INCLUDE_SLEEP_H_ */
