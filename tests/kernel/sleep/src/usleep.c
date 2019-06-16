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

#define RETRIES		10

/*
 * Theory of operation: we can't use absolute units (e.g., "sleep for
 * 10us") in testing k_usleep() because the granularity of sleeps is
 * highly dependent on the hardware's capabilities and kernel
 * configuration. Instead, we test that k_usleep() actually sleeps for
 * the minimum possible duration.  So, we loop k_usleep()ing for as
 * many iterations as should comprise a second, and check to see that
 * a total of one second has elapsed.
 */

#define LOOPS (CONFIG_SYS_CLOCK_TICKS_PER_SEC / 2)

/* It should never iterate faster than the tick rate.  It might be as
 * much as 4x slower on drivers with fast tick rates (each of the app,
 * sleep, timeout and cycle layers may need to align).
 */
#define LOWER_BOUND_MS	((1000 * LOOPS) / CONFIG_SYS_CLOCK_TICKS_PER_SEC)
#define UPPER_BOUND_MS	((4 * 1000 * LOOPS) / CONFIG_SYS_CLOCK_TICKS_PER_SEC)

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

		for (i = 0; i < LOOPS; ++i) {
			k_usleep(1);
		}

		end_ms = k_uptime_get();
		elapsed_ms = end_ms - start_ms;

		/* if at first you don't succeed, keep sucking. */

		if ((elapsed_ms >= LOWER_BOUND_MS) &&
		    (elapsed_ms <= UPPER_BOUND_MS)) {
			break;
		}
	}

	printk("elapsed_ms = %lld\n", elapsed_ms);
	zassert_true(elapsed_ms >= LOWER_BOUND_MS, "short sleep");
	zassert_true(elapsed_ms <= UPPER_BOUND_MS, "overslept");
}
