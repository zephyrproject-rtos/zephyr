/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <time.h>
#include <unistd.h>

#include <zephyr/ztest.h>

#define SECS_TO_SLEEP 2
#define DURATION_SECS 1
#define DURATION_NSECS 0
#define PERIOD_SECS 0
#define PERIOD_NSECS 100000000

static int exp_count;

void handler(union sigval val)
{
	printk("Handler Signal value :%d for %d times\n", val.sival_int,
	       ++exp_count);
}

void test_timer(int sigev_notify)
{
	int ret;
	struct sigevent sig = { 0 };
	timer_t timerid;
	struct itimerspec value, ovalue;
	struct timespec ts, te;
	int64_t nsecs_elapsed, secs_elapsed;

	exp_count = 0;
	sig.sigev_notify = sigev_notify;
	sig.sigev_notify_function = handler;
	sig.sigev_value.sival_int = 20;

	if (sigev_notify == SIGEV_SIGNAL)
		printk("POSIX timer test SIGEV_SIGNAL\n");
	else
		printk("POSIX timer test SIGEV_THREAD\n");

	ret = timer_create(CLOCK_MONOTONIC, &sig, &timerid);

	/*TESTPOINT: Check if timer is created successfully*/
	zassert_false(ret, "POSIX timer create failed");

	value.it_value.tv_sec = DURATION_SECS;
	value.it_value.tv_nsec = DURATION_NSECS;
	value.it_interval.tv_sec = PERIOD_SECS;
	value.it_interval.tv_nsec = PERIOD_NSECS;
	ret = timer_settime(timerid, 0, &value, &ovalue);
	usleep(100 * USEC_PER_MSEC);
	ret = timer_gettime(timerid, &value);
	zassert_false(ret, "Failed to get time to expire.");

	if (ret == 0) {
		printk("Timer fires every %d secs and  %d nsecs\n",
		       (int) value.it_interval.tv_sec,
		       (int) value.it_interval.tv_nsec);
		printk("Time remaining to fire %d secs and  %d nsecs\n",
		       (int) value.it_value.tv_sec,
		       (int) value.it_value.tv_nsec);
	}

	clock_gettime(CLOCK_MONOTONIC, &ts);

	/*TESTPOINT: Check if timer has started successfully*/
	zassert_false(ret, "POSIX timer failed to start");

	sleep(SECS_TO_SLEEP);

	clock_gettime(CLOCK_MONOTONIC, &te);
	zassert_equal(ret, 0, "Number of timer overruns is incorrect");
	timer_delete(timerid);

	if (te.tv_nsec >= ts.tv_nsec) {
		secs_elapsed = te.tv_sec - ts.tv_sec;
		nsecs_elapsed = te.tv_nsec - ts.tv_nsec;
	} else {
		nsecs_elapsed = NSEC_PER_SEC + te.tv_nsec - ts.tv_nsec;
		secs_elapsed = (te.tv_sec - ts.tv_sec - 1);
	}

	uint64_t elapsed = secs_elapsed*NSEC_PER_SEC + nsecs_elapsed;
	uint64_t first_sig = value.it_value.tv_sec * NSEC_PER_SEC + value.it_value.tv_nsec;
	uint64_t sig_interval = value.it_interval.tv_sec * NSEC_PER_SEC + value.it_interval.tv_nsec;
	int expected_signal_count = (elapsed - first_sig) / sig_interval + 1;

	/*TESTPOINT: Check if POSIX timer test passed*/
	zassert_within(exp_count, expected_signal_count, 1,
		       "POSIX timer test has failed %i != %i",
		       exp_count, expected_signal_count);
}

ZTEST(timer, test_timer)
{
	test_timer(SIGEV_SIGNAL);
	test_timer(SIGEV_THREAD);
}

ZTEST(timer, test_timer_overrun)
{
	timer_t timerid;
	struct sigevent sig = { 0 };
	struct itimerspec value;

	sig.sigev_notify = SIGEV_NONE;

	timer_create(CLOCK_MONOTONIC, &sig, &timerid);

	/*Set the timer to expire every 500 milliseconds*/
	value.it_interval.tv_sec = 0;
	value.it_interval.tv_nsec = 500000000;
	value.it_value.tv_sec = 0;
	value.it_value.tv_nsec = 500000000;
	timer_settime(timerid, 0, &value, NULL);
	k_sleep(K_MSEC(2500));

	int overruns = timer_getoverrun(timerid);

	timer_delete(timerid);
	zassert_equal(overruns, 4, "Number of overruns is incorrect");
}

ZTEST_SUITE(timer, NULL, NULL, NULL, NULL, NULL);
