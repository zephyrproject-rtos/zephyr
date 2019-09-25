/*
 * Copyright (c) 2019 Intel Corp.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/counter.h>
#include <ztest.h>
#include <kernel.h>

/*
 * Basic test to ensure that the clock is ticking at roughly 1Hz.
 */

#define DELAY_MS 1200	/* pause 1.2 seconds should always pass */
#define MIN_BOUND 1	/* counter must report at least MIN_BOUND .. */
#define MAX_BOUND 2	/* .. but at most MAX_BOUND seconds elapsed */

void test_cmos_rate(void)
{
	struct device *cmos;
	u32_t start, elapsed;

	cmos = device_get_binding("CMOS");
	zassert_true(cmos != NULL, "can't find CMOS counter device");

	start = counter_read(cmos);
	k_sleep(DELAY_MS);
	elapsed = counter_read(cmos) - start;

	zassert_true(elapsed >= MIN_BOUND, "busted minimum bound");
	zassert_true(elapsed <= MAX_BOUND, "busted maximum bound");
}

void test_main(void)
{
	ztest_test_suite(test_cmos_counter, ztest_unit_test(test_cmos_rate));
	ztest_run_test_suite(test_cmos_counter);
}
