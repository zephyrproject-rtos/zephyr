/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/sys/printk.h>
#include "dummy_parent.h"
#include "dummy_driver.h"

static const struct device *parent;

static int dummy_open(const struct device *dev)
{
	int ret;
	enum pm_device_state state;

	/* Make sure parent is resumed */
	ret = pm_device_runtime_get(parent);
	if (ret < 0) {
		return ret;
	}

	ret = pm_device_runtime_get(dev);
	if (ret < 0) {
		(void)pm_device_runtime_put(parent);
		return ret;
	}

	(void)pm_device_state_get(dev, &state);
	if (state == PM_DEVICE_STATE_ACTIVE) {
		printk("Dummy device resumed\n");
		ret = 0;
	} else {
		printk("Dummy device Not resumed\n");
		ret = -1;
	}

	return ret;
}

static int dummy_read(const struct device *dev, uint32_t *val)
{
	struct dummy_parent_api *api;
	int ret;

	api = (struct dummy_parent_api *)parent->api;
	ret = api->transfer(parent, DUMMY_PARENT_RD, val);
	return ret;
}

static int dummy_write(const struct device *dev, uint32_t val)
{
	struct dummy_parent_api *api;
	int ret;

	api = (struct dummy_parent_api *)parent->api;
	ret = api->transfer(parent, DUMMY_PARENT_WR, &val);
	return ret;
}

static int dummy_close(const struct device *dev)
{
	int ret;

	ret = pm_device_runtime_put(dev);
	if (ret == 1) {
		printk("Async suspend request queued\n");
	}

	/* Parent can be suspended */
	if (parent) {
		pm_device_runtime_put(parent);
	}

	return ret;
}

static int dummy_device_pm_action(const struct device *dev,
				  enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		printk("child resuming..\n");
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		printk("child suspending..\n");
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static const struct dummy_driver_api funcs = {
	.open = dummy_open,
	.read = dummy_read,
	.write = dummy_write,
	.close = dummy_close,
};

int dummy_init(const struct device *dev)
{
	parent = device_get_binding(DUMMY_PARENT_NAME);
	if (!parent) {
		printk("parent not found\n");
	}

	return pm_device_runtime_enable(dev);
}

PM_DEVICE_DEFINE(dummy_driver, dummy_device_pm_action);

DEVICE_DEFINE(dummy_driver, DUMMY_DRIVER_NAME, &dummy_init,
		    PM_DEVICE_GET(dummy_driver), NULL, NULL, APPLICATION,
		    CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &funcs);
