/*
 * Copyright 2021 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

#ifdef CONFIG_ZTEST_COVERAGE_PER_TEST
#include <coverage.h>
#include <zephyr/debug/gcov.h>

static void coverage_per_test_before_each(const struct ztest_unit_test *test, void *data)
{
	ARG_UNUSED(test);
	ARG_UNUSED(data);
	gcov_reset_all_counts();
}
static void coverage_per_test_after_each(const struct ztest_unit_test *test, void *data)
{
	char tag[CONFIG_ZTEST_COVERAGE_PER_TEST_TAG_SIZE];

	ARG_UNUSED(data);

	snprintk(tag, sizeof(tag), "%s.%s", test->test_suite_name, test->name);
	gcov_coverage_dump_tagged(tag);
}
ZTEST_RULE(coverage_per_test, coverage_per_test_before_each, coverage_per_test_after_each);
#endif /* CONFIG_ZTEST_COVERAGE_PER_TEST */

#ifdef CONFIG_ZTEST_RULE_1CPU
static void one_cpu_rule_before_each(const struct ztest_unit_test *test, void *data)
{
	ARG_UNUSED(test);
	ARG_UNUSED(data);
	z_test_1cpu_start();
}
static void one_cpu_rule_after_each(const struct ztest_unit_test *test, void *data)
{
	ARG_UNUSED(test);
	ARG_UNUSED(data);
	z_test_1cpu_stop();
}
ZTEST_RULE(one_cpu, one_cpu_rule_before_each, one_cpu_rule_after_each);
#endif /* CONFIG_ZTEST_RULE_1CPU */
