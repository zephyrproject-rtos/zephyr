/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <ksched.h>
#include <zephyr/wait_q.h>
#include <zephyr/posix/time.h>

#ifdef CONFIG_POSIX_CLOCK
int64_t timespec_to_timeoutms(const struct timespec *abstime)
{
	int64_t milli_secs, secs, nsecs;
	struct timespec curtime;

	/* FIXME: Zephyr does have CLOCK_REALTIME to get time.
	 * As per POSIX standard time should be calculated wrt CLOCK_REALTIME.
	 * Zephyr deviates from POSIX 1003.1 standard on this aspect.
	 */
	clock_gettime(CLOCK_MONOTONIC, &curtime);
	secs = abstime->tv_sec - curtime.tv_sec;
	nsecs = abstime->tv_nsec - curtime.tv_nsec;

	if (secs < 0 || (secs == 0 && nsecs < NSEC_PER_MSEC)) {
		milli_secs = 0;
	} else {
		milli_secs =  secs * MSEC_PER_SEC + nsecs / NSEC_PER_MSEC;
	}

	return milli_secs;
}
#endif	/* CONFIG_POSIX_CLOCK */
