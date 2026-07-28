/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/devicetree.h>

static const uint32_t cpu_regs[] = {
	DT_FOREACH_CPU_STATUS_OKAY_SEP(DT_REG_ADDR, (,))
};

ZTEST_SUITE(devicetree_cpu, NULL, NULL, NULL, NULL, NULL);

ZTEST(devicetree_cpu, test_foreach_cpu_status_okay_sep_skips_non_cpu)
{
	/*
	 * "/cpus" has two types of children:
	 * (1) At least 1 "cpu@#" node with (device_type = "cpu")
	 * (2) "power-states" (no device_type)
	 *
	 * Only cpu nodes should be counted
	 */
	printf("Number of cpu entries found: %d\n", ARRAY_SIZE(cpu_regs));

	zassert_true(ARRAY_SIZE(cpu_regs) >= 1, "Expected at least 1 cpu entry");
	zassert_equal(cpu_regs[0], DT_REG_ADDR(DT_PATH(cpus, cpu_0)), "");
}
