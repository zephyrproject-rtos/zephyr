/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <kernel.h>
#include <device.h>
#include <sys/__assert.h>
#include <pm/device_runtime.h>
#include <spinlock.h>

#define LOG_LEVEL CONFIG_PM_LOG_LEVEL /* From power module Kconfig */
#include <logging/log.h>
LOG_MODULE_DECLARE(power);

/* Device PM request type */
#define PM_DEVICE_SYNC          BIT(0)
#define PM_DEVICE_ASYNC         BIT(1)

static void pm_device_runtime_state_set(struct pm_device *pm)
{
	const struct device *dev = pm->dev;
	int ret = 0;

	switch (dev->pm->state) {
	case PM_DEVICE_STATE_ACTIVE:
		if ((dev->pm->usage == 0) && dev->pm->enable) {
			dev->pm->state = PM_DEVICE_STATE_SUSPENDING;
			ret = pm_device_state_set(dev, PM_DEVICE_STATE_SUSPEND);
			if (ret == 0) {
				dev->pm->state = PM_DEVICE_STATE_SUSPEND;
			}
		}
		break;
	case PM_DEVICE_STATE_SUSPEND:
		if ((dev->pm->usage > 0) || !dev->pm->enable) {
			dev->pm->state = PM_DEVICE_STATE_RESUMING;
			ret = pm_device_state_set(dev, PM_DEVICE_STATE_ACTIVE);
			if (ret == 0) {
				dev->pm->state = PM_DEVICE_STATE_ACTIVE;
			}
		}
		break;
	case PM_DEVICE_STATE_SUSPENDING:
		__fallthrough;
	case PM_DEVICE_STATE_RESUMING:
		/* Do nothing: We are waiting for resume/suspend to finish */
		break;
	default:
		LOG_ERR("Invalid state!!\n");
	}

	__ASSERT(ret == 0, "Set Power state error");

	/*
	 * This function returns the number of woken threads on success. There
	 * is nothing we can do with this information. Just ignoring it.
	 */
	(void)k_condvar_broadcast(&dev->pm->condvar);
}

static void pm_work_handler(struct k_work *work)
{
	struct pm_device *pm = CONTAINER_OF(work,
					struct pm_device, work);

	(void)k_mutex_lock(&pm->lock, K_FOREVER);
	pm_device_runtime_state_set(pm);
	(void)k_mutex_unlock(&pm->lock);
}

static int pm_device_request(const struct device *dev,
			     uint32_t target_state, uint32_t pm_flags)
{
	int ret = 0;

	SYS_PORT_TRACING_FUNC_ENTER(pm, device_request, dev, target_state);
	__ASSERT((target_state == PM_DEVICE_STATE_ACTIVE) ||
			(target_state == PM_DEVICE_STATE_SUSPEND),
			"Invalid device PM state requested");

	if (k_is_pre_kernel()) {
		if (target_state == PM_DEVICE_STATE_ACTIVE) {
			dev->pm->usage++;
		} else {
			dev->pm->usage--;
		}

		/* If we are being called before the kernel was initialized
		 * we can assume that the system took care of initialized
		 * devices properly. It means that all dependencies were
		 * satisfied and this call just incremented the reference count
		 * for this device.
		 */

		/* Unfortunately this is not what is happening yet. There are
		 * cases, for example, like the pinmux being initialized before
		 * the gpio. Lets just power on/off the device.
		 */
		if (dev->pm->usage == 1) {
			(void)pm_device_state_set(dev, PM_DEVICE_STATE_ACTIVE);
		} else if (dev->pm->usage == 0) {
			(void)pm_device_state_set(dev, PM_DEVICE_STATE_SUSPEND);
		}
		goto out;
	}

	(void)k_mutex_lock(&dev->pm->lock, K_FOREVER);

	if (!dev->pm->enable) {
		ret = -ENOTSUP;
		goto out_unlock;
	}

	if (target_state == PM_DEVICE_STATE_ACTIVE) {
		dev->pm->usage++;
	} else {
		dev->pm->usage--;
	}


	/* Return in case of Async request */
	if (pm_flags & PM_DEVICE_ASYNC) {
		(void)k_work_schedule(&dev->pm->work, K_NO_WAIT);
		goto out_unlock;
	}

	while ((k_work_delayable_is_pending(&dev->pm->work)) ||
		(dev->pm->state == PM_DEVICE_STATE_SUSPENDING) ||
		(dev->pm->state == PM_DEVICE_STATE_RESUMING)) {
		ret = k_condvar_wait(&dev->pm->condvar, &dev->pm->lock,
			       K_FOREVER);
		if (ret != 0) {
			break;
		}
	}

	pm_device_runtime_state_set(dev->pm);

	/*
	 * dev->pm->state was set in pm_device_runtime_state_set(). As the
	 * device may not have been properly changed to the target_state or
	 * another thread we check it here before returning.
	 */
	ret = target_state == dev->pm->state ? 0 : -EIO;

out_unlock:
	(void)k_mutex_unlock(&dev->pm->lock);
out:
	SYS_PORT_TRACING_FUNC_EXIT(pm, device_request, dev, ret);
	return ret;
}

