/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "_main.h"

#define SECS_TO_SLEEP  2
#define DURATION_SECS  1
#define DURATION_NSECS 0
#define PERIOD_SECS    0
#define PERIOD_NSECS   100000000

LOG_MODULE_REGISTER(timerfd_test);

void test_timerfd(struct timerfd_fixture *fixture, clockid_t clock_id, int flags)
{
	struct itimerspec value, ovalue;
	struct timespec ts, te;
	int64_t nsecs_elapsed, secs_elapsed;

	reopen(&fixture->fd, clock_id, flags);
	value.it_value.tv_sec = DURATION_SECS;
	value.it_value.tv_nsec = DURATION_NSECS;
	value.it_interval.tv_sec = PERIOD_SECS;
	value.it_interval.tv_nsec = PERIOD_NSECS;
	zassert_ok(timerfd_settime(fixture->fd, 0, &value, &ovalue));
	usleep(100 * USEC_PER_MSEC);
	/*TESTPOINT: Check if timer has started successfully*/
	zassert_ok(timerfd_gettime(fixture->fd, &value));

	LOG_DBG("Timer fires every %d secs and  %d nsecs", (int)value.it_interval.tv_sec,
		(int)value.it_interval.tv_nsec);
	LOG_DBG("Time remaining to fire %d secs and  %d nsecs", (int)value.it_value.tv_sec,
		(int)value.it_value.tv_nsec);

	clock_gettime(clock_id, &ts);
	sleep(SECS_TO_SLEEP);
	clock_gettime(clock_id, &te);

	if (te.tv_nsec >= ts.tv_nsec) {
		secs_elapsed = te.tv_sec - ts.tv_sec;
		nsecs_elapsed = te.tv_nsec - ts.tv_nsec;
	} else {
		nsecs_elapsed = NSEC_PER_SEC + te.tv_nsec - ts.tv_nsec;
		secs_elapsed = (te.tv_sec - ts.tv_sec - 1);
	}

	uint64_t elapsed = secs_elapsed * NSEC_PER_SEC + nsecs_elapsed;
	uint64_t first_sig = value.it_value.tv_sec * NSEC_PER_SEC + value.it_value.tv_nsec;
	uint64_t sig_interval = value.it_interval.tv_sec * NSEC_PER_SEC + value.it_interval.tv_nsec;
	uint64_t expected_signal_count = (elapsed - first_sig) / sig_interval + 1;

	/*TESTPOINT: Check if timerfd test passed*/
	uint64_t exp_count;
	zassert_equal(read(fixture->fd, &exp_count, sizeof(exp_count)), sizeof(exp_count));
	zassert_within(exp_count, expected_signal_count, 1, "timerfd test has failed %" PRIu64 " != %" PRIu64,
		       exp_count, expected_signal_count);
}

ZTEST_F(timerfd, test_CLOCK_REALTIME)
{
	test_timerfd(fixture, CLOCK_REALTIME, 0);
}

ZTEST_F(timerfd, test_CLOCK_MONOTONIC)
{
	test_timerfd(fixture, CLOCK_MONOTONIC, 0);
}

ZTEST_F(timerfd, test_timerfd_overrun)
{
	/* Set the timer to expire every 500 milliseconds */
	struct itimerspec ts = {
		.it_interval = {0, 500000000},
		.it_value = {0, 500000000}
	};

	reopen(&fixture->fd, CLOCK_MONOTONIC, 0);

	zassert_ok(timerfd_settime(fixture->fd, 0, &ts, NULL));
	k_sleep(K_MSEC(2000));

	uint64_t exp_count;
	zassert_equal(read(fixture->fd, &exp_count, sizeof(exp_count)), sizeof(exp_count));
	zassert_equal(exp_count, 4, "Number of overruns is incorrect");
}

ZTEST_F(timerfd, test_one_shot)
{
	/* Set the timer to expire only once */

	struct itimerspec ts = {
		.it_interval = {0},
		.it_value = {0, 100 * NSEC_PER_MSEC}
	};

	reopen(&fixture->fd, CLOCK_MONOTONIC, 0);

	zassert_ok(timerfd_settime(fixture->fd, 0, &ts, NULL));
	k_sleep(K_MSEC(300));

	uint64_t exp_count;
	zassert_equal(read(fixture->fd, &exp_count, sizeof(exp_count)), sizeof(exp_count));
	zassert_equal(exp_count, 1, "Number of expiry is incorrect");
}
