/*
 * Copyright (c) 2019 Intel Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <zephyr.h>

/*
 * precision timing tests in an emulation environment are not reliable.
 * if the test passes at least once, we know it works properly, so we
 * attempt to repeat the test RETRIES times before reporting failure.
 */

#define RETRIES		1000

/*
 * test parameters. SLEEP_US * LOOPS should be at least a few milliseconds
 * so that the precision of the system uptime clock (1ms) isn't a factor.
 */

#define SLEEP_US	50
#define LOOPS		200

#define LOWER_BOUND_MS	((SLEEP_US * LOOPS) / 1000)
#define UPPER_BOUND_MS	(LOWER_BOUND_MS * 2)

void test_usleep(void)
{
	int retries = 0;
	s64_t elapsed_ms;

	while (retries < RETRIES) {
		s64_t start_ms;
		s64_t end_ms;
		int i;

		++retries;
		start_ms = k_uptime_get();

		for (i = 0; i < LOOPS; ++i)
			k_usleep(SLEEP_US);

		end_ms = k_uptime_get();
		elapsed_ms = end_ms - start_ms;

		/* if at first you don't succeed, keep sucking. */

		if ((elapsed_ms >= LOWER_BOUND_MS) &&
		    (elapsed_ms < UPPER_BOUND_MS)) {
			break;
		}
	}

	zassert_true(elapsed_ms >= LOWER_BOUND_MS, "short sleep");
	zassert_true(elapsed_ms < UPPER_BOUND_MS, "overslept");
}

void test_main(void)
{
	ztest_test_suite(usleep,
			 ztest_user_unit_test(test_usleep));

	ztest_run_test_suite(usleep);
}
