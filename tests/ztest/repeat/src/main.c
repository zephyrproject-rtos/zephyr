/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/ztest.h>

extern struct ztest_suite_stats UTIL_CAT(z_ztest_suite_node_stats_, testsuite);
struct ztest_suite_stats *suite_stats = &UTIL_CAT(z_ztest_suite_node_stats_, testsuite);
extern struct ztest_unit_test_stats z_ztest_unit_test_stats_testsuite_test_update_suite_stats;
struct ztest_unit_test_stats *case_stats =
	&z_ztest_unit_test_stats_testsuite_test_update_suite_stats;

static struct test_data {
	int suite_run;
	int case_run;
} expected = {0, 0};

ZTEST(testsuite, test_update_suite_stats)
{
	expected.case_run++;

	if (case_stats->run_count > 0) {
		zassert_true(case_stats->run_count == expected.case_run);
	}

	if (suite_stats->run_count > 0) {
		zassert_true(suite_stats->run_count + 1 == expected.suite_run);
	}
}

static void *repeat_setup(void)
{
	expected.suite_run++;
	return NULL;
}

ZTEST_SUITE(testsuite, NULL, repeat_setup, NULL, NULL, NULL);
