/*
 * Copyright 2021 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

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
