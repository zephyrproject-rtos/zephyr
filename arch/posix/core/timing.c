/*
 * Copyright (c) 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys_clock.h>
#include <zephyr/timing/timing.h>

static struct timespec resolution;
static bool timing_initialized;

void arch_timing_init(void)
{
	int rc = clock_getres(CLOCK_MONOTONIC, &resolution);

	timing_initialized = rc == 0;
}

void arch_timing_start(void)
{
}

void arch_timing_stop(void)
{
}

timing_t arch_timing_counter_get(void)
{
	struct timespec t = {0};

	if (timing_initialized) {
		clock_gettime(CLOCK_MONOTONIC, &t);
	}

	return t;
}

uint64_t arch_timing_cycles_get(volatile timing_t *const start,
				volatile timing_t *const end)
{
	return ((end->tv_sec - start->tv_sec) * UINT64_C(1000000000)) +
	       (end->tv_nsec - start->tv_nsec);
}

uint64_t arch_timing_freq_get(void)
{
	if (!timing_initialized) {
		return 0;
	}
	return (resolution.tv_sec * UINT64_C(1000000000)) + resolution.tv_nsec;
}

uint64_t arch_timing_cycles_to_ns(uint64_t cycles)
{
	return cycles;
}

uint64_t arch_timing_cycles_to_ns_avg(uint64_t cycles, uint32_t count)
{
	return cycles / count;
}

uint32_t arch_timing_freq_get_mhz(void)
{
	return (uint32_t)MIN(UINT32_MAX, arch_timing_freq_get() * UINT64_C(1000));
}
