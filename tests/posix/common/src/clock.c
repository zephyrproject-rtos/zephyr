/*
 * Copyright (c) 2018 Intel Corporation
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

LOG_MODULE_REGISTER(clock_test);

#define _tp_op(_as, _ans, _bs, _bns, op)                                                           \
	((_as) * NSEC_PER_SEC + (_ans)op(_bs) * NSEC_PER_SEC + (_bns))

/* a == b */
static inline bool tp_eq(time_t a_sec, int64_t a_nsec, time_t b_sec, int64_t b_nsec)
{
	return _tp_op(a_sec, a_nsec, b_sec, b_nsec, ==);
}

/* a < b */
static inline bool tp_lt(time_t a_sec, int64_t a_nsec, time_t b_sec, int64_t b_nsec)
{
	return _tp_op(a_sec, a_nsec, b_sec, b_nsec, <);
}

/* a > b */
static inline bool tp_gt(time_t a_sec, int64_t a_nsec, time_t b_sec, int64_t b_nsec)
{
	return _tp_op(a_sec, a_nsec, b_sec, b_nsec, >);
}

/* a <= b */
static inline bool tp_le(time_t a_sec, int64_t a_nsec, time_t b_sec, int64_t b_nsec)
{
	return _tp_op(a_sec, a_nsec, b_sec, b_nsec, <=);
}

/* a >= b */
static inline bool tp_ge(time_t a_sec, int64_t a_nsec, time_t b_sec, int64_t b_nsec)
{
	return _tp_op(a_sec, a_nsec, b_sec, b_nsec, >=);
}

/* a - b */
static inline int64_t tp_diff_ns(time_t a_sec, int64_t a_nsec, time_t b_sec, int64_t b_nsec)
{
	return _tp_op((int64_t)a_sec, a_nsec, (int64_t)b_sec, b_nsec, -);
}

/* lo <= (a - b) < hi */
static inline bool tp_diff_in_range_ns(time_t a_sec, int64_t a_nsec, time_t b_sec, int64_t b_nsec,
				       int64_t lo, int64_t hi)
{
	int64_t diff = tp_diff_ns(a_sec, a_nsec, b_sec, b_nsec);

	return diff >= lo && diff < hi;
}

static inline int64_t ts_to_ns(const struct timespec *ts)
{
	return ts->tv_sec * NSEC_PER_SEC + ts->tv_nsec;
}

ZTEST(posix_apis, test_clock_gettime)
{
	clockid_t clocks[] = {
		CLOCK_MONOTONIC,
		CLOCK_REALTIME,
	};
	struct timespec ts;

	/* ensure argument validation is performed */
	errno = 0;
	zassert_equal(clock_gettime(CLOCK_INVALID, &ts), -1);
	zassert_equal(errno, EINVAL);

	if (false) {
		/* not hardened */
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

ZTEST(posix_apis, test_gettimeofday)
{
	struct timeval tv;
	struct timespec rts;

	if (false) {
		/* not hardened */
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
	zassert_true(tp_ge(rts.tv_sec, rts.tv_nsec, tv.tv_sec, tv.tv_usec * NSEC_PER_USEC));
}

ZTEST(posix_apis, test_clock_setttime)
{
	int64_t diff_ns;
	clockid_t clocks[] = {
		CLOCK_MONOTONIC,
		CLOCK_REALTIME,
	};
	bool settable[] = {
		false,
		true,
	};
	struct timespec ts = {0};

	BUILD_ASSERT(ARRAY_SIZE(settable) == ARRAY_SIZE(clocks));

	/* ensure argument validation is performed */
	errno = 0;
	zassert_equal(clock_settime(CLOCK_INVALID, &ts), -1);
	zassert_equal(errno, EINVAL);

	if (false) {
		/* not hardened */
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

		/* Set a particular time.  In this case, the output of:
		 * `date +%s -d 2018-01-01T15:45:01Z`
		 */
		ts = (struct timespec){1514821501, NSEC_PER_SEC / 2U};
		zassert_ok(clock_settime(clocks[i], &ts));

		/* read-back the time */
		zassert_ok(clock_gettime(clocks[i], &ts));
		/* dt should be >= 0, but definitely <= 1s */
		diff_ns = tp_diff_ns(ts.tv_sec, ts.tv_nsec, 1514821501, NSEC_PER_SEC / 2U);
		zassert_true(diff_ns >= 0 && diff_ns <= NSEC_PER_SEC);
	}
}

ZTEST(posix_apis, test_realtime)
{
	int64_t delta;
	int64_t error;
	int64_t last_delta = 0;
	struct timespec rts = {0};

	/* This test fails way too frequently on Qemu and POSIX platforms */
	if (IS_ENABLED(CONFIG_QEMU_TARGET) || IS_ENABLED(CONFIG_ARCH_POSIX)) {
		ztest_test_skip();
	}

	/*
	 * Loop 20 times, sleeping a little bit for each, making sure
	 * that the arithmetic roughly makes sense.  This tries to
	 * catch all of the boundary conditions of the clock to make
	 * sure there are no errors in the arithmetic.
	 */
	zassert_ok(clock_gettime(CLOCK_REALTIME, &rts), "Fail to set realtime clock");
	for (int i = 1; i <= 20; i++) {
		zassert_ok(k_usleep(USEC_PER_MSEC * 90U));
		zassert_ok(clock_gettime(CLOCK_REALTIME, &rts), "Fail to read realtime clock");

		delta = tp_diff_ns(rts.tv_sec, rts.tv_nsec, 0, 0);

		/* Make the delta milliseconds. */
		delta /= (NSEC_PER_SEC / 1000U);

		zassert_true(delta > last_delta, "Clock moved backward");
		error = delta - last_delta;

		LOG_DBG("i: %d, delta: %lld, error: %lld", i, delta, error);

		/* Allow for a little drift upward, but not downward */
		zassert_true(error >= 90, "Clock inaccurate %d", error);
		zassert_true(error <= 110, "Clock inaccurate %d", error);

		last_delta = delta;
	}
}
