/*
 * Copyright (c) 2019 Intel Corp.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/counter.h>
#include <zephyr/ztest.h>
#include <zephyr/kernel.h>

/*
 * Basic test to ensure that the clock is ticking at roughly 1Hz.
 */

#ifdef CONFIG_COUNTER_CMOS
#define CTR_DEV		DT_COMPAT_GET_ANY_STATUS_OKAY(motorola_mc146818)
#else
#define CTR_DEV		DT_ALIAS(rtc_0)
#endif

#define DELAY_MS 1200	/* pause 1.2 seconds should always pass */
#define MIN_BOUND 1	/* counter must report at least MIN_BOUND .. */
#define MAX_BOUND 2	/* .. but at most MAX_BOUND seconds elapsed */

ZTEST(seconds_counter, test_seconds_rate)
{
	const struct device *const dev = DEVICE_DT_GET(CTR_DEV);
	uint32_t start, elapsed;
	int err;

	zassert_true(device_is_ready(dev), "counter device is not ready");

	err = counter_get_value(dev, &start);
	zassert_true(err == 0, "failed to read counter device");

	k_msleep(DELAY_MS);

	err = counter_get_value(dev, &elapsed);
	zassert_true(err == 0, "failed to read counter device");
	elapsed -= start;

	zassert_true(elapsed >= MIN_BOUND, "busted minimum bound");
	zassert_true(elapsed <= MAX_BOUND, "busted maximum bound");
}

ZTEST_SUITE(seconds_counter, NULL, NULL, NULL, NULL, NULL);
