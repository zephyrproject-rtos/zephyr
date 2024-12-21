/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

extern struct ztest_suite_stats UTIL_CAT(z_ztest_suite_node_stats_, testsuite);
struct ztest_suite_stats *suite_stats = &UTIL_CAT(z_ztest_suite_node_stats_, testsuite);
extern struct ztest_unit_test_stats z_ztest_unit_test_stats_testsuite_test_repeating;
struct ztest_unit_test_stats *case_stats = &z_ztest_unit_test_stats_testsuite_test_repeating;

ZTEST(testsuite, test_repeating)
{
	ztest_test_pass();
}

static void repeat_teardown(void *)
{
	/* run_count + 1 counter is incremented after the testcase is executed. */
	printk("Test suite executed: %d times.\n", suite_stats->run_count + 1);
	printk("Test case executed : %d times.\n", case_stats->run_count);
}

ZTEST_SUITE(testsuite, NULL, NULL, NULL, NULL, repeat_teardown);
