/*
 * Copyright (c) 2026 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <zephyr/ztest.h>

LOG_MODULE_REGISTER(smbus_it51xxx_test, LOG_LEVEL_INF);

static const struct device *const smbus_host_dev = DEVICE_DT_GET(DT_ALIAS(smbus_host));
static const struct device *const smbus_target_bus_dev = DEVICE_DT_GET(DT_ALIAS(i2c_target));

/* Pure bring-up checks */
ZTEST(smbus_device, test_devices_ready)
{
	zassert_true(device_is_ready(smbus_host_dev), "SMBus host device %s not ready",
		     smbus_host_dev->name);
	zassert_true(device_is_ready(smbus_target_bus_dev), "I2C target bus device %s not ready",
		     smbus_target_bus_dev->name);
}

ZTEST_SUITE(smbus_device, NULL, NULL, NULL, NULL, NULL);
