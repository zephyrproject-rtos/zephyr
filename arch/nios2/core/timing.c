/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys_clock.h>
#include <zephyr/timing/timing.h>
#include "altera_avalon_timer_regs.h"

#define NIOS2_SUBTRACT_CLOCK_CYCLES(val)                                       \
	((IORD_ALTERA_AVALON_TIMER_PERIODH(TIMER_0_BASE) << 16 |               \
	  (IORD_ALTERA_AVALON_TIMER_PERIODL(TIMER_0_BASE))) -                  \
	 ((uint32_t)val))

#define TIMING_INFO_OS_GET_TIME()                                              \
	(NIOS2_SUBTRACT_CLOCK_CYCLES(                                          \
		((uint32_t)IORD_ALTERA_AVALON_TIMER_SNAPH(TIMER_0_BASE)        \
		 << 16) |                                                      \
		((uint32_t)IORD_ALTERA_AVALON_TIMER_SNAPL(TIMER_0_BASE))))

void arch_timing_init(void)
{
}

void arch_timing_start(void)
{
}

void arch_timing_stop(void)
{
}

timing_t arch_timing_counter_get(void)
{
	IOWR_ALTERA_AVALON_TIMER_SNAPL(TIMER_0_BASE, 10);
	return TIMING_INFO_OS_GET_TIME();
}

uint64_t arch_timing_cycles_get(volatile timing_t *const start,
				volatile timing_t *const end)
{
	timing_t start_ = *start;
	timing_t end_ = *end;

	if (end_ >= start_) {
		return (end_ - start_);
	}
	return (end_ + NIOS2_SUBTRACT_CLOCK_CYCLES(start_));
}

uint64_t arch_timing_freq_get(void)
{
	return sys_clock_hw_cycles_per_sec();
}

uint64_t arch_timing_cycles_to_ns(uint64_t cycles)
{
	return k_cyc_to_ns_floor64(cycles);
}

uint64_t arch_timing_cycles_to_ns_avg(uint64_t cycles, uint32_t count)
{
	return arch_timing_cycles_to_ns(cycles) / count;
}

uint32_t arch_timing_freq_get_mhz(void)
{
	return (uint32_t)(arch_timing_freq_get() / 1000000U);
}
