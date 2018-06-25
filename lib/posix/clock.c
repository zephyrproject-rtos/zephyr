/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <kernel.h>
#include <errno.h>
#include <posix/time.h>
#include <posix/sys/types.h>

/*
 * `k_uptime_get` returns a timestamp based on an always increasing
 * value from the system start.  To support the `CLOCK_REALTIME`
 * clock, this `rt_clock_base` records the time that the system was
 * started.  This can either be set via 'clock_settime', or could be
 * set from a real time clock, if such hardware is present.
 */
static struct timespec rt_clock_base;

/**
 * @brief Get clock time specified by clock_id.
 *
 * See IEEE 1003.1
 */
int clock_gettime(clockid_t clock_id, struct timespec *ts)
{
	u64_t elapsed_msecs;
	struct timespec base;

	switch (clock_id) {
	case CLOCK_MONOTONIC:
		base.tv_sec = 0;
		base.tv_nsec = 0;
		break;

	case CLOCK_REALTIME:
		base = rt_clock_base;
		break;

	default:
		errno = EINVAL;
		return -1;
	}

	elapsed_msecs = k_uptime_get();
	ts->tv_sec = (s32_t) (elapsed_msecs / MSEC_PER_SEC);
	ts->tv_nsec = (s32_t) ((elapsed_msecs % MSEC_PER_SEC) *
					USEC_PER_MSEC * NSEC_PER_USEC);

	ts->tv_sec += base.tv_sec;
	ts->tv_nsec += base.tv_nsec;
	if (ts->tv_nsec > NSEC_PER_SEC) {
		ts->tv_sec++;
		ts->tv_nsec -= NSEC_PER_SEC;
	}

	return 0;
}

/**
 * @brief Set the time of the specified clock.
 *
 * See IEEE 1003.1.
 *
 * Note that only the `CLOCK_REALTIME` clock can be set using this
 * call.
 */
int clock_settime(clockid_t clock_id, const struct timespec *tp)
{
	struct timespec base;
	int res;

	if (clock_id != CLOCK_REALTIME) {
		errno = EINVAL;
		return -1;
	}

	res = clock_gettime(clock_id, &base);
	if (res != 0) {
		return res;
	}

	s64_t delta = (s64_t)NSEC_PER_SEC * (tp->tv_sec - base.tv_sec) +
		(tp->tv_nsec - base.tv_nsec);

	base.tv_sec = delta / NSEC_PER_SEC;
	base.tv_nsec = delta % NSEC_PER_SEC;

	rt_clock_base = base;

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
