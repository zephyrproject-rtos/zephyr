/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "posix_clock.h"

#include <limits.h>
#include <stdint.h>
#include <time.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/clock.h>
#include <zephyr/sys/minmax.h>
#include <zephyr/sys/timeutil.h>
#include <zephyr/sys/util.h>

uint32_t timespec_to_timeoutms(int clock_id, const struct timespec *abstime)
{
	struct timespec curtime;

	if (sys_clock_gettime(sys_clock_from_clockid(clock_id), &curtime) < 0) {
		return 0;
	}

	return clamp(tp_diff(abstime, &curtime) / NSEC_PER_MSEC, 0, UINT32_MAX);
}

k_timeout_t timespec_abs_to_timeout(int clock_id, const struct timespec *abstime)
{
	struct timespec delta = *abstime;
	struct timespec curtime;

	if (sys_clock_gettime(sys_clock_from_clockid(clock_id), &curtime) < 0) {
		return K_NO_WAIT;
	}

	if (!timespec_sub(&delta, &curtime)) {
		/* unreachable for valid clock values; saturate */
		return (k_timeout_t){.ticks = K_TICK_MAX};
	}

	/* Negative deltas (abstime in the past) become K_NO_WAIT; positive
	 * deltas are rounded up to the next tick boundary so the wait can
	 * never end before abstime.
	 */
	return timespec_to_timeout(&delta, NULL);
}
