/*
 * Copyright (c) 2022, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>

static void test_demo(void)
{
	const struct device *reg_0 = DEVICE_DT_GET(DT_NODELABEL(test_reg_0));
	const struct device *reg_1 = DEVICE_DT_GET(DT_NODELABEL(test_reg_1));
	const struct device *reg_chained = DEVICE_DT_GET(DT_NODELABEL(test_reg_chained));

	TC_PRINT("Enabling runtime power management on regulators\n");

	pm_device_runtime_enable(reg_0);
	pm_device_runtime_enable(reg_1);
	pm_device_runtime_enable(reg_chained);

	TC_PRINT("Cycling: %s\n", reg_0->name);

	pm_device_runtime_get(reg_0);
	pm_device_runtime_put(reg_0);

	TC_PRINT("Cycling: %s\n", reg_1->name);

	pm_device_runtime_get(reg_1);
	pm_device_runtime_put(reg_1);

	TC_PRINT("Cycling: %s\n", reg_chained->name);

	pm_device_runtime_get(reg_chained);
	pm_device_runtime_put(reg_chained);

	TC_PRINT("DONE\n");
}

void test_main(void)
{
	ztest_test_suite(device_power_domain,
			 ztest_unit_test(test_demo));
	ztest_run_test_suite(device_power_domain);
}
