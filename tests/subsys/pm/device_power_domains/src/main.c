/*
 * Copyright (c) 2021 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <pm/device.h>
#include <pm/device_runtime.h>

static void test_demo(void)
{
	const struct device *reg_0 = DEVICE_DT_GET(DT_NODELABEL(test_reg_0));
	const struct device *reg_1 = DEVICE_DT_GET(DT_NODELABEL(test_reg_1));
	const struct device *reg_chained = DEVICE_DT_GET(DT_NODELABEL(test_reg_chained));

	printk("Enabling runtime power management on regulators\n");

	pm_device_runtime_enable(reg_0);
	pm_device_runtime_enable(reg_1);
	pm_device_runtime_enable(reg_chained);

	printk("Cycling: %s\n", reg_0->name);

	pm_device_runtime_get(reg_0);
	pm_device_runtime_put(reg_0);

	printk("Cycling: %s\n", reg_1->name);

	pm_device_runtime_get(reg_1);
	pm_device_runtime_put(reg_1);

	printk("Cycling: %s\n", reg_chained->name);

	pm_device_runtime_get(reg_chained);
	pm_device_runtime_put(reg_chained);

	printk("DONE\n");
}

void test_main(void)
{
	ztest_test_suite(device_power_domain,
			 ztest_unit_test(test_demo));
	ztest_run_test_suite(device_power_domain);
}
