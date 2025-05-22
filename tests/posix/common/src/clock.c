/*
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2023, Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include <zephyr/ztest.h>
#include <zephyr/logging/log.h>

static inline int64_t ts_to_ns(const struct timespec *ts)
{
	return ts->tv_sec * NSEC_PER_SEC + ts->tv_nsec;
}

static inline void tv_to_ts(const struct timeval *tv, struct timespec *ts)
{
	ts->tv_sec = tv->tv_sec;
	ts->tv_nsec = tv->tv_usec * NSEC_PER_USEC;
}

#define _tp_op(_a, _b, _op) (ts_to_ns(_a) _op ts_to_ns(_b))

#define _decl_op(_type, _name, _op)                                                                \
	static inline _type _name(const struct timespec *_a, const struct timespec *_b)            \
	{                                                                                          \
		return _tp_op(_a, _b, _op);                                                        \
	}

_decl_op(bool, tp_ge, >=); /* a >= b */

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
