/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "posix_clock.h"

#include <limits.h>
#include <stdint.h>

#include <zephyr/posix/time.h>
#include <zephyr/sys/util.h>

uint32_t timespec_to_timeoutms(clockid_t clock_id, const struct timespec *abstime)
{
	struct timespec curtime;

	if (clock_gettime(clock_id, &curtime) < 0) {
		return 0;
	}

	return CLAMP(tp_diff(abstime, &curtime) / NSEC_PER_MSEC, 0, UINT32_MAX);
}
