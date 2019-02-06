/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <kernel.h>
#include <init.h>
#include <misc/__assert.h>
#include <string.h>
#include <soc.h>
#include <device.h>
#include "policy/pm_policy.h"

#define LOG_LEVEL CONFIG_PM_LOG_LEVEL /* From power module Kconfig */
#include <logging/log.h>
LOG_MODULE_DECLARE(power);

/* Device PM request type */
#define DEVICE_PM_ASYNC			(1 << 0)

/* device PM worker queue thread info */
static K_THREAD_STACK_DEFINE(devpm_workq_stack,
			CONFIG_DEVPM_WORKQUEUE_STACK_SIZE);
struct k_work_q devpm_workq;

/*
 * Set the device power state to target
 * state based on the current state.
 */
static int device_pm_state_handler(struct device *dev, u32_t target_state)
{
	u32_t current_state;
	int ret = 0;

	/* Certain devices may take lot of time to switch
	 * between power states. So to avoid blocking of
	 * Async API's for longer periods we are using a
	 * dedicated lock to control power state transitions
	 * between Sync and Async calls.
	 */
	k_sem_take(&dev->pm->pm_lock, K_FOREVER);

	/* Get current device power state */
	ret = device_get_power_state(dev, &current_state);
	if (ret) {
		LOG_ERR("Get power state error(%d)\n", ret);
		goto handler_out;
	}

	/* Check if current power state is same as target
	 * state so that we can skip processing new request.
	 */
	if (current_state == target_state) {
		goto handler_out;
	}

	/* Incase of suspend request, make sure the device is not busy */
	if (device_busy_check(dev) &&
			(target_state == DEVICE_PM_SUSPEND_STATE)) {
		ret = -EBUSY;
		goto handler_out;
	}

	ret = device_set_power_state(dev, target_state);
	if (ret < 0) {
		LOG_ERR("Set power state(%d) error(%d)\n",
						target_state, ret);
	}

handler_out:
	switch (target_state) {
	case DEVICE_PM_ACTIVE_STATE:
		if (ret < 0) {
			dev->pm->pm_status = DEVICE_PM_STATUS_WAKEUP_ERR;
		} else {
			dev->pm->pm_status = DEVICE_PM_STATUS_ACTIVE;
		}
		break;
	case DEVICE_PM_SUSPEND_STATE:
		if (ret < 0) {
			dev->pm->pm_status = DEVICE_PM_STATUS_SUSPEND_ERR;
		} else {
			dev->pm->pm_status = DEVICE_PM_STATUS_SUSPENDED;
		}
		break;
	default:
		LOG_ERR("Invalid PM state requested (%d)\n", target_state);
		ret = -EINVAL;
	}

	/* Notify the driver through signal */
	k_poll_signal_raise(&dev->pm->signal, dev->pm->pm_status);
	k_sem_give(&dev->pm->pm_lock);
	return ret;
}

/*
 * Device PM worker to handle the Async PM requests
 * from device_pm_get/device_pm_put API's. The worker
 * will call the device_pm_state_handler() to put the
 * device in desired power state and notify the caller
 * through signal mechanism.
 */
static void pm_work_handler(struct k_work *work)
{
	struct device_pm *pm = CONTAINER_OF(work,
					struct device_pm, dwork);
	struct device *dev = pm->dev;
	int ret;
	u32_t request;

	while (1) {
		k_sem_take(&dev->pm->lock, K_FOREVER);
		if (dev->pm->async_req_pending) {
			request = dev->pm->async_request;
			dev->pm->async_req_pending = false;
		} else {
			k_sem_give(&dev->pm->lock);
			break;
		}
		k_sem_give(&dev->pm->lock);

		ret = device_pm_state_handler(dev, request);
	}
}

