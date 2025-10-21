/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "posix_clock.h"

#include <limits.h>
#include <stdint.h>
#include <time.h>

#include <zephyr/sys/clock.h>
#include <zephyr/sys/util.h>

uint32_t timespec_to_timeoutms(int clock_id, const struct timespec *abstime)
{
	struct timespec curtime;

	if (sys_clock_gettime(sys_clock_from_clockid(clock_id), &curtime) < 0) {
		return 0;
	}

	return clamp(tp_diff(abstime, &curtime) / NSEC_PER_MSEC, 0, UINT32_MAX);
}
