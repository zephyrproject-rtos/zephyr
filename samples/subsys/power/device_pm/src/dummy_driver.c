/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/printk.h>
#include "dummy_parent.h"
#include "dummy_driver.h"

uint32_t device_power_state;
static const struct device *parent;

static int dummy_open(const struct device *dev)
{
	int ret;
	struct k_mutex wait_mutex;

	printk("open()\n");

	/* Make sure parent is resumed */
	ret = device_pm_get_sync(parent);
	if (ret < 0) {
		return ret;
	}

	ret = device_pm_get(dev);
	if (ret < 0) {
		return ret;
	}

	printk("Async wakeup request queued\n");

	k_mutex_init(&wait_mutex);
	k_mutex_lock(&wait_mutex, K_FOREVER);
	(void) k_condvar_wait(&dev->pm->condvar, &wait_mutex, K_FOREVER);
	k_mutex_unlock(&wait_mutex);

	if (atomic_get(&dev->pm->fsm_state) == DEVICE_PM_ACTIVE_STATE) {
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

	printk("read()\n");

	api = (struct dummy_parent_api *)parent->api;
	ret = api->transfer(parent, DUMMY_PARENT_RD, val);
	return ret;
}

static int dummy_write(const struct device *dev, uint32_t val)
{
	struct dummy_parent_api *api;
	int ret;

	printk("write()\n");
	api = (struct dummy_parent_api *)parent->api;
	ret = api->transfer(parent, DUMMY_PARENT_WR, &val);
	return ret;
}

static int dummy_close(const struct device *dev)
{
	int ret;

	printk("close()\n");
	ret = device_pm_put_sync(dev);
	if (ret == 1) {
		printk("Async suspend request ququed\n");
	}

	/* Parent can be suspended */
	if (parent) {
		device_pm_put(parent);
	}

	return ret;
}

static uint32_t dummy_get_power_state(const struct device *dev)
{
	return device_power_state;
}

static int dummy_suspend(const struct device *dev)
{
	printk("child suspending..\n");
	device_power_state = DEVICE_PM_SUSPEND_STATE;

	return 0;
}

static int dummy_resume_from_suspend(const struct device *dev)
{
	printk("child resuming..\n");
	device_power_state = DEVICE_PM_ACTIVE_STATE;

	return 0;
}

static int dummy_device_pm_ctrl(const struct device *dev,
				uint32_t ctrl_command,
				uint32_t *state, device_pm_cb cb, void *arg)
{
	int ret = 0;

	switch (ctrl_command) {
	case DEVICE_PM_SET_POWER_STATE:
		if (*state == DEVICE_PM_ACTIVE_STATE) {
			ret = dummy_resume_from_suspend(dev);
		} else {
			ret = dummy_suspend(dev);
		}
		break;
	case DEVICE_PM_GET_POWER_STATE:
		*state = dummy_get_power_state(dev);
		break;
	default:
		ret = -EINVAL;

	}

	cb(dev, ret, state, arg);

	return ret;
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

	device_pm_enable(dev);
	device_power_state = DEVICE_PM_ACTIVE_STATE;

	return 0;
}

DEVICE_DEFINE(dummy_driver, DUMMY_DRIVER_NAME, &dummy_init,
		    dummy_device_pm_ctrl, NULL, NULL, APPLICATION,
		    CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &funcs);
