/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <ztest.h>
#include <posix/time.h>
#include <posix/sys/time.h>
#include <posix/unistd.h>

#define SLEEP_SECONDS 1
#define CLOCK_INVALID -1

void test_posix_clock(void)
{
	int64_t nsecs_elapsed, secs_elapsed;
	struct timespec ts, te;

	printk("POSIX clock APIs\n");

	/* TESTPOINT: Pass invalid clock type */
	zassert_equal(clock_gettime(CLOCK_INVALID, &ts), -1,
			NULL);
	zassert_equal(errno, EINVAL, NULL);

	clock_gettime(CLOCK_MONOTONIC, &ts);
	/* 2 Sec Delay */
	sleep(SLEEP_SECONDS);
	usleep(SLEEP_SECONDS * USEC_PER_SEC);
	clock_gettime(CLOCK_MONOTONIC, &te);

	if (te.tv_nsec >= ts.tv_nsec) {
		secs_elapsed = te.tv_sec - ts.tv_sec;
		nsecs_elapsed = te.tv_nsec - ts.tv_nsec;
	} else {
		nsecs_elapsed = NSEC_PER_SEC + te.tv_nsec - ts.tv_nsec;
		secs_elapsed = (te.tv_sec - ts.tv_sec - 1);
	}

	/*TESTPOINT: Check if POSIX clock API test passes*/
	zassert_equal(secs_elapsed, (2 * SLEEP_SECONDS),
			"POSIX clock API test failed");

	printk("POSIX clock APIs test done\n");
}

void test_posix_realtime(void)
{
	/* Make sure the realtime and monotonic clocks start out the
	 * same.  This is not true on posix and where there is a
	 * realtime clock, so don't keep this code.
	 */

	int ret;
	struct timespec rts, mts;
	struct timeval tv;

	printk("POSIX clock set APIs\n");

	/* Minimal sleep to align us to the next tick interval. This
	 * helps with a case that 2 consecutive calls to clock_gettime()
	 * below return different values. Note that the tick alignment
	 * approach may break, in which case follow the suggestion in the
	 * comment above.
	 */
	k_usleep(1);

	ret = clock_gettime(CLOCK_MONOTONIC, &mts);
	zassert_equal(ret, 0, "Fail to get monotonic clock");

	ret = clock_gettime(CLOCK_REALTIME, &rts);
	zassert_equal(ret, 0, "Fail to get realtime clock");

	zassert_equal(rts.tv_sec, mts.tv_sec, "Seconds not equal");
	zassert_equal(rts.tv_nsec, mts.tv_nsec, "Nanoseconds not equal");

	/* Set a particular time.  In this case, the output of:
	 * `date +%s -d 2018-01-01T15:45:01Z`
	 */
	struct timespec nts;
	nts.tv_sec = 1514821501;
	nts.tv_nsec = NSEC_PER_SEC / 2U;

	/* TESTPOINT: Pass invalid clock type */
	zassert_equal(clock_settime(CLOCK_INVALID, &nts), -1,
			NULL);
	zassert_equal(errno, EINVAL, NULL);

	ret = clock_settime(CLOCK_MONOTONIC, &nts);
	zassert_not_equal(ret, 0, "Should not be able to set monotonic time");

	ret = clock_settime(CLOCK_REALTIME, &nts);
	zassert_equal(ret, 0, "Fail to set realtime clock");

	/*
	 * Loop 20 times, sleeping a little bit for each, making sure
	 * that the arithmetic roughly makes sense.  This tries to
	 * catch all of the boundary conditions of the clock to make
	 * sure there are no errors in the arithmetic.
	 */
	int64_t last_delta = 0;
	for (int i = 1; i <= 20; i++) {
		usleep(USEC_PER_MSEC * 90U);
		ret = clock_gettime(CLOCK_REALTIME, &rts);
		zassert_equal(ret, 0, "Fail to read realitime clock");

		int64_t delta =
			((int64_t)rts.tv_sec * NSEC_PER_SEC -
			 (int64_t)nts.tv_sec * NSEC_PER_SEC) +
			((int64_t)rts.tv_nsec - (int64_t)nts.tv_nsec);

		/* Make the delta milliseconds. */
		delta /= (NSEC_PER_SEC / 1000U);

		zassert_true(delta > last_delta, "Clock moved backward");
		int64_t error = delta - last_delta;

		/* printk("Delta %d: %lld\n", i, delta); */

		/* Allow for a little drift upward, but not
		 * downward
		 */
		zassert_true(error >= 90, "Clock inaccurate %d", error);
		zassert_true(error <= 110, "Clock inaccurate %d", error);

		last_delta = delta;
	}

	/* Validate gettimeofday API */
	ret = gettimeofday(&tv, NULL);
	zassert_equal(ret, 0, NULL);

	ret = clock_gettime(CLOCK_REALTIME, &rts);
	zassert_equal(ret, 0, NULL);

	/* TESTPOINT: Check if time obtained from
	 * gettimeofday is same or more than obtained
	 * from clock_gettime
	 */
	zassert_true(rts.tv_sec >= tv.tv_sec, "gettimeofday didn't"
			" provide correct result");
	zassert_true(rts.tv_nsec >= tv.tv_usec * NSEC_PER_USEC,
			"gettimeofday didn't provide correct result");

	printk("POSIX clock set APIs test done\n");
}
