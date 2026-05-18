/*
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2026 The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <time.h>
#include <errno.h>

#include "nsi_errno.h"
#include "ptp_clock_native_bottom.h"

int ptp_clock_native_gettime(uint64_t *second, uint32_t *nanosecond)
{
	struct timespec tp;
	int ret;

	ret = clock_gettime(CLOCK_MONOTONIC_RAW, &tp);
	if (ret < 0) {
		return -nsi_errno_to_mid(errno);
	}

	*second = (uint64_t)tp.tv_sec;
	*nanosecond = (uint32_t)tp.tv_nsec;

	return 0;
}
