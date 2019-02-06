/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <misc/printk.h>
#include "dummy_parent.h"
#include "dummy_driver.h"

static struct k_poll_event async_evt;
u32_t device_power_state;
static struct device *parent;

static int dummy_open(struct device *dev)
{
	int ret, result;
	u32_t signaled;

	printk("open()\n");
	ret = device_pm_get(dev);
	if (ret < 0) {
		return ret;
	}

	printk("Async wakeup request queued\n");
	ret = k_poll(&async_evt, 1, K_FOREVER);
	if (ret < 0) {
		return ret;
	}

	if (async_evt.state == K_POLL_STATE_SIGNALED) {
		k_poll_signal_check(&dev->pm->signal, &signaled, &result);
		if (result == DEVICE_PM_STATUS_ACTIVE) {
			printk("Dummy device resumed\n");
			ret = 0;
		} else {
			printk("Dummy device Not resumed\n");
			ret = -1;
		}
	} else {
		printk("k_poll event not signalled\n");
		ret = -1;
	}

	async_evt.state = K_POLL_STATE_NOT_READY;
	k_poll_signal_reset(&dev->pm->signal);

	return ret;
}

static int dummy_read(struct device *dev, u32_t *val)
{
	struct dummy_parent_api *api;
	int ret;

	printk("read()\n");

	api = (struct dummy_parent_api *)parent->driver_api;
	ret = api->transfer(parent, DUMMY_PARENT_RD, val);
	return ret;
}

static int dummy_write(struct device *dev, u32_t val)
{
	struct dummy_parent_api *api;
	int ret;

	printk("write()\n");
	api = (struct dummy_parent_api *)parent->driver_api;
	ret = api->transfer(parent, DUMMY_PARENT_WR, &val);
	return ret;
}

static int dummy_close(struct device *dev)
{
	int ret;

	printk("close()\n");
	ret = device_pm_put(dev);
	if (ret == 1) {
		printk("Async suspend request ququed\n");
	}
	return ret;
}

static u32_t dummy_get_power_state(struct device *dev)
{
	return device_power_state;
}

static int dummy_suspend(struct device *dev)
{
	printk("child suspending..\n");
	device_power_state = DEVICE_PM_SUSPEND_STATE;

	return 0;
}

static int dummy_resume_from_suspend(struct device *dev)
{
	printk("child resuming..\n");
	device_power_state = DEVICE_PM_ACTIVE_STATE;

	return 0;
}

static int dummy_device_pm_ctrl(struct device *dev, u32_t ctrl_command,
				void *context)
{
	int ret = 0;

	switch (ctrl_command) {
	case DEVICE_PM_SET_POWER_STATE:
		if (*((u32_t *)context) == DEVICE_PM_ACTIVE_STATE) {
			/* Make sure parent is resumed */
			if (parent) {
				device_pm_get_sync(parent);
			}

			ret = dummy_resume_from_suspend(dev);

		} else {
			ret = dummy_suspend(dev);

			/* Parent can be suspended */
			if (parent) {
				device_pm_put(parent);
			}
		}
		break;
	case DEVICE_PM_GET_POWER_STATE:
		*((u32_t *)context) = dummy_get_power_state(dev);
		break;
	default:
		ret = -EINVAL;

	}

	return ret;
}

static const struct dummy_driver_api funcs = {
	.open = dummy_open,
	.read = dummy_read,
	.write = dummy_write,
	.close = dummy_close,
};

int dummy_init(struct device *dev)
{
	parent = device_get_binding(DUMMY_PARENT_NAME);
	if (!parent) {
		printk("parent not found\n");
	}

	device_power_state = DEVICE_PM_ACTIVE_STATE;

	k_poll_event_init(&async_evt, K_POLL_TYPE_SIGNAL,
			K_POLL_MODE_NOTIFY_ONLY, &dev->pm->signal);
	return 0;
}

DEVICE_DEFINE(dummy_driver, DUMMY_DRIVER_NAME, &dummy_init,
		    dummy_device_pm_ctrl, NULL, NULL, APPLICATION,
		    CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &funcs);
