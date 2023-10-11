/*
 * Copyright (c) 2022, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>

static int dev_init(const struct device *dev)
{
	if (!pm_device_is_powered(dev)) {
		pm_device_init_off(dev);
	}
	return 0;
}

int dev_pm_control(const struct device *dev, enum pm_device_action action)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(action);

	return 0;
}

PM_DEVICE_DT_DEFINE(DT_NODELABEL(test_dev), dev_pm_control);
DEVICE_DT_DEFINE(DT_NODELABEL(test_dev), dev_init, PM_DEVICE_DT_GET(DT_NODELABEL(test_dev)),
		 NULL, NULL, POST_KERNEL, 80, NULL);

ZTEST(device_power_domain, test_device_power_domain)
{
	const struct device *const reg_0 = DEVICE_DT_GET(DT_NODELABEL(test_reg_0));
	const struct device *const reg_1 = DEVICE_DT_GET(DT_NODELABEL(test_reg_1));
	const struct device *const reg_chained = DEVICE_DT_GET(DT_NODELABEL(test_reg_chained));
	const struct device *const dev = DEVICE_DT_GET(DT_NODELABEL(test_dev));
	enum pm_device_state state;

	/* Initial power state */
	zassert_true(pm_device_is_powered(reg_0), "");
	zassert_true(pm_device_is_powered(reg_1), "");
	zassert_true(pm_device_is_powered(reg_chained), "");
	zassert_true(pm_device_is_powered(dev), "");

	TC_PRINT("Enabling runtime power management on regulators\n");

	pm_device_runtime_enable(dev);
	pm_device_runtime_enable(reg_chained);
	pm_device_runtime_enable(reg_1);
	pm_device_runtime_enable(reg_0);

	/* Power domains should now be suspended */
	zassert_true(pm_device_is_powered(reg_0), "");
	zassert_true(pm_device_is_powered(reg_1), "");
	zassert_false(pm_device_is_powered(reg_chained), "");
	zassert_false(pm_device_is_powered(dev), "");
	pm_device_state_get(dev, &state);
	zassert_equal(PM_DEVICE_STATE_OFF, state, "");

	TC_PRINT("Cycling: %s\n", reg_0->name);

	/* reg_chained is powered off reg_0, so it's power state should change */
	pm_device_runtime_get(reg_0);
	zassert_true(pm_device_is_powered(reg_chained), "");
	zassert_false(pm_device_is_powered(dev), "");
	pm_device_runtime_put(reg_0);
	zassert_false(pm_device_is_powered(reg_chained), "");

	TC_PRINT("Cycling: %s\n", reg_1->name);

	pm_device_runtime_get(reg_1);
	zassert_false(pm_device_is_powered(reg_chained), "");
	zassert_true(pm_device_is_powered(dev), "");
	/* dev is on reg_1, should have automatically moved to suspended */
	pm_device_state_get(dev, &state);
	zassert_equal(PM_DEVICE_STATE_SUSPENDED, state, "");
	pm_device_runtime_put(reg_1);
	pm_device_state_get(dev, &state);
	zassert_equal(PM_DEVICE_STATE_OFF, state, "");

	TC_PRINT("Cycling: %s\n", reg_chained->name);

	/* reg_chained should be powered after being requested */
	pm_device_runtime_get(reg_chained);
	zassert_true(pm_device_is_powered(reg_chained), "");
	zassert_false(pm_device_is_powered(dev), "");
	/* dev is not on reg_chained but does reference it, should still be in OFF */
	pm_device_state_get(dev, &state);
	zassert_equal(PM_DEVICE_STATE_OFF, state, "");
	pm_device_runtime_put(reg_chained);

	TC_PRINT("Requesting dev: %s\n", dev->name);

	/* Directly request the supported device */
	pm_device_runtime_get(dev);
	zassert_true(pm_device_is_powered(dev), "");
	pm_device_state_get(dev, &state);
	zassert_equal(PM_DEVICE_STATE_ACTIVE, state, "");
	pm_device_state_get(reg_1, &state);
	zassert_equal(PM_DEVICE_STATE_ACTIVE, state, "");
	pm_device_state_get(reg_chained, &state);
	zassert_equal(PM_DEVICE_STATE_OFF, state, "");
	/* Directly release the supported device */
	pm_device_runtime_put(dev);
	zassert_false(pm_device_is_powered(dev), "");
	pm_device_state_get(dev, &state);
	zassert_equal(PM_DEVICE_STATE_OFF, state, "");
	pm_device_state_get(reg_1, &state);
	zassert_equal(PM_DEVICE_STATE_SUSPENDED, state, "");
	pm_device_state_get(reg_chained, &state);
	zassert_equal(PM_DEVICE_STATE_OFF, state, "");

	TC_PRINT("DONE\n");
}

ZTEST_SUITE(device_power_domain, NULL, NULL, NULL, NULL, NULL);
