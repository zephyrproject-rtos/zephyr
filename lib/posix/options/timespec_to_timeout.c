/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "posix_clock.h"

#include <zephyr/kernel.h>
#include <ksched.h>
#include <zephyr/posix/time.h>

uint32_t timespec_to_clock_timeoutms(clockid_t clock_id, const struct timespec *abstime)
{
	int64_t milli_secs, secs, nsecs;
	struct timespec curtime;

	clock_gettime(clock_id, &curtime);
	secs = abstime->tv_sec - curtime.tv_sec;
	nsecs = abstime->tv_nsec - curtime.tv_nsec;

	if (secs < 0 || (secs == 0 && nsecs < NSEC_PER_MSEC)) {
		milli_secs = 0;
	} else {
		milli_secs =  secs * MSEC_PER_SEC + nsecs / NSEC_PER_MSEC;
	}

	return milli_secs;
}

uint32_t timespec_to_timeoutms(const struct timespec *abstime)
{
	return timespec_to_clock_timeoutms(CLOCK_MONOTONIC, abstime);
}
