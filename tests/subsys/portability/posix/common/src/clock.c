/*
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2023, Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "posix_clock.h"

#include <limits.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include <zephyr/ztest.h>
#include <zephyr/logging/log.h>

ZTEST(clock, test_gettimeofday)
{
	struct timeval tv;
	struct timespec ts;
	struct timespec rts;

	if (false) {
		/* undefined behaviour */
		errno = 0;
		zassert_equal(gettimeofday(NULL, NULL), -1);
		zassert_equal(errno, EINVAL);
	}

	/* Validate gettimeofday API */
	zassert_ok(gettimeofday(&tv, NULL));
	zassert_ok(clock_gettime(CLOCK_REALTIME, &rts));

	/* TESTPOINT: Check if time obtained from
	 * gettimeofday is same or more than obtained
	 * from clock_gettime
	 */
	tv_to_ts(&tv, &ts);
	zassert_true(tp_ge(&rts, &ts));
}

ZTEST_SUITE(clock, NULL, NULL, NULL, NULL, NULL);
