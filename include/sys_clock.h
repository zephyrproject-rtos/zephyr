/*
 * Copyright (c) 2014-2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Variables needed needed for system clock
 *
 *
 * Declare variables used by both system timer device driver and kernel
 * components that use timer functionality.
 */

#ifndef ZEPHYR_INCLUDE_SYS_CLOCK_H_
#define ZEPHYR_INCLUDE_SYS_CLOCK_H_

#include <sys/util.h>
#include <sys/dlist.h>

#include <toolchain.h>
#include <zephyr/types.h>

#include <sys/time_units.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_TICKLESS_KERNEL
extern int _sys_clock_always_on;
extern void z_enable_sys_clock(void);
#endif

#if defined(CONFIG_SYS_CLOCK_EXISTS) && \
	(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC == 0)
#error "SYS_CLOCK_HW_CYCLES_PER_SEC must be non-zero!"
#endif

/* number of nsec per usec */
#define NSEC_PER_USEC 1000U

/* number of microseconds per millisecond */
#define USEC_PER_MSEC 1000U

/* number of milliseconds per second */
#define MSEC_PER_SEC 1000U

/* number of microseconds per second */
#define USEC_PER_SEC ((USEC_PER_MSEC) * (MSEC_PER_SEC))

/* number of nanoseconds per second */
#define NSEC_PER_SEC ((NSEC_PER_USEC) * (USEC_PER_MSEC) * (MSEC_PER_SEC))

#define k_msleep(ms) k_sleep(ms)
#define K_TIMEOUT_EQ(a, b) ((a) == (b))

/* kernel clocks */

/*
 * We default to using 64-bit intermediates in timescale conversions,
 * but if the HW timer cycles/sec, ticks/sec and ms/sec are all known
 * to be nicely related, then we can cheat with 32 bits instead.
 */

#ifdef CONFIG_SYS_CLOCK_EXISTS

#if defined(CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME) || \
	(MSEC_PER_SEC % CONFIG_SYS_CLOCK_TICKS_PER_SEC) || \
	(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC % CONFIG_SYS_CLOCK_TICKS_PER_SEC)
#define _NEED_PRECISE_TICK_MS_CONVERSION
#endif

#endif

#define __ticks_to_ms(t) __DEPRECATED_MACRO \
	k_ticks_to_ms_floor64((u64_t)(t))
#define z_ms_to_ticks(t) \
	((s32_t)k_ms_to_ticks_ceil32((u32_t)(t)))
#define __ticks_to_us(t) __DEPRECATED_MACRO \
	((s32_t)k_ticks_to_us_floor32((u32_t)(t)))
#define z_us_to_ticks(t) __DEPRECATED_MACRO \
	((s32_t)k_us_to_ticks_ceil32((u32_t)(t)))
#define sys_clock_hw_cycles_per_tick() __DEPRECATED_MACRO \
	((int)k_ticks_to_cyc_floor32(1U))
#define SYS_CLOCK_HW_CYCLES_TO_NS64(t) __DEPRECATED_MACRO \
	k_cyc_to_ns_floor64((u64_t)(t))
#define SYS_CLOCK_HW_CYCLES_TO_NS(t) __DEPRECATED_MACRO \
	((u32_t)k_cyc_to_ns_floor64(t))

/* added tick needed to account for tick in progress */
#define _TICK_ALIGN 1

/*
 * SYS_CLOCK_HW_CYCLES_TO_NS_AVG converts CPU clock cycles to nanoseconds
 * and calculates the average cycle time
 */
#define SYS_CLOCK_HW_CYCLES_TO_NS_AVG(X, NCYCLES) \
	(u32_t)(k_cyc_to_ns_floor64(X) / NCYCLES)

/**
 * @defgroup clock_apis Kernel Clock APIs
 * @ingroup kernel_apis
 * @{
 */

/**
 * @} end defgroup clock_apis
 */

/**
 *
 * @brief Return the lower part of the current system tick count
 *
 * @return the current system tick count
 *
 */
u32_t z_tick_get_32(void);

/**
 *
 * @brief Return the current system tick count
 *
 * @return the current system tick count
 *
 */
s64_t z_tick_get(void);

#ifndef CONFIG_SYS_CLOCK_EXISTS
#define z_tick_get() (0)
#define z_tick_get_32() (0)
#endif

/* timeouts */

struct _timeout;
typedef void (*_timeout_func_t)(struct _timeout *t);

struct _timeout {
	sys_dnode_t node;
	s32_t dticks;
	_timeout_func_t fn;
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_CLOCK_H_ */
