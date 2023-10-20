/*
 * Copyright (c) 2014-2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Utilities needed for system clock
 *
 * Declare utilities for both, system timer device driver and kernel components,
 * that use the system clock.
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
 * @addtogroup sys_clock_apis
 * @{
 */

/** @cond INTERNAL_HIDDEN */
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
/** @endcond */

/**
 * SYS_CLOCK_HW_CYCLES_TO_NS_AVG converts CPU clock cycles to nanoseconds
 * and calculates the average cycle time
 */
#define SYS_CLOCK_HW_CYCLES_TO_NS_AVG(X, NCYCLES) \
	(uint32_t)(k_cyc_to_ns_floor64(X) / NCYCLES)

#ifdef CONFIG_SYS_CLOCK_EXISTS

#if CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC == 0
#error "SYS_CLOCK_HW_CYCLES_PER_SEC must be non-zero!"
#endif

/* We default to using 64-bit intermediates in timescale conversions,
 * but if the HW timer cycles/sec, ticks/sec and ms/sec are all known
 * to be nicely related, then we can cheat with 32 bits instead.
 */
#if defined(CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME) || \
	(MSEC_PER_SEC % CONFIG_SYS_CLOCK_TICKS_PER_SEC) || \
	(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC % CONFIG_SYS_CLOCK_TICKS_PER_SEC)
#define _NEED_PRECISE_TICK_MS_CONVERSION
#endif

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

/**
 * @brief Provided for backward compatibility.
 *
 * This is deprecated. Consider `sys_timepoint_calc()` instead.
 *
 * @see sys_timepoint_calc()
 */
__deprecated
static inline uint64_t sys_clock_timeout_end_calc(k_timeout_t timeout)
{
	k_timepoint_t tp = sys_timepoint_calc(timeout);

	return tp.tick;
}
#else
#define sys_clock_tick_get() (0)
#define sys_clock_tick_get_32() (0)
#endif /* CONFIG_SYS_CLOCK_EXISTS */

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_CLOCK_H_ */
