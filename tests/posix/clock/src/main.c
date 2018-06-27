/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <pthread.h>

#define SLEEP_SECONDS 1

void test_posix_clock(void)
{
	s64_t nsecs_elapsed, secs_elapsed;
	struct timespec ts, te;

	printk("POSIX clock APIs\n");
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
	printk("POSIX clock set APIs\n");
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
	nts.tv_nsec = NSEC_PER_SEC / 2;
	ret = clock_settime(CLOCK_MONOTONIC, &nts);
	zassert_not_equal(ret, 0, "Should not be able to set monotonic time");

	ret = clock_settime(CLOCK_REALTIME, &nts);
	zassert_equal(ret, 0, "Fail to set realtime clock");

	/*
	 * Loop for 20 10ths of a second, sleeping a little bit for
	 * each, making sure that the arithmetic roughly makes sense.
	 * This tries to catch all of the boundary conditions of the
	 * clock to make sure there are no errors in the arithmetic.
	 */
	s64_t last_delta = 0;
	for (int i = 1; i <= 20; i++) {
		usleep(90 * USEC_PER_MSEC);
		ret = clock_gettime(CLOCK_REALTIME, &rts);
		zassert_equal(ret, 0, "Fail to read realitime clock");

		s64_t delta =
			((s64_t)rts.tv_sec * NSEC_PER_SEC -
			 (s64_t)nts.tv_sec * NSEC_PER_SEC) +
			((s64_t)rts.tv_nsec - (s64_t)nts.tv_nsec);

		/* Make the delta 10ths of a second. */
		delta /= (NSEC_PER_SEC / 1000);

		zassert_true(delta > last_delta, "Clock moved backward");
		s64_t error = delta - last_delta;

		/* printk("Delta %d: %lld\n", i, delta); */

		/* Allow for a little drift */
		zassert_true(error > 90, "Clock inaccurate");
		zassert_true(error < 110, "Clock inaccurate");

		last_delta = delta;
	}

	printk("POSIX clock set APIs test done\n");
}

void test_main(void)
{
	ztest_test_suite(test_posix_clock_api,
			ztest_unit_test(test_posix_clock),
			ztest_unit_test(test_posix_realtime));
	ztest_run_test_suite(test_posix_clock_api);
}
