/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "posix_internal.h"

#include <zephyr/kernel.h>
#include <ksched.h>
#include <zephyr/posix/time.h>
#include <zephyr/sys/util.h>

uint32_t timespec_to_timeoutms(clockid_t clock, const struct timespec *abstime)
{
	struct timespec curtime;

	if (clock_gettime(clock, &curtime) < 0) {
		return 0;
	}

	return CLAMP(tp_diff(abstime, &curtime) / NSEC_PER_MSEC, 0, INT32_MAX);
}
