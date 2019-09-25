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

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_TICKLESS_KERNEL
extern int _sys_clock_always_on;
extern void z_enable_sys_clock(void);
#endif

#if defined(CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME)
__syscall int z_clock_hw_cycles_per_sec_runtime_get(void);

static inline int z_impl_z_clock_hw_cycles_per_sec_runtime_get(void)
{
	extern int z_clock_hw_cycles_per_sec;

	return z_clock_hw_cycles_per_sec;
}
#endif /* CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME */

static inline int sys_clock_hw_cycles_per_sec(void)
{
#if defined(CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME)
	return z_clock_hw_cycles_per_sec_runtime_get();
#else
	return CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC;
#endif
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

#if defined(CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME) || \
	(MSEC_PER_SEC % CONFIG_SYS_CLOCK_TICKS_PER_SEC) || \
	(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC % CONFIG_SYS_CLOCK_TICKS_PER_SEC)
#define _NEED_PRECISE_TICK_MS_CONVERSION
#endif

#endif

static ALWAYS_INLINE s32_t z_ms_to_ticks(s32_t ms)
{
#ifdef CONFIG_SYS_CLOCK_EXISTS

#ifdef _NEED_PRECISE_TICK_MS_CONVERSION
	int cyc = sys_clock_hw_cycles_per_sec();

	/* use 64-bit math to keep precision */
	return (s32_t)ceiling_fraction((s64_t)ms * cyc,
		((s64_t)MSEC_PER_SEC * cyc) / CONFIG_SYS_CLOCK_TICKS_PER_SEC);
#else
	/* simple division keeps precision */
	s32_t ms_per_tick = MSEC_PER_SEC / CONFIG_SYS_CLOCK_TICKS_PER_SEC;

	return (s32_t)ceiling_fraction(ms, ms_per_tick);
#endif

#else
	__ASSERT(ms == 0, "ms not zero");
	return 0;
#endif
}

static inline u64_t __ticks_to_ms(s64_t ticks)
{
#ifdef CONFIG_SYS_CLOCK_EXISTS
	return (u64_t)ticks * MSEC_PER_SEC /
	       (u64_t)CONFIG_SYS_CLOCK_TICKS_PER_SEC;
#else
	__ASSERT(ticks == 0, "ticks not zero");
	return 0ULL;
#endif
}

/*
 * These are only currently used by k_usleep(), but they are
 * defined here for parity with their ms analogs above. Note:
 * we don't bother trying the 32-bit intermediate shortcuts
 * possible with ms, because of the magnitudes involved.
 */

static inline s32_t z_us_to_ticks(s32_t us)
{
#ifdef CONFIG_SYS_CLOCK_EXISTS
	return (s32_t) ceiling_fraction(
		(s64_t)us * sys_clock_hw_cycles_per_sec(),
		((s64_t)USEC_PER_SEC * sys_clock_hw_cycles_per_sec()) /
		CONFIG_SYS_CLOCK_TICKS_PER_SEC);
#else
	__ASSERT(us == 0, "us not zero");
	return 0;
#endif
}

static inline s32_t __ticks_to_us(s32_t ticks)
{
#ifdef CONFIG_SYS_CLOCK_EXISTS
	return (s32_t) ((s64_t)ticks * USEC_PER_SEC /
	       (s64_t)CONFIG_SYS_CLOCK_TICKS_PER_SEC);
#else
	__ASSERT(ticks == 0, "ticks not zero");
	return 0;
#endif
}

/* added tick needed to account for tick in progress */
#define _TICK_ALIGN 1

/* SYS_CLOCK_HW_CYCLES_TO_NS64 converts CPU clock cycles to nanoseconds */
#define SYS_CLOCK_HW_CYCLES_TO_NS64(X) \
	(((u64_t)(X) * NSEC_PER_SEC) / sys_clock_hw_cycles_per_sec())

/*
 * SYS_CLOCK_HW_CYCLES_TO_NS_AVG converts CPU clock cycles to nanoseconds
 * and calculates the average cycle time
 */
#define SYS_CLOCK_HW_CYCLES_TO_NS_AVG(X, NCYCLES) \
	(u32_t)(SYS_CLOCK_HW_CYCLES_TO_NS64(X) / NCYCLES)

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
	s32_t dticks;
	_timeout_func_t fn;
};

#ifdef __cplusplus
}
#endif

#include <syscalls/sys_clock.h>

#endif /* ZEPHYR_INCLUDE_SYS_CLOCK_H_ */
