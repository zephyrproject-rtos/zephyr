/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <kernel.h>
#include <errno.h>
#include <posix/time.h>
#include <posix/sys/types.h>

/**
 * @brief Get clock time specified by clock_id.
 *
 * See IEEE 1003.1
 */
int clock_gettime(clockid_t clock_id, struct timespec *ts)
{
	u64_t elapsed_msecs;

	if (clock_id != CLOCK_MONOTONIC) {
		errno = EINVAL;
		return -1;
	}

	elapsed_msecs = k_uptime_get();
	ts->tv_sec = (s32_t) (elapsed_msecs / MSEC_PER_SEC);
	ts->tv_nsec = (s32_t) ((elapsed_msecs % MSEC_PER_SEC) *
					USEC_PER_MSEC * NSEC_PER_USEC);

	return 0;
}

/**
 * @brief Get current real time.
 *
 * See IEEE 1003.1
 */
int gettimeofday(struct timeval *tv, const void *tz)
{
	struct timespec ts;
	int res;

	/* As per POSIX, "if tzp is not a null pointer, the behavior
	 * is unspecified."  "tzp" is the "tz" parameter above. */
	ARG_UNUSED(tv);

	res = clock_gettime(CLOCK_REALTIME, &ts);
	tv->tv_sec = ts.tv_sec;
	tv->tv_usec = ts.tv_nsec / NSEC_PER_USEC;

	return res;
}
