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

#define SLEEP_SECONDS 1
#define CLOCK_INVALID -1

LOG_MODULE_REGISTER(clock_test, LOG_LEVEL_DBG);

/* Set a particular time.  In this case, the output of: `date +%s -d 2018-01-01T15:45:01Z` */
static const struct timespec ref_ts = {1514821501, NSEC_PER_SEC / 2U};

static const clockid_t clocks[] = {
	CLOCK_MONOTONIC,
	CLOCK_REALTIME,
};
static const bool settable[] = {
	false,
	true,
};

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

_decl_op(bool, tp_eq, ==);     /* a == b */
_decl_op(bool, tp_lt, <);      /* a < b */
_decl_op(bool, tp_gt, >);      /* a > b */
_decl_op(bool, tp_le, <=);     /* a <= b */
_decl_op(bool, tp_ge, >=);     /* a >= b */
_decl_op(int64_t, tp_diff, -); /* a - b */

/* lo <= (a - b) < hi */
static inline bool tp_diff_in_range_ns(const struct timespec *a, const struct timespec *b,
				       int64_t lo, int64_t hi)
{
	int64_t diff = tp_diff(a, b);

	return diff >= lo && diff < hi;
}

ZTEST(clock, test_clock_gettime)
{
	struct timespec ts;

	/* ensure argument validation is performed */
	errno = 0;
	zassert_equal(clock_gettime(CLOCK_INVALID, &ts), -1);
	zassert_equal(errno, EINVAL);

	if (false) {
		/* undefined behaviour */
		errno = 0;
		zassert_equal(clock_gettime(clocks[0], NULL), -1);
		zassert_equal(errno, EINVAL);
	}

	/* verify that we can call clock_gettime() on supported clocks */
	ARRAY_FOR_EACH(clocks, i)
	{
		ts = (struct timespec){-1, -1};
		zassert_ok(clock_gettime(clocks[i], &ts));
		zassert_not_equal(ts.tv_sec, -1);
		zassert_not_equal(ts.tv_nsec, -1);
	}
}

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

ZTEST(clock, test_clock_settime)
{
	int64_t diff_ns;
	struct timespec ts = {0};

	BUILD_ASSERT(ARRAY_SIZE(settable) == ARRAY_SIZE(clocks));

	/* ensure argument validation is performed */
	errno = 0;
	zassert_equal(clock_settime(CLOCK_INVALID, &ts), -1);
	zassert_equal(errno, EINVAL);

	if (false) {
		/* undefined behaviour */
		errno = 0;
		zassert_equal(clock_settime(CLOCK_REALTIME, NULL), -1);
		zassert_equal(errno, EINVAL);
	}

	/* verify nanoseconds */
	errno = 0;
	ts = (struct timespec){0, NSEC_PER_SEC};
	zassert_equal(clock_settime(CLOCK_REALTIME, &ts), -1);
	zassert_equal(errno, EINVAL);
	errno = 0;
	ts = (struct timespec){0, -1};
	zassert_equal(clock_settime(CLOCK_REALTIME, &ts), -1);
	zassert_equal(errno, EINVAL);

	ARRAY_FOR_EACH(clocks, i)
	{
		if (!settable[i]) {
			/* should fail attempting to set unsettable clocks */
			errno = 0;
			zassert_equal(clock_settime(clocks[i], &ts), -1);
			zassert_equal(errno, EINVAL);
			continue;
		}

		zassert_ok(clock_settime(clocks[i], &ref_ts));

		/* read-back the time */
		zassert_ok(clock_gettime(clocks[i], &ts));
		/* dt should be >= 0, but definitely <= 1s */
		diff_ns = tp_diff(&ts, &ref_ts);
		zassert_true(diff_ns >= 0 && diff_ns <= NSEC_PER_SEC);
	}
}

ZTEST(clock, test_realtime)
{
	struct timespec then, now;
	/*
	 * For calculating cumulative moving average
	 * Note: we do not want to assert any individual samples due to scheduler noise.
	 * The CMA filters out the noise so we can make an assertion (on average).
	 * https://en.wikipedia.org/wiki/Moving_average#Cumulative_moving_average
	 */
	int64_t cma_prev = 0;
	int64_t cma;
	int64_t x_i;
	/* lower and uppoer boundary for assertion */
	int64_t lo = CONFIG_TEST_CLOCK_RT_SLEEP_MS;
	int64_t hi = CONFIG_TEST_CLOCK_RT_SLEEP_MS + CONFIG_TEST_CLOCK_RT_ERROR_MS;
	/* lower and upper watermark */
	int64_t lo_wm = INT64_MAX;
	int64_t hi_wm = INT64_MIN;

	/* Loop n times, sleeping a little bit for each */
	(void)clock_gettime(CLOCK_REALTIME, &then);
	for (int i = 0; i < CONFIG_TEST_CLOCK_RT_ITERATIONS; ++i) {

		zassert_ok(k_usleep(USEC_PER_MSEC * CONFIG_TEST_CLOCK_RT_SLEEP_MS));
		(void)clock_gettime(CLOCK_REALTIME, &now);

		/* Make the delta milliseconds. */
		x_i = tp_diff(&now, &then) / NSEC_PER_MSEC;
		then = now;

		if (x_i < lo_wm) {
			/* update low watermark */
			lo_wm = x_i;
		}

		if (x_i > hi_wm) {
			/* update high watermark */
			hi_wm = x_i;
		}

		/* compute cumulative running average */
		cma = (x_i + i * cma_prev) / (i + 1);
		cma_prev = cma;
	}

	LOG_INF("n: %d, sleep: %d, margin: %d, lo: %lld, avg: %lld, hi: %lld",
		CONFIG_TEST_CLOCK_RT_ITERATIONS, CONFIG_TEST_CLOCK_RT_SLEEP_MS,
		CONFIG_TEST_CLOCK_RT_ERROR_MS, lo_wm, cma, hi_wm);
	zassert_between_inclusive(cma, lo, hi);
}

ZTEST(clock, test_clock_getcpuclockid)
{
	int ret = 0;
	clockid_t clock_id = CLOCK_INVALID;

	ret = clock_getcpuclockid((pid_t)0, &clock_id);
	zassert_equal(ret, 0, "POSIX clock_getcpuclock id failed");
	zassert_equal(clock_id, CLOCK_PROCESS_CPUTIME_ID, "POSIX clock_getcpuclock id failed");

	ret = clock_getcpuclockid((pid_t)2482, &clock_id);
	zassert_equal(ret, EPERM, "POSIX clock_getcpuclock id failed");
}

ZTEST_SUITE(clock, NULL, NULL, NULL, NULL, NULL);
