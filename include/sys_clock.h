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

#include <misc/util.h>
#include <misc/dlist.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <toolchain.h>
#include <zephyr/types.h>

#ifdef CONFIG_TICKLESS_KERNEL
extern int _sys_clock_always_on;
extern void z_enable_sys_clock(void);
#endif

__syscall int z_clock_hw_cycles_per_sec_runtime_get(void);

static inline int z_impl_z_clock_hw_cycles_per_sec_runtime_get(void)
{
	extern int z_clock_hw_cycles_per_sec;

	return z_clock_hw_cycles_per_sec;
}

static inline int sys_clock_hw_cycles_per_sec(void)
{
	return z_clock_hw_cycles_per_sec_runtime_get();
}

/* Note that some systems with comparatively slow cycle counters
 * experience precision loss when doing math like this.  In the
 * general case it is not correct that "cycles" are much faster than
 * "ticks".
 */
static inline int sys_clock_hw_cycles_per_tick(void)
{
#ifdef CONFIG_SYS_CLOCK_EXISTS
	return sys_clock_hw_cycles_per_sec() / CONFIG_SYS_CLOCK_TICKS_PER_SEC;
#else
	return 1; /* Just to avoid a division by zero */
#endif
}

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


/* kernel clocks */

/*
 * We default to using 64-bit intermediates in timescale conversions,
 * but if the HW timer cycles/sec, ticks/sec and ms/sec are all known
 * to be nicely related, then we can cheat with 32 bits instead.
 */

#ifdef CONFIG_SYS_CLOCK_EXISTS

static ALWAYS_INLINE s32_t z_ms_to_cycles(s32_t ms)
{
	return ceiling_fraction((u64_t)(ms) * sys_clock_hw_cycles_per_sec(),
				MSEC_PER_SEC);
}

static ALWAYS_INLINE s32_t z_us_to_cycles(s32_t us)
{
	return ceiling_fraction((u64_t)(us) * sys_clock_hw_cycles_per_sec(),
				USEC_PER_SEC);
}

static ALWAYS_INLINE s32_t z_cycles_to_ms(s32_t cycles)
{
	return (u64_t)(cycles) * MSEC_PER_SEC / sys_clock_hw_cycles_per_sec();
}

static ALWAYS_INLINE s32_t z_cycles_to_us(s32_t cycles)
{
	return (u64_t)(cycles) * USEC_PER_SEC / sys_clock_hw_cycles_per_sec();
}


/* SYS_CLOCK_HW_CYCLES_TO_NS64 converts CPU clock cycles to nanoseconds */
#define SYS_CLOCK_HW_CYCLES_TO_NS64(X) \
	(((u64_t)(X) * NSEC_PER_SEC) / sys_clock_hw_cycles_per_sec())

/*
 * SYS_CLOCK_HW_CYCLES_TO_NS_AVG converts CPU clock cycles to nanoseconds
 * and calculates the average cycle time
 */
#define SYS_CLOCK_HW_CYCLES_TO_NS_AVG(X, NCYCLES) \
	(u32_t)(SYS_CLOCK_HW_CYCLES_TO_NS64(X) / NCYCLES)


#endif
/**
 * @defgroup clock_apis Kernel Clock APIs
 * @ingroup kernel_apis
 * @{
 */

/**
 * @brief Compute nanoseconds from hardware clock cycles.
 *
 * This macro converts a time duration expressed in hardware clock cycles
 * to the equivalent duration expressed in nanoseconds.
 *
 * @param X Duration in hardware clock cycles.
 *
 * @return Duration in nanoseconds.
 */
#define SYS_CLOCK_HW_CYCLES_TO_NS(X) (u32_t)(SYS_CLOCK_HW_CYCLES_TO_NS64(X))

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
	s32_t cycle;
	_timeout_func_t fn;
};

#ifdef __cplusplus
}
#endif

#include <syscalls/sys_clock.h>

#endif /* ZEPHYR_INCLUDE_SYS_CLOCK_H_ */
