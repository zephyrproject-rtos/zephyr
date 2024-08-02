/*
 * Copyright (c) 2021 Golioth, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <time.h>

ZTEST(libc_time, test_time_passing)
{
	time_t time_initial_unaligned;
	time_t time_initial;
	time_t time_current;
	int i;

	time_initial_unaligned = time(NULL);
	zassert_true(time_initial_unaligned >= 0, "Fail to get time");

	/* Wait until time() will return new value, which should be aligned */
	for (i = 0; i < 100; i++) {
		k_sleep(K_MSEC(10));

		if (time(NULL) != time_initial_unaligned) {
			break;
		}
	}

	time_initial = time(NULL);
	zassert_equal(time_initial, time_initial_unaligned + 1,
		      "Time (%d) should be one second larger than initially (%d)",
		      time_initial, time_initial_unaligned);

	for (i = 1; i <= 10; i++) {
		k_sleep(K_SECONDS(1));

		time_current = time(NULL);
		zassert_equal(time_current, time_initial + i,
			      "Current time (%d) does not match expected time (%d)",
			      (int) time_current, (int) (time_initial + i));
	}
}

ZTEST(libc_time, test_time_param)
{
	time_t time_result;
	time_t time_param;
	int i;

	time_result = time(&time_param);

	zassert_equal(time_result, time_param,
		      "time() result does not match param value");

	for (i = 0; i < 10; i++) {
		k_sleep(K_SECONDS(1));

		zassert_equal(time_result, time_param,
		      "time() result does not match param value");
	}
}

ZTEST_SUITE(libc_time, NULL, NULL, NULL, NULL, NULL);
