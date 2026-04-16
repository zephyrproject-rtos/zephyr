/*
 * Copyright (c) 2024, Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <time.h>

#include <zephyr/kernel.h>
#include <zephyr/posix/sys/times.h>
#include <zephyr/posix/unistd.h>
#include <zephyr/sys/clock.h>
#include <zephyr/sys/time_units.h>
#include <zephyr/sys/util.h>

pid_t getpid(void)
{
	/*
	 * To maintain compatibility with some other POSIX operating systems,
	 * a PID of zero is used to indicate that the process exists in another namespace.
	 * PID zero is also used by the scheduler in some cases.
	 * PID one is usually reserved for the init process.
	 * Also note, that negative PIDs may be used by kill()
	 * to send signals to process groups in some implementations.
	 *
	 * At the moment, getpid just returns an arbitrary number >= 2
	 */

	return 42;
}
#ifdef CONFIG_POSIX_MULTI_PROCESS_ALIAS_GETPID
FUNC_ALIAS(getpid, _getpid, pid_t);
#endif /* CONFIG_POSIX_MULTI_PROCESS_ALIAS_GETPID */

clock_t times(struct tms *buffer)
{
	int ret;
	clock_t utime; /* user time */
	k_thread_runtime_stats_t stats;

	ret = k_thread_runtime_stats_all_get(&stats);
	if (ret < 0) {
		errno = -ret;
		return (clock_t)-1;
	}

	utime = z_tmcvt(stats.total_cycles, sys_clock_hw_cycles_per_sec(), USEC_PER_SEC,
			IS_ENABLED(CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME) ? false : true,
			sizeof(clock_t) == sizeof(uint32_t), false, false);

	*buffer = (struct tms){
		.tms_utime = utime,
		.tms_stime = 0,
		.tms_cutime = 0,
		.tms_cstime = 0,
	};

	return utime;
}
