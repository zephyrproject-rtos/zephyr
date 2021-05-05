/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <pm/device_runtime.h>
#include <sys/printk.h>
#include "dummy_parent.h"

static uint32_t store_value;
uint32_t parent_power_state;

static int dummy_transfer(const struct device *dev, uint32_t cmd,
			  uint32_t *val)
{
	printk("transfer()\n");

	if (cmd == DUMMY_PARENT_WR) {
		store_value = *val;
	} else {
		*val = store_value;
	}

	return 0;
}

static uint32_t dummy_get_power_state(const struct device *dev)
{
	return parent_power_state;
}

static int dummy_suspend(const struct device *dev)
{
	printk("parent suspending..\n");
	parent_power_state = PM_DEVICE_SUSPEND_STATE;

	return 0;
}

static int dummy_resume_from_suspend(const struct device *dev)
{
	printk("parent resuming..\n");
	parent_power_state = PM_DEVICE_ACTIVE_STATE;

	return 0;
}

static int dummy_parent_pm_ctrl(const struct device *dev,
				uint32_t ctrl_command,
				void *context, pm_device_cb cb, void *arg)
{
	int ret = 0;

	switch (ctrl_command) {
	case PM_DEVICE_STATE_SET:
		if (*((uint32_t *)context) == PM_DEVICE_ACTIVE_STATE) {
			ret = dummy_resume_from_suspend(dev);
		} else {
			ret = dummy_suspend(dev);
		}
		break;
	case PM_DEVICE_STATE_GET:
		*((uint32_t *)context) = dummy_get_power_state(dev);
		break;
	default:
		ret = -EINVAL;

	}

	cb(dev, ret, context, arg);
	return ret;
}

static const struct dummy_parent_api funcs = {
	.transfer = dummy_transfer,
};

int dummy_parent_init(const struct device *dev)
{
	pm_device_enable(dev);
	parent_power_state = PM_DEVICE_ACTIVE_STATE;
	return 0;
}

DEVICE_DEFINE(dummy_parent, DUMMY_PARENT_NAME, &dummy_parent_init,
		    dummy_parent_pm_ctrl, NULL, NULL, POST_KERNEL,
		    CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &funcs);
