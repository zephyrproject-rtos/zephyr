/*
 * Copyright (c) 2026 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Tick/cycle conversion helpers shared by system timer drivers
 *
 * Multiply-first conversion between kernel ticks and hardware counter cycles.
 * When TIMER_CORE_CYCLES_PER_SEC equals CONFIG_SYS_CLOCK_TICKS_PER_SEC the
 * mapping is one-to-one and collapses to a no-op.
 */

#ifndef ZEPHYR_DRIVERS_TIMER_TIMER_CORE_CONVERT_H_
#define ZEPHYR_DRIVERS_TIMER_TIMER_CORE_CONVERT_H_

#include <stdint.h>

#include <zephyr/sys/clock.h>

#if !defined(TIMER_CORE_RATE_IS_RUNTIME) && !defined(TIMER_CORE_RATE_IS_CONSTANT)
#if defined(TIMER_CORE_CYCLES_PER_SEC_RUNTIME)
#define TIMER_CORE_RATE_IS_RUNTIME
#elif !defined(TIMER_CORE_CYCLES_PER_SEC)
#if defined(CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME) || \
	defined(CONFIG_SYSTEM_CLOCK_HW_CYCLES_PER_SEC_RUNTIME_UPDATE)
#define TIMER_CORE_RATE_IS_RUNTIME
#define TIMER_CORE_CYCLES_PER_SEC sys_clock_hw_cycles_per_sec()
#else
#define TIMER_CORE_CYCLES_PER_SEC CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC
#define TIMER_CORE_RATE_IS_CONSTANT
#endif
#else
#define TIMER_CORE_RATE_IS_CONSTANT
#endif
#endif

#if !defined(TIMER_CORE_TICKS_TO_CYCLES) && defined(TIMER_CORE_RATE_IS_CONSTANT)
#if TIMER_CORE_CYCLES_PER_SEC == CONFIG_SYS_CLOCK_TICKS_PER_SEC
#define TIMER_CORE_TICKS_TO_CYCLES(ticks) (ticks)
#define TIMER_CORE_CYCLES_TO_TICKS(cycles) (cycles)
#else
#define TIMER_CORE_TICKS_TO_CYCLES(ticks) \
	(((ticks) * (uint64_t)TIMER_CORE_CYCLES_PER_SEC + \
	  (CONFIG_SYS_CLOCK_TICKS_PER_SEC / 2U)) / CONFIG_SYS_CLOCK_TICKS_PER_SEC)
#define TIMER_CORE_CYCLES_TO_TICKS(cycles) \
	(((cycles) * (uint64_t)CONFIG_SYS_CLOCK_TICKS_PER_SEC) / \
	 (uint64_t)TIMER_CORE_CYCLES_PER_SEC)
#endif
#endif

static inline uint64_t timer_core_ticks_to_cycles(uint64_t ticks)
{
#if defined(TIMER_CORE_RATE_IS_CONSTANT) && \
	(TIMER_CORE_CYCLES_PER_SEC == CONFIG_SYS_CLOCK_TICKS_PER_SEC)
	return ticks;
#elif defined(TIMER_CORE_RATE_IS_CONSTANT)
	return TIMER_CORE_TICKS_TO_CYCLES(ticks);
#else
	return (ticks * (uint64_t)sys_clock_hw_cycles_per_sec()
		+ (CONFIG_SYS_CLOCK_TICKS_PER_SEC / 2U))
		/ CONFIG_SYS_CLOCK_TICKS_PER_SEC;
#endif
}

static inline uint64_t timer_core_cycles_to_ticks(uint64_t cycles)
{
#if defined(TIMER_CORE_RATE_IS_CONSTANT) && \
	(TIMER_CORE_CYCLES_PER_SEC == CONFIG_SYS_CLOCK_TICKS_PER_SEC)
	return cycles;
#elif defined(TIMER_CORE_RATE_IS_CONSTANT)
	return TIMER_CORE_CYCLES_TO_TICKS(cycles);
#else
	return (cycles * (uint64_t)CONFIG_SYS_CLOCK_TICKS_PER_SEC)
		/ (uint64_t)sys_clock_hw_cycles_per_sec();
#endif
}

#endif /* ZEPHYR_DRIVERS_TIMER_TIMER_CORE_CONVERT_H_ */
