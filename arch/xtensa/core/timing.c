/* Copyright (c) 2022 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>

void arch_timing_init(void)
{
}

void arch_timing_start(void)
{
}

void arch_timing_stop(void)
{
}

uint64_t arch_timing_freq_get(void)
{
	return CONFIG_XTENSA_CCOUNT_HZ;
}

timing_t arch_timing_counter_get(void)
{
	uint32_t ccount;

	__asm__ volatile ("rsr %0, CCOUNT" : "=r"(ccount));

	return ccount;
}

uint64_t arch_timing_cycles_get(volatile timing_t *const start,
				volatile timing_t *const end)
{
	int64_t dt = (int64_t) (*end - *start);

	if (dt < 0) {
		dt += 0x100000000ULL;
	}

	return (uint64_t) dt;
}

uint64_t arch_timing_cycles_to_ns(uint64_t cycles)
{
	return cycles * 1000000000ULL / CONFIG_XTENSA_CCOUNT_HZ;
}

uint64_t arch_timing_cycles_to_ns_avg(uint64_t cycles, uint32_t count)
{
	/* Why is this an arch API?  This is just math! */
	return arch_timing_cycles_to_ns(cycles) / (uint64_t) count;
}

uint32_t arch_timing_freq_get_mhz(void)
{
	return arch_timing_freq_get() / 1000000ULL;
}
