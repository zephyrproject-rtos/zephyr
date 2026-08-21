/*
 * Copyright (c) 2026 Muhammad Waleed Badar
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/drivers/pm_cpu_ops.h>

/* Zephyr kernel start address. */
extern void __start(void);

ZTEST(arm64_spin_table, test_spin_table)
{
	int ret;

	for (int cpu = 0; cpu < CONFIG_MP_MAX_NUM_CPUS; cpu++) {
		ret = pm_cpu_on(cpu, (uintptr_t)&__start);

		zassert_equal(ret, 0, "pm_cpu_on() failed for CPU %d", cpu);
	}
}

ZTEST_SUITE(arm64_spin_table, NULL, NULL, NULL, NULL, NULL);
