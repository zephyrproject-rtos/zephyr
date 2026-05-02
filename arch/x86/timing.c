/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/x86/arch.h>
#include <zephyr/kernel.h>
#include <zephyr/sys_clock.h>
#include <zephyr/timing/timing.h>
#include <zephyr/app_memory/app_memdomain.h>

K_APP_BMEM(z_libc_partition) static uint64_t tsc_freq;

void arch_timing_x86_init(void)
{
	uint32_t cyc_start, cyc_end;
	uint64_t tsc_start, tsc_end;
	uint64_t cyc_freq = sys_clock_hw_cycles_per_sec();
	uint64_t dcyc, dtsc;

	do {
		cyc_start = k_cycle_get_32();
		tsc_start = z_tsc_read();

		k_busy_wait(10 * USEC_PER_MSEC);

		cyc_end = k_cycle_get_32();
		tsc_end = z_tsc_read();

		/*
		 * cycles are in 32-bit, and delta must be
		 * calculated in 32-bit precision. Or it would be
		 * wrapping around in 64-bit.
		 */
		dcyc = (uint32_t)cyc_end - (uint32_t)cyc_start;
		dtsc = tsc_end - tsc_start;
	} while ((dcyc == 0) || (dtsc == 0));

	tsc_freq = (cyc_freq * dtsc) / dcyc;
}

uint64_t arch_timing_x86_freq_get(void)
{
	return tsc_freq;
}

void arch_timing_init(void)
{
	arch_timing_x86_init();
}

void arch_timing_start(void)
{
}

void arch_timing_stop(void)
{
}

timing_t arch_timing_counter_get(void)
{
	return z_tsc_read();
}

uint64_t arch_timing_cycles_get(volatile timing_t *const start,
				volatile timing_t *const end)
{
	return (*end - *start);
}


uint64_t arch_timing_freq_get(void)
{
	return arch_timing_x86_freq_get();
}

uint64_t arch_timing_cycles_to_ns(uint64_t cycles)
{
	return ((cycles) * NSEC_PER_SEC / tsc_freq);
}

uint64_t arch_timing_cycles_to_ns_avg(uint64_t cycles, uint32_t count)
{
	return arch_timing_cycles_to_ns(cycles) / count;
}

uint32_t arch_timing_freq_get_mhz(void)
{
	return (uint32_t)(arch_timing_freq_get() / 1000000U);
}
