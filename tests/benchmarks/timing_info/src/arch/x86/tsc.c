/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <zephyr.h>
#include <tc_util.h>
#include <ksched.h>
#include <sys_clock.h>

#define CALIB_LOOPS	8

static uint64_t tsc_freq;

void calibrate_timer(void)
{
	uint32_t cyc_start = k_cycle_get_32();
	uint64_t tsc_start = z_tsc_read();

	k_busy_wait(10 * USEC_PER_MSEC);

	uint32_t cyc_end = k_cycle_get_32();
	uint64_t tsc_end = z_tsc_read();

	uint64_t cyc_freq = sys_clock_hw_cycles_per_sec();

	/*
	 * cycles are in 32-bit, and delta must be
	 * calculated in 32-bit percision. Or it would
	 * wrapping around in 64-bit.
	 */
	uint64_t dcyc = (uint32_t)cyc_end - (uint32_t)cyc_start;

	uint64_t dtsc = tsc_end - tsc_start;

	tsc_freq = (cyc_freq * dtsc) / dcyc;
}

uint32_t x86_get_timer_freq_MHz(void)
{
	return (uint32_t)(tsc_freq / 1000000);
}

uint32_t x86_cyc_to_ns_floor64(uint64_t cyc)
{
	return ((cyc) * USEC_PER_SEC / tsc_freq);
}