static int device_pm_request(struct device *dev,
			     u32_t target_state, u32_t pm_flags)
{
	int ret = 0;

	__ASSERT((target_state == DEVICE_PM_ACTIVE_STATE) ||
			(target_state == DEVICE_PM_SUSPEND_STATE),
			"Invalid device PM state requested");

	if (target_state == DEVICE_PM_ACTIVE_STATE) {
		if (atomic_inc(&dev->pm->usage) < 0) {
			goto req_out;
		}
	} else {
		if (atomic_dec(&dev->pm->usage) > 1) {
			goto req_out;
		}
	}

	k_sem_take(&dev->pm->lock, K_FOREVER);
	if (!dev->pm->enable) {
		k_sem_give(&dev->pm->lock);
		goto req_out;
	}

	/* Cancel the already queued work */
	k_delayed_work_cancel(&dev->pm->dwork);

	if (pm_flags & DEVICE_PM_ASYNC) {
		dev->pm->async_request = target_state;
		dev->pm->async_req_pending = true;
		k_delayed_work_submit_to_queue(&devpm_workq,
					&dev->pm->dwork, dev->pm->timeout);
	}
	k_sem_give(&dev->pm->lock);

	if (!(pm_flags & DEVICE_PM_ASYNC)) {
		ret = device_pm_state_handler(dev, target_state);
	}

req_out:
	return ret;
}

int device_pm_get(struct device *dev)
{
	return device_pm_request(dev, DEVICE_PM_ACTIVE_STATE, DEVICE_PM_ASYNC);
}

int device_pm_get_sync(struct device *dev)
{
	return device_pm_request(dev, DEVICE_PM_ACTIVE_STATE, 0);
}

int device_pm_put(struct device *dev)
{
	return device_pm_request(dev, DEVICE_PM_SUSPEND_STATE, DEVICE_PM_ASYNC);
}

int device_pm_put_sync(struct device *dev)
{
	return device_pm_request(dev, DEVICE_PM_SUSPEND_STATE, 0);
}

void device_pm_enable(struct device *dev)
{
	k_sem_take(&dev->pm->lock, K_FOREVER);
	dev->pm->enable = true;

	/* During the driver init, device can set the
	 * PM state accordingly. For later cases we need
	 * to check the usage and set the device PM state.
	 */
	if (!dev->pm->dwork.work.handler) {
		dev->pm->dev = dev;
		k_delayed_work_init(&dev->pm->dwork, pm_work_handler);
		k_poll_signal_init(&dev->pm->signal);
	} else {
		k_delayed_work_cancel(&dev->pm->dwork);
		if (dev->pm->usage) {
			dev->pm->async_request = DEVICE_PM_ACTIVE_STATE;
		} else {
			dev->pm->async_request = DEVICE_PM_SUSPEND_STATE;
		}
		dev->pm->async_req_pending = true;
		k_delayed_work_submit_to_queue(&devpm_workq,
						&dev->pm->dwork, 0);
	}
	k_sem_give(&dev->pm->lock);
}

void device_pm_disable(struct device *dev)
{
	k_sem_take(&dev->pm->lock, K_FOREVER);
	k_delayed_work_cancel(&dev->pm->dwork);
	/* Bring up the device before disabling the Idle PM */
	dev->pm->async_request = DEVICE_PM_ACTIVE_STATE;
	dev->pm->async_req_pending = true;
	k_delayed_work_submit_to_queue(&devpm_workq, &dev->pm->dwork, 0);
	dev->pm->enable = false;
	k_sem_give(&dev->pm->lock);
}

/* System init function to initialize PM work queue thread */
static int devpm_workq_init(struct device *dev)
{
	ARG_UNUSED(dev);

	k_work_q_start(&devpm_workq,
		       devpm_workq_stack,
		       K_THREAD_STACK_SIZEOF(devpm_workq_stack),
		       K_LOWEST_THREAD_PRIO - 1);
	k_thread_name_set(&devpm_workq.thread, "pmworkq");
	return 0;
}

SYS_INIT(devpm_workq_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
