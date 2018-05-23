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

void test_main(void)
{
	ztest_test_suite(test_posix_clock_api,
			ztest_unit_test(test_posix_clock));
	ztest_run_test_suite(test_posix_clock_api);
}