int pm_device_get(const struct device *dev)
{
	return pm_device_request(dev, PM_DEVICE_STATE_ACTIVE, 0);
}

int pm_device_get_async(const struct device *dev)
{
	return pm_device_request(dev, PM_DEVICE_STATE_ACTIVE, PM_DEVICE_ASYNC);
}

int pm_device_put(const struct device *dev)
{
	return pm_device_request(dev, PM_DEVICE_STATE_SUSPEND, 0);
}

int pm_device_put_async(const struct device *dev)
{
	return pm_device_request(dev, PM_DEVICE_STATE_SUSPEND, PM_DEVICE_ASYNC);
}

void pm_device_enable(const struct device *dev)
{
	SYS_PORT_TRACING_FUNC_ENTER(pm, device_enable, dev);
	if (k_is_pre_kernel()) {
		dev->pm->dev = dev;
		if (dev->pm_control != NULL) {
			dev->pm->enable = true;
			dev->pm->state = PM_DEVICE_STATE_SUSPEND;
			k_work_init_delayable(&dev->pm->work, pm_work_handler);
		}
		goto out;
	}

	(void)k_mutex_lock(&dev->pm->lock, K_FOREVER);
	if (dev->pm_control == NULL) {
		dev->pm->enable = false;
		goto out_unlock;
	}

	dev->pm->enable = true;

	/* During the driver init, device can set the
	 * PM state accordingly. For later cases we need
	 * to check the usage and set the device PM state.
	 */
	if (!dev->pm->dev) {
		dev->pm->dev = dev;
		dev->pm->state = PM_DEVICE_STATE_SUSPEND;
		k_work_init_delayable(&dev->pm->work, pm_work_handler);
	} else {
		k_work_schedule(&dev->pm->work, K_NO_WAIT);
	}

out_unlock:
	(void)k_mutex_unlock(&dev->pm->lock);
out:
	SYS_PORT_TRACING_FUNC_EXIT(pm, device_enable, dev);
}

void pm_device_disable(const struct device *dev)
{
	SYS_PORT_TRACING_FUNC_ENTER(pm, device_disable, dev);
	__ASSERT(k_is_pre_kernel() == false, "Device should not be disabled "
		 "before kernel is initialized");

	(void)k_mutex_lock(&dev->pm->lock, K_FOREVER);
	if (dev->pm->enable) {
		dev->pm->enable = false;
		/* Bring up the device before disabling the Idle PM */
		k_work_schedule(&dev->pm->work, K_NO_WAIT);
	}
	(void)k_mutex_unlock(&dev->pm->lock);
	SYS_PORT_TRACING_FUNC_EXIT(pm, device_disable, dev);
}

int pm_device_wait(const struct device *dev, k_timeout_t timeout)
{
	int ret = 0;

	k_mutex_lock(&dev->pm->lock, K_FOREVER);
	while ((k_work_delayable_is_pending(&dev->pm->work)) ||
		(dev->pm->state == PM_DEVICE_STATE_SUSPENDING) ||
		(dev->pm->state == PM_DEVICE_STATE_RESUMING)) {
		ret = k_condvar_wait(&dev->pm->condvar, &dev->pm->lock,
			       timeout);
		if (ret != 0) {
			break;
		}
	}
	k_mutex_unlock(&dev->pm->lock);

	return ret;
}
