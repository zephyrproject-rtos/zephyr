/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/printk.h>
#include <zephyr/types.h>
#include <pm/device_runtime.h>
#include "dummy_driver.h"

static uint32_t device_power_state;

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

static uint32_t dummy_get_power_state(const struct device *dev)
{
	return device_power_state;
}

static int dummy_suspend(const struct device *dev)
{
	device_power_state = PM_DEVICE_STATE_SUSPEND;
	return 0;
}

static int dummy_resume_from_suspend(const struct device *dev)
{
	device_power_state = PM_DEVICE_STATE_ACTIVE;
	return 0;
}

static int dummy_device_pm_ctrl(const struct device *dev,
				uint32_t ctrl_command,
				enum pm_device_state *state)
{
	int ret = 0;

	switch (ctrl_command) {
	case PM_DEVICE_STATE_SET:
		if (*state == PM_DEVICE_STATE_ACTIVE) {
			ret = dummy_resume_from_suspend(dev);
		} else {
			ret = dummy_suspend(dev);
		}
		break;
	case PM_DEVICE_STATE_GET:
		*state = dummy_get_power_state(dev);
		break;
	default:
		ret = -EINVAL;

	}

	return ret;
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
