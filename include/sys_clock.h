/*
 * Copyright (c) 2014-2015 Wind River Systems, Inc.
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_CLOCK_H_
#define ZEPHYR_INCLUDE_SYS_CLOCK_H_

#include <sys/util.h>
#include <sys/dlist.h>
#include <toolchain.h>
#include <zephyr/types.h>
#include <time_units.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_SYS_TIMEOUT_64BIT
typedef u64_t k_ticks_t;
#else
typedef u32_t k_ticks_t;
#endif

/* Putting the docs at the top allows the macros to be defined below
 * more cleanly.  They vary with configuration.
 */

/** \def K_TIMEOUT_TICKS(t)
 * @brief Initializes a k_timeout_t object with ticks
 *
 * Evaluates to a k_timeout_t object representing an API timeout that
 * will expire at least @a t ticks in the future.
 *
 * @param t Timeout in ticks
 */

/** \def K_TIMEOUT_MS(t)
 * @brief Initializes a k_timeout_t object with milliseconds
 *
 * Evaluates to a k_timeout_t object representing an API timeout that
 * will expire at least @a t milliseconds in the future.
 *
 * @param t Timeout in milliseconds
 */

/** \def K_TIMEOUT_US(t)
 * @brief Initializes a k_timeout_t object with microseconds
 *
 * Evaluates to a k_timeout_t object representing an API timeout that
 * will expire at least @a t microseconds in the future.
 *
 * @param t Timeout in microseconds
 */

/** \def K_TIMEOUT_CYC(t)
 * @brief Initializes a k_timeout_t object with hardware cycles
 *
 * Evaluates to a k_timeout_t object representing an API timeout that
 * will expire at least @a t hardware cycles in the future.
 *
 * @param t Timeout in hardware cycles
 */

/** \def Z_TIMEOUT_ABSOLUTE_TICKS(t)
 * @brief Initializes a k_timeout_t object with uptime in ticks
 *
 * Evaluates to a k_timeout_t object representing an API timeout that
 * will expire after the system uptime reaches @a t ticks
 *
 * @param t Timeout in ticks
 */

/** \def Z_TIMEOUT_ABSOLUTE_MS(t)
 * @brief Initializes a k_timeout_t object with uptime in milliseconds
 *
 * Evaluates to a k_timeout_t object representing an API timeout that
 * will expire after the system uptime reaches @a t milliseconds
 *
 * @param t Timeout in milliseconds
 */

/** \def Z_TIMEOUT_ABSOLUTE_US(t)
 * @brief Initializes a k_timeout_t object with uptime in microseconds
 *
 * Evaluates to a k_timeout_t object representing an API timeout that
 * will expire after the system uptime reaches @a t microseconds
 *
 * @param t Timeout in microseconds
 */

/** \def Z_TIMEOUT_ABSOLUTE_CYC(t)
 * @brief Initializes a k_timeout_t object with uptime in hardware cycles
 *
 * Evaluates to a k_timeout_t object representing an API timeout that
 * will expire after the system uptime reaches @a t hardware cycles
 *
 * @param t Timeout in hardware cycles
 */

/** \def K_TIMEOUT_GET(t)
 * @brief Returns the ticks expiration from a k_timeout_t
 *
 * Evaluates to the integer number of ticks stored in the k_timeout_t
 * argument, or K_FOREVER_TICKS.
 *
 * @param t A k_timeout_t object
 */

/** \def K_NO_WAIT
 * @brief Constant k_timeout_t representing immediate expiration
 *
 * Evaluates to a k_timeout_t object representing an API timeout that
 * will expire immediately (i.e. it produces a synchronous return)
 */

/** \def K_FOREVER
 * @brief Constaint k_timeout_t object representing "never expires"
 *
 * Evaluates to a k_timeout_t object representing an API timeout that
 * will never expire (such a call will wait forever).
 */

/** @brief k_ticks_t value representing no expiration
 *
 * The internal ticks value of a k_timeout_t used to represent "no
 * expiration, wait forever".
 */
#define K_FOREVER_TICKS ((k_ticks_t) -1)

