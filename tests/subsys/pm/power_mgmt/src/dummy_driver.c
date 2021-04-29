/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/printk.h>
#include <zephyr/types.h>
#include "dummy_driver.h"

static uint32_t device_power_state;

static int dummy_open(const struct device *dev)
{
	return device_pm_get_sync(dev);
}

static int dummy_close(const struct device *dev)
{
	return device_pm_put_sync(dev);
}

static uint32_t dummy_get_power_state(const struct device *dev)
{
	return device_power_state;
}

static int dummy_suspend(const struct device *dev)
{
	device_power_state = DEVICE_PM_SUSPEND_STATE;
	return 0;
}

static int dummy_resume_from_suspend(const struct device *dev)
{
	device_power_state = DEVICE_PM_ACTIVE_STATE;
	return 0;
}

static int dummy_device_pm_ctrl(const struct device *dev,
				uint32_t ctrl_command,
				void *context, device_pm_cb cb, void *arg)
{
	int ret = 0;

	switch (ctrl_command) {
	case DEVICE_PM_SET_POWER_STATE:
		if (*((uint32_t *)context) == DEVICE_PM_ACTIVE_STATE) {
			ret = dummy_resume_from_suspend(dev);
		} else {
			ret = dummy_suspend(dev);
		}
		break;
	case DEVICE_PM_GET_POWER_STATE:
		*((uint32_t *)context) = dummy_get_power_state(dev);
		break;
	default:
		ret = -EINVAL;

	}

	if (cb) {
		cb(dev, ret, context, arg);
	}

	return ret;
}

static const struct dummy_driver_api funcs = {
	.open = dummy_open,
	.close = dummy_close,
};

int dummy_init(const struct device *dev)
{
	device_pm_enable(dev);
	return 0;
}

DEVICE_DEFINE(dummy_driver, DUMMY_DRIVER_NAME, &dummy_init,
		    dummy_device_pm_ctrl, NULL, NULL, APPLICATION,
		    CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &funcs);
