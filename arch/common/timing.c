/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <sys_clock.h>
#include <timing/timing.h>

void timing_init(void)
{
}

void timing_start(void)
{
}

void timing_stop(void)
{
}

timing_t timing_counter_get(void)
{
	return k_cycle_get_32();
}

uint64_t timing_cycles_get(volatile timing_t *const start,
				  volatile timing_t *const end)
{
	return (*end - *start);
}


uint64_t timing_freq_get(void)
{
	return sys_clock_hw_cycles_per_sec();
}

uint64_t timing_cycles_to_ns(uint64_t cycles)
{
	return k_cyc_to_ns_floor64(cycles);
}

uint64_t timing_cycles_to_ns_avg(uint64_t cycles, uint32_t count)
{
	return timing_cycles_to_ns(cycles) / count;
}

uint32_t timing_freq_get_mhz(void)
{
	return (uint32_t)(timing_freq_get() / 1000000);
}
