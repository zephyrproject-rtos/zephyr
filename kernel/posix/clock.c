/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <kernel.h>
#include <time.h>
#include <errno.h>

/**
 * @brief Get clock time specified by clock_id.
 *
 * See IEEE 1003.1
 */
int clock_gettime(clockid_t clock_id, struct timespec *ts)
{
	s64_t elapsed_msecs, elapsed_secs;
	s64_t elapsed_nsec, elapsed_cycles;

	if (clock_id != CLOCK_MONOTONIC) {
		errno = EINVAL;
		return -1;
	}

	elapsed_msecs = k_uptime_get();
	elapsed_secs = elapsed_msecs / MSEC_PER_SEC;

	elapsed_cycles = (s64_t) (k_cycle_get_32() %
				  sys_clock_hw_cycles_per_sec);
	elapsed_nsec = (s64_t) ((elapsed_cycles * NSEC_PER_SEC) /
				sys_clock_hw_cycles_per_sec);

	ts->tv_sec = (s32_t) elapsed_secs;
	ts->tv_nsec = (s32_t) elapsed_nsec;
	return 0;
}
