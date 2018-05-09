/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <ksched.h>
#include <wait_q.h>
#include <posix/pthread.h>
#include <posix/time.h>

s64_t timespec_to_timeoutms(const struct timespec *abstime)
{
	s64_t milli_secs, secs, nsecs;
	struct timespec curtime;

	/* FIXME: Zephyr does have CLOCK_REALTIME to get time.
	 * As per POSIX standard time should be calculated wrt CLOCK_REALTIME.
	 * Zephyr deviates from POSIX 1003.1 standard on this aspect.
	 */
	clock_gettime(CLOCK_MONOTONIC, &curtime);
	secs = abstime->tv_sec - curtime.tv_sec;
	nsecs = abstime->tv_nsec - curtime.tv_nsec;

	if (secs < 0 || (secs == 0 && nsecs < NSEC_PER_MSEC)) {
		milli_secs = K_NO_WAIT;
	} else {
		milli_secs =  secs * MSEC_PER_SEC + nsecs / NSEC_PER_MSEC;
	}

	return milli_secs;
}