#ifdef CONFIG_SYS_TIMEOUT_LEGACY_API
/* Fallback API where timeouts are still 32 bit millisecond counts */
typedef k_ticks_t k_timeout_t;
#define K_TIMEOUT_MS(t) (t)
#define K_TIMEOUT_TICKS(t) K_TIMEOUT_MS(k_ticks_to_ms_ceil32(t))
#define K_TIMEOUT_US(t) K_TIMEOUT_MS(((t) + 999) / 1000)
#define K_TIMEOUT_CYC(t) K_TIMEOUT_MS(k_cyc_to_ms_ceil32(t))
#define K_TIMEOUT_GET(t) (t)
#define K_NO_WAIT 0
#define K_FOREVER K_FOREVER_TICKS
#else
/* New API going forward: k_timeout_t is an opaque struct */
typedef struct { k_ticks_t ticks; } k_timeout_t;
#define K_TIMEOUT_TICKS(t) ((k_timeout_t){ (k_ticks_t)t })
#define K_TIMEOUT_GET(t) ((t).ticks)
#define K_NO_WAIT K_TIMEOUT_TICKS(0)
#define K_FOREVER K_TIMEOUT_TICKS(K_FOREVER_TICKS)
#endif

#define K_TIMEOUT_EQ(a, b) (K_TIMEOUT_GET(a) == K_TIMEOUT_GET(b))

struct _timeout;
typedef void (*_timeout_func_t)(struct _timeout *t);

struct _timeout {
	sys_dnode_t node;
	k_ticks_t dticks;
	_timeout_func_t fn;
};

#ifndef CONFIG_SYS_TIMEOUT_LEGACY_API
# ifdef CONFIG_SYS_TIMEOUT_64BIT
#  define K_TIMEOUT_MS(t) K_TIMEOUT_TICKS(k_ms_to_ticks_ceil64(t))
#  define K_TIMEOUT_US(t) K_TIMEOUT_TICKS(k_us_to_ticks_ceil64(t))
#  define K_TIMEOUT_CYC(t) K_TIMEOUT_TICKS(k_cyc_to_ticks_ceil64(t))
# else
#  define K_TIMEOUT_MS(t) K_TIMEOUT_TICKS(k_ms_to_ticks_ceil32(t))
#  define K_TIMEOUT_US(t) K_TIMEOUT_TICKS(k_us_to_ticks_ceil32(t))
#  define K_TIMEOUT_CYC(t) K_TIMEOUT_TICKS(k_cyc_to_ticks_ceil32(t))
# endif
#endif

#if defined(CONFIG_SYS_TIMEOUT_64BIT) && !defined(CONFIG_SYS_TIMEOUT_LEGACY_API)
#define Z_TIMEOUT_ABSOLUTE_TICKS(t) \
	K_TIMEOUT_TICKS((k_ticks_t)(K_FOREVER_TICKS - (t + 1)))
#define Z_TIMEOUT_ABSOLUTE_MS(t) Z_TIMEOUT_ABSOLUTE_TICKS(k_ms_to_ticks_ceil64(t))
#define Z_TIMEOUT_ABSOLUTE_US(t) Z_TIMEOUT_ABSOLUTE_TICKS(k_us_to_ticks_ceil64(t))
#define Z_TIMEOUT_ABSOLUTE_CYC(t) Z_TIMEOUT_ABSOLUTE_TICKS(k_cyc_to_ticks_ceil64(t))
#endif

s64_t z_tick_get(void);
u32_t z_tick_get_32(void);

/* Legacy timer conversion APIs.  Soon to be deprecated.
 */

#define __ticks_to_ms(t) k_ticks_to_ms_floor64(t)
#define z_ms_to_ticks(t) k_ms_to_ticks_ceil32(t)
#define __ticks_to_us(t) k_ticks_to_us_floor64(t)
#define z_us_to_ticks(t) k_us_to_ticks_ceil64(t)
#define sys_clock_hw_cycles_per_tick() k_ticks_to_cyc_floor32(1)
#define SYS_CLOCK_HW_CYCLES_TO_NS64(t) (1000 * k_cyc_to_us_floor64(t))
#define SYS_CLOCK_HW_CYCLES_TO_NS(t) ((u32_t)(1000 * k_cyc_to_us_floor64(t)))
#define SYS_CLOCK_HW_CYCLES_TO_NS_AVG(x, cyc) \
	((u32_t)(SYS_CLOCK_HW_CYCLES_TO_NS64(x) / cyc))

#define MSEC_PER_SEC 1000
#define USEC_PER_MSEC 1000
#define USEC_PER_SEC 1000000
#define NSEC_PER_USEC 1000
#define NSEC_PER_SEC 1000000000

#define K_MSEC(ms)     K_TIMEOUT_MS(ms)
#define K_SECONDS(s)   K_MSEC((s) * MSEC_PER_SEC)
#define K_MINUTES(m)   K_SECONDS((m) * 60)
#define K_HOURS(h)     K_MINUTES((h) * 60)

#define _TICK_ALIGN 1

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_CLOCK_H_ */
