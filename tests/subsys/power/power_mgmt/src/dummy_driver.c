/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/printk.h>
#include <zephyr/types.h>
#include "dummy_driver.h"
#include "power/power.h"

static uint32_t device_power_state;
static bool busy;
static int low_power_times;
static struct k_poll_event async_evt;

static int dummy_open(const struct device *dev)
{
	int ret;
	int signaled = 0, result;

	if (busy)
		return -EBUSY;

	ret = device_pm_get_sync(dev);
	if (ret < 0) {
		return ret;
	}

	ret = device_pm_get(dev);
	if (ret < 0) {
		return ret;
	}

	printk("Async wakeup request queued\n");

	do {
		(void)k_poll(&async_evt, 1, K_FOREVER);
		k_poll_signal_check(&dev->pm->signal,
						&signaled, &result);
	} while (!signaled);

	async_evt.state = K_POLL_STATE_NOT_READY;
	k_poll_signal_reset(&dev->pm->signal);

	if (result == DEVICE_PM_ACTIVE_STATE) {
		printk("Dummy device resumed\n");
		/* I'm working, don't go standby */
		pm_constraint_set(PM_STATE_STANDBY);
		ret = 0;
	} else {
		printk("Dummy device Not resumed\n");
		ret = -1;
	}

	return ret;
}

static int dummy_close(const struct device *dev)
{
	int ret;

	busy = false;
	ret = device_pm_put(dev);
	if (ret < 0) {
		return ret;
	}

	ret = device_pm_put_sync(dev);
	if (ret < 0) {
		return ret;
	}
	pm_constraint_release(PM_STATE_STANDBY);
	return ret;
}

static int dummy_busy(const struct device *dev)
{
	busy = true;
	return 0;
}

static int dummy_pm_disable(const struct device *dev)
{
	device_pm_disable(dev);
	return 0;
}

static int dummy_low_power_times(const struct device *dev)
{
	return low_power_times;
}

static uint32_t dummy_get_power_state(const struct device *dev)
{
	return device_power_state;
}

static int dummy_suspend(uint32_t state)
{
	int ret = 0;

	if (pm_constraint_get(PM_STATE_STANDBY)) {
		pm_constraint_release(PM_STATE_STANDBY);
	}
	switch (state) {
	case DEVICE_PM_LOW_POWER_STATE:
		if (busy) {
			ret = -EBUSY;
			break;
		}
		device_power_state = state;
		low_power_times++;
		break;
	case DEVICE_PM_SUSPEND_STATE:
		if (busy) {
			ret = -EBUSY;
			break;
		}
		device_power_state = state;
		low_power_times++;
		break;
	case DEVICE_PM_FORCE_SUSPEND_STATE:
		device_power_state = state;
		low_power_times++;
		busy = false;
		break;
	case DEVICE_PM_OFF_STATE:
		device_power_state = state;
		busy = false;
		break;
	default:
		ret = -EINVAL;
		break;
	}
	return ret;
}

static int dummy_resume_from_suspend(const struct device *dev)
{
		device_power_state = DEVICE_PM_ACTIVE_STATE;
		/* I'm working, don't go standby */
		pm_constraint_set(PM_STATE_STANDBY);
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
			ret = dummy_suspend(*((uint32_t *)context));
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
	.busy = dummy_busy,
	.low_power_times = dummy_low_power_times,
	.pm_disable = dummy_pm_disable,
};

int dummy_init(const struct device *dev)
{
	device_pm_enable(dev);
	device_power_state = DEVICE_PM_ACTIVE_STATE;

	k_poll_event_init(&async_evt, K_POLL_TYPE_SIGNAL,
			K_POLL_MODE_NOTIFY_ONLY, &dev->pm->signal);
	return 0;
}

DEVICE_DEFINE(dummy_driver, DUMMY_DRIVER_NAME, &dummy_init,
		    dummy_device_pm_ctrl, NULL, NULL, APPLICATION,
		    CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &funcs);
