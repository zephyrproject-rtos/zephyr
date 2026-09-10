/*
 * Copyright 2025 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/devicetree.h>
#include <zephyr/device.h>

#define POWER_DOMAIN_NODE DT_NODELABEL(adc0_pd)

ZTEST(tisci_power_domain, test_power_domain_runtime)
{
	const struct device *pd_dev = DEVICE_DT_GET(POWER_DOMAIN_NODE);

	zassert_not_null(pd_dev, "Power domain device not found");
	zassert_true(device_is_ready(pd_dev), "Power domain device not ready");

	const struct device *dmsc = DEVICE_DT_GET(DT_NODELABEL(dmsc));

	zassert_not_null(dmsc, "DMSC device not found");
	zassert_true(device_is_ready(dmsc), "DMSC device not ready");

	int ret;

	/* Power on */
	ret = pm_device_runtime_get(pd_dev);

	zassert_ok(ret, "Failed to power ON");

	/* Power off */
	ret = pm_device_runtime_put(pd_dev);

	zassert_ok(ret, "Failed to power OFF");

	/* Power on again */
	ret = pm_device_runtime_get(pd_dev);

	zassert_ok(ret, "Failed to power ON again");

	/* Power off again */
	ret = pm_device_runtime_put(pd_dev);

	zassert_ok(ret, "Failed to power OFF again");
}

ZTEST_SUITE(tisci_power_domain, NULL, NULL, NULL, NULL, NULL);
