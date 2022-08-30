/*
 * Copyright (c) 2022, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>

ZTEST(device_power_domain, test_demo)
{
	const struct device *const reg_0 = DEVICE_DT_GET(DT_NODELABEL(test_reg_0));
	const struct device *const reg_1 = DEVICE_DT_GET(DT_NODELABEL(test_reg_1));
	const struct device *const reg_chained = DEVICE_DT_GET(DT_NODELABEL(test_reg_chained));

	/* Initial power state */
	zassert_true(pm_device_is_powered(reg_0), "");
	zassert_true(pm_device_is_powered(reg_1), "");
	zassert_false(pm_device_is_powered(reg_chained), "");

	TC_PRINT("Enabling runtime power management on regulators\n");

	pm_device_runtime_enable(reg_0);
	pm_device_runtime_enable(reg_1);
	pm_device_runtime_enable(reg_chained);

	/* State shouldn't have changed */
	zassert_true(pm_device_is_powered(reg_0), "");
	zassert_true(pm_device_is_powered(reg_1), "");
	zassert_false(pm_device_is_powered(reg_chained), "");

	TC_PRINT("Cycling: %s\n", reg_0->name);

	/* reg_chained is powered off reg_0, so it's power state should change */
	pm_device_runtime_get(reg_0);
	zassert_true(pm_device_is_powered(reg_chained), "");
	pm_device_runtime_put(reg_0);
	zassert_false(pm_device_is_powered(reg_chained), "");

	TC_PRINT("Cycling: %s\n", reg_1->name);

	pm_device_runtime_get(reg_1);
	zassert_false(pm_device_is_powered(reg_chained), "");
	pm_device_runtime_put(reg_1);

	TC_PRINT("Cycling: %s\n", reg_chained->name);

	/* reg_chained should be powered after being requested */
	pm_device_runtime_get(reg_chained);
	zassert_true(pm_device_is_powered(reg_chained), "");
	pm_device_runtime_put(reg_chained);

	TC_PRINT("DONE\n");
}

ZTEST_SUITE(device_power_domain, NULL, NULL, NULL, NULL, NULL);
