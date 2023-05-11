/*
 * Copyright (c) 2023 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <time.h>

#include <zephyr/kernel.h>

#ifndef CLOCKS_PER_SEC
#error libc does not define CLOCKS_PER_SEC which is #required by ISO
#endif

clock_t clock(void)
{
#ifdef CONFIG_SCHED_THREAD_USAGE_ALL
	int ret;
	k_thread_runtime_stats_t stats;

	ret = k_thread_runtime_stats_all_get(&stats);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}

	if (IS_ENABLED(CONFIG_SMP)) {
		/* total times include the time executed on each core */
		stats.total_cycles /= CONFIG_MP_MAX_NUM_CPUS;
		stats.idle_cycles /= CONFIG_MP_MAX_NUM_CPUS;
	}

	return z_tmcvt(stats.total_cycles - stats.idle_cycles, sys_clock_hw_cycles_per_sec(),
		       CLOCKS_PER_SEC,
		       IS_ENABLED(CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME) ? false : true,
		       sizeof(clock_t) == sizeof(uint32_t), false, false);
#endif

	/*
	 * Without a more accurate alternative, return uptime. This implies that the application
	 * is using 100% of CPU cycles, which is likely not accurate, but at least the result is
	 * consistent with a monotonically increasing clock count.
	 */
	return z_tmcvt(k_uptime_ticks(), sys_clock_hw_cycles_per_sec(), CLOCKS_PER_SEC, true,
		       sizeof(clock_t) == sizeof(uint32_t), false, false);
}
