/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/printk.h>
#include <zephyr/types.h>
#include <pm/device_runtime.h>
#include "dummy_driver.h"

static int dummy_wait(const struct device *dev)
{
	return pm_device_wait(dev, K_FOREVER);
}

static int dummy_open(const struct device *dev)
{
	return pm_device_get_async(dev);
}

static int dummy_open_sync(const struct device *dev)
{
	return pm_device_get(dev);
}

static int dummy_close(const struct device *dev)
{
	return pm_device_put_async(dev);
}

static int dummy_close_sync(const struct device *dev)
{
	return pm_device_put(dev);
}

static int dummy_device_pm_ctrl(const struct device *dev,
				enum pm_device_action action)
{
	return 0;
}

static const struct dummy_driver_api funcs = {
	.open = dummy_open,
	.open_sync = dummy_open_sync,
	.close = dummy_close,
	.close_sync = dummy_close_sync,
	.wait = dummy_wait,
};

int dummy_init(const struct device *dev)
{
	pm_device_enable(dev);
	return 0;
}

DEVICE_DEFINE(dummy_driver, DUMMY_DRIVER_NAME, &dummy_init,
		    dummy_device_pm_ctrl, NULL, NULL, APPLICATION,
		    CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &funcs);
