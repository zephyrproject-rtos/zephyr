/*
 * Copyright (c) 2026 Fabien Proriol
 * SPDX-License-Identifier: Apache-2.0
 *
 * Whole-stack build/boot smoke test: CONFIG_KNX_STACK=y with the loopback L1
 * driver. Its job is mostly the build — knx_core.c, layer1_physical.c and the
 * layer sources compile as configured — plus a check that knx_init() ran.
 */

#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/knx/knx_l1.h>
#include "layer1_physical.h"

ZTEST(knx_stack, test_l1_driver_resolved)
{
	const struct device *dev = knx_phy_get();

	zassert_not_null(dev, "knx_phy_get() found no L1 device");
	zassert_true(device_is_ready(dev), "L1 device not ready");
}

ZTEST_SUITE(knx_stack, NULL, NULL, NULL, NULL, NULL);
