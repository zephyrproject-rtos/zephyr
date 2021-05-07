/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <pm/device_runtime.h>
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
	ret = pm_device_get_sync(parent);
	if (ret < 0) {
		return ret;
	}

	ret = pm_device_get(dev);
	if (ret < 0) {
		return ret;
	}

	printk("Async wakeup request queued\n");

	k_mutex_init(&wait_mutex);
	k_mutex_lock(&wait_mutex, K_FOREVER);
	(void) k_condvar_wait(&dev->pm->condvar, &wait_mutex, K_FOREVER);
	k_mutex_unlock(&wait_mutex);

	if (atomic_get(&dev->pm->state) == PM_DEVICE_STATE_ACTIVE) {
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
	ret = pm_device_put_sync(dev);
	if (ret == 1) {
		printk("Async suspend request ququed\n");
	}

	/* Parent can be suspended */
	if (parent) {
		pm_device_put(parent);
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
	device_power_state = PM_DEVICE_STATE_SUSPEND;

	return 0;
}

static int dummy_resume_from_suspend(const struct device *dev)
{
	printk("child resuming..\n");
	device_power_state = PM_DEVICE_STATE_ACTIVE;

	return 0;
}

static int dummy_device_pm_ctrl(const struct device *dev,
				uint32_t ctrl_command,
				uint32_t *state, pm_device_cb cb, void *arg)
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

	pm_device_enable(dev);
	device_power_state = PM_DEVICE_STATE_ACTIVE;

	return 0;
}

DEVICE_DEFINE(dummy_driver, DUMMY_DRIVER_NAME, &dummy_init,
		    dummy_device_pm_ctrl, NULL, NULL, APPLICATION,
		    CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &funcs);
