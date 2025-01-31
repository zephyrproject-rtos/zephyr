/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys_clock.h>
#include <zephyr/timing/timing.h>

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
#if CONFIG_TIMER_HAS_64BIT_CYCLE_COUNTER
	return k_cycle_get_64();
#else
	return k_cycle_get_32();
#endif
}

uint64_t arch_timing_cycles_get(volatile timing_t *const start,
				volatile timing_t *const end)
{
#if CONFIG_TIMER_HAS_64BIT_CYCLE_COUNTER
	return (*end - *start);
#else
	return ((uint32_t)*end - (uint32_t)*start);
#endif
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
