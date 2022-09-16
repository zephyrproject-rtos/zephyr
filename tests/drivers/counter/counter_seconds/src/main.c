/*
 * Copyright (c) 2019 Intel Corp.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/counter.h>
#include <ztest.h>
#include <zephyr/kernel.h>

/*
 * Basic test to ensure that the clock is ticking at roughly 1Hz.
 */

#ifdef CONFIG_COUNTER_CMOS
#define DEVICE_LABEL "CMOS"
#else
#define DEVICE_LABEL DT_LABEL(DT_ALIAS(rtc_0))
#endif

#define DELAY_MS 1200	/* pause 1.2 seconds should always pass */
#define MIN_BOUND 1	/* counter must report at least MIN_BOUND .. */
#define MAX_BOUND 2	/* .. but at most MAX_BOUND seconds elapsed */

void test_seconds_rate(void)
{
	const struct device *dev;
	uint32_t start, elapsed;
	int err;

	dev = device_get_binding(DEVICE_LABEL);
	zassert_true(dev != NULL, "can't find counter device");

	err = counter_get_value(dev, &start);
	zassert_true(err == 0, "failed to read counter device");

	k_msleep(DELAY_MS);

	err = counter_get_value(dev, &elapsed);
	zassert_true(err == 0, "failed to read counter device");
	elapsed -= start;

	zassert_true(elapsed >= MIN_BOUND, "busted minimum bound");
	zassert_true(elapsed <= MAX_BOUND, "busted maximum bound");
}

void test_main(void)
{
	ztest_test_suite(test_seconds_counter, ztest_unit_test(test_seconds_rate));
	ztest_run_test_suite(test_seconds_counter);
}
