/*
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2025 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <pthread.h>
#include <time.h>
#include <errno.h>

#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#define SELECT_NANOSLEEP       1
#define SELECT_CLOCK_NANOSLEEP 0

void common_lower_bound_check(int selection, clockid_t clock_id, int flags, const uint32_t s,
			      uint32_t ns);
int select_nanosleep(int selection, clockid_t clock_id, int flags, const struct timespec *rqtp,
		     struct timespec *rmtp);

ZTEST(posix_clock_selection, test_clock_nanosleep_execution)
{
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);

	/* absolute sleeps with the monotonic clock and reference time ts */

	/* until 1s + 1ns past the reference time */
	common_lower_bound_check(SELECT_CLOCK_NANOSLEEP, CLOCK_MONOTONIC, TIMER_ABSTIME,
				 ts.tv_sec + 1, 1);

	/* until 1s + 1us past the reference time */
	common_lower_bound_check(SELECT_CLOCK_NANOSLEEP, CLOCK_MONOTONIC, TIMER_ABSTIME,
				 ts.tv_sec + 1, 1000);

	/* until 1s + 500000000ns past the reference time */
	common_lower_bound_check(SELECT_CLOCK_NANOSLEEP, CLOCK_MONOTONIC, TIMER_ABSTIME,
				 ts.tv_sec + 1, 500000000);

	/* until 2s past the reference time */
	common_lower_bound_check(SELECT_CLOCK_NANOSLEEP, CLOCK_MONOTONIC, TIMER_ABSTIME,
				 ts.tv_sec + 2, 0);

	/* until 2s + 1ns past the reference time */
	common_lower_bound_check(SELECT_CLOCK_NANOSLEEP, CLOCK_MONOTONIC, TIMER_ABSTIME,
				 ts.tv_sec + 2, 1);

	/* until 2s + 1us + 1ns past reference time */
	common_lower_bound_check(SELECT_CLOCK_NANOSLEEP, CLOCK_MONOTONIC, TIMER_ABSTIME,
				 ts.tv_sec + 2, 1001);

	clock_gettime(CLOCK_REALTIME, &ts);

	/* absolute sleeps with the real time clock and adjusted reference time ts */

	/* until 1s + 1ns past the reference time */
	common_lower_bound_check(SELECT_CLOCK_NANOSLEEP, CLOCK_REALTIME, TIMER_ABSTIME,
				 ts.tv_sec + 1, 1);

	/* until 1s + 1us past the reference time */
	common_lower_bound_check(SELECT_CLOCK_NANOSLEEP, CLOCK_REALTIME, TIMER_ABSTIME,
				 ts.tv_sec + 1, 1000);

	/* until 1s + 500000000ns past the reference time */
	common_lower_bound_check(SELECT_CLOCK_NANOSLEEP, CLOCK_REALTIME, TIMER_ABSTIME,
				 ts.tv_sec + 1, 500000000);

	/* until 2s past the reference time */
	common_lower_bound_check(SELECT_CLOCK_NANOSLEEP, CLOCK_REALTIME, TIMER_ABSTIME,
				 ts.tv_sec + 2, 0);

	/* until 2s + 1ns past the reference time */
	common_lower_bound_check(SELECT_CLOCK_NANOSLEEP, CLOCK_REALTIME, TIMER_ABSTIME,
				 ts.tv_sec + 2, 1);

	/* until 2s + 1us + 1ns past the reference time */
	common_lower_bound_check(SELECT_CLOCK_NANOSLEEP, CLOCK_REALTIME, TIMER_ABSTIME,
				 ts.tv_sec + 2, 1001);
}

ZTEST(posix_clock_selection, test_pthread_condattr_getclock)
{
	clockid_t clock_id;
	pthread_condattr_t att = {0};

	zassert_ok(pthread_condattr_init(&att));

	zassert_ok(pthread_condattr_getclock(&att, &clock_id), "pthread_condattr_getclock failed");
	zassert_equal(clock_id, CLOCK_REALTIME, "clock attribute not set correctly");

	zassert_ok(pthread_condattr_destroy(&att));
}

ZTEST(posix_clock_selection, test_pthread_condattr_setclock)
{
	clockid_t clock_id;
	pthread_condattr_t att = {0};

	zassert_ok(pthread_condattr_init(&att));

	zassert_ok(pthread_condattr_setclock(&att, CLOCK_MONOTONIC),
		   "pthread_condattr_setclock failed");

	zassert_ok(pthread_condattr_getclock(&att, &clock_id), "pthread_condattr_setclock failed");
	zassert_equal(clock_id, CLOCK_MONOTONIC, "clock attribute not set correctly");

	zassert_equal(pthread_condattr_setclock(&att, 42), EINVAL,
		      "pthread_condattr_setclock did not return EINVAL");
	zassert_equal(pthread_condattr_setclock(NULL, CLOCK_MONOTONIC), EINVAL,
		      "pthread_condattr_setclock did not return EINVAL");

	zassert_ok(pthread_condattr_destroy(&att));
}

ZTEST(posix_clock_selection, test_clock_nanosleep_errors)
{
	struct timespec ts;

	errno = 0;

	/* NULL rqtp */
	zassert_equal(clock_nanosleep(CLOCK_MONOTONIC, 0, NULL, NULL), EINVAL);
	zassert_equal(errno, 0, "clock_nanosleep modified errno");

	/* Negative tv_sec */
	ts = (struct timespec){.tv_sec = -1, .tv_nsec = 0};
	zassert_equal(clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, NULL), EINVAL);
	zassert_equal(errno, 0, "clock_nanosleep modified errno");

	/* Negative tv_nsec */
	ts = (struct timespec){.tv_sec = 0, .tv_nsec = -1};
	zassert_equal(clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, NULL), EINVAL);
	zassert_equal(errno, 0, "clock_nanosleep modified errno");

	/* tv_nsec >= 1000 million */
	ts = (struct timespec){.tv_sec = 0, .tv_nsec = 1000000000L};
	zassert_equal(clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, NULL), EINVAL);
	zassert_equal(errno, 0, "clock_nanosleep modified errno");

	/* Invalid flags */
	ts = (struct timespec){.tv_sec = 0, .tv_nsec = 1};
	zassert_equal(clock_nanosleep(CLOCK_MONOTONIC, ~TIMER_ABSTIME, &ts, NULL), EINVAL);
	zassert_equal(errno, 0, "clock_nanosleep modified errno");

	/* Invalid clock_id */
	zassert_equal(clock_nanosleep(42, 0, &ts, NULL), EINVAL);
	zassert_equal(errno, 0, "clock_nanosleep modified errno");
}

ZTEST_SUITE(posix_clock_selection, NULL, NULL, NULL, NULL, NULL);
