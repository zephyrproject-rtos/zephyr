/*
 * Copyright (c) 2019 Intel Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <zephyr/zephyr.h>

/*
 * precision timing tests in an emulation environment are not reliable.
 * if the test passes at least once, we know it works properly, so we
 * attempt to repeat the test RETRIES times before reporting failure.
 */

#define RETRIES		10

/*
 * We need to know how many ticks will elapse when we ask for the
 * shortest possible tick timeout.  That's generally 1, but in some
 * cases it may be more.  On Nordic paths that take 5 or 6 ticks may
 * be observed depending on clock stability and alignment. The base
 * rate assumes 3 ticks for non-timeout effects so increase the
 * maximum effect of timeout to 3 ticks on this platform.
 */

#if defined(CONFIG_NRF_RTC_TIMER) && (CONFIG_SYS_CLOCK_TICKS_PER_SEC > 16384)
/* The overhead of k_usleep() adds three ticks per loop iteration on
 * nRF51, which has a slow CPU clock.
 */
#define MAXIMUM_SHORTEST_TICKS (IS_ENABLED(CONFIG_SOC_SERIES_NRF51X) ? 6 : 3)
/*
 * Similar situation for TI CC13X2/CC26X2 RTC due to the limitation
 * that a value too close to the current time cannot be loaded to
 * its comparator.
 */
#elif defined(CONFIG_CC13X2_CC26X2_RTC_TIMER) && \
	(CONFIG_SYS_CLOCK_TICKS_PER_SEC > 16384)
#define MAXIMUM_SHORTEST_TICKS 3
#else
#define MAXIMUM_SHORTEST_TICKS 1
#endif

/*
 * Theory of operation: we can't use absolute units (e.g., "sleep for
 * 10us") in testing k_usleep() because the granularity of sleeps is
 * highly dependent on the hardware's capabilities and kernel
 * configuration. Instead, we test that k_usleep() actually sleeps for
 * the minimum possible duration, which is nominally two ticks.  So,
 * we loop k_usleep()ing for as many iterations as should comprise a
 * second, and check to see that a total of one second has elapsed.
 */

#define LOOPS (CONFIG_SYS_CLOCK_TICKS_PER_SEC / 2)

/* It should never iterate faster than the tick rate.  However the
 * app, sleep, and timeout layers may each add a tick alignment with
 * fast tick rates, and cycle layer may inject another to guarantee
 * the timeout deadline is met.
 */
#define LOWER_BOUND_MS	((1000 * LOOPS) / CONFIG_SYS_CLOCK_TICKS_PER_SEC)
#define UPPER_BOUND_MS	(((3 + MAXIMUM_SHORTEST_TICKS) * 1000 * LOOPS)	\
			 / CONFIG_SYS_CLOCK_TICKS_PER_SEC)

void test_usleep(void)
{
	int retries = 0;
	int64_t elapsed_ms;

	while (retries < RETRIES) {
		int64_t start_ms;
		int64_t end_ms;
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

	printk("elapsed_ms = %" PRId64 "\n", elapsed_ms);
	zassert_true(elapsed_ms >= LOWER_BOUND_MS, "short sleep");
	zassert_true(elapsed_ms <= UPPER_BOUND_MS, "overslept");
}
