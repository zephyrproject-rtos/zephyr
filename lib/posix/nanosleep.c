/*
 * Copyright (c) 2018 Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/kernel.h>
#include <limits.h>
#include <errno.h>
/* required for struct timespec */
#include <zephyr/posix/time.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys_clock.h>

/**
 * @brief Suspend execution for nanosecond intervals.
 *
 * See IEEE 1003.1
 */
int nanosleep(const struct timespec *rqtp, struct timespec *rmtp)
{
	return clock_nanosleep(CLOCK_MONOTONIC, 0, rqtp, rmtp);
}
