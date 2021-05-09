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

static void device_pm_callback(const struct device *dev,
			       int retval, uint32_t *state, void *arg)
{
	__ASSERT(retval == 0, "Device set power state failed");

	atomic_set(&dev->pm->state, *state);

	/*
	 * This function returns the number of woken threads on success. There
	 * is nothing we can do with this information. Just ignore it.
	 */
	(void)k_condvar_broadcast(&dev->pm->condvar);
}

static void pm_work_handler(struct k_work *work)
{
	struct pm_device *pm = CONTAINER_OF(work,
					struct pm_device, work);
	const struct device *dev = pm->dev;
	int ret = 0;

	switch (atomic_get(&dev->pm->state)) {
	case PM_DEVICE_STATE_ACTIVE:
		if ((atomic_get(&dev->pm->usage) == 0) &&
					dev->pm->enable) {
			atomic_set(&dev->pm->state,
				   PM_DEVICE_STATE_SUSPENDING);
			ret = pm_device_state_set(dev, PM_DEVICE_STATE_SUSPEND,
						  device_pm_callback, NULL);
		} else {
			goto fsm_out;
		}
		break;
	case PM_DEVICE_STATE_SUSPEND:
		if ((atomic_get(&dev->pm->usage) > 0) ||
					!dev->pm->enable) {
			atomic_set(&dev->pm->state,
				   PM_DEVICE_STATE_RESUMING);
			ret = pm_device_state_set(dev, PM_DEVICE_STATE_ACTIVE,
						  device_pm_callback, NULL);
		} else {
			goto fsm_out;
		}
		break;
	case PM_DEVICE_STATE_SUSPENDING:
		__fallthrough;
	case PM_DEVICE_STATE_RESUMING:
		/* Do nothing: We are waiting for device_pm_callback() */
		break;
	default:
		LOG_ERR("Invalid FSM state!!\n");
	}

	__ASSERT(ret == 0, "Set Power state error");
	return;

fsm_out:
	/*
	 * This function returns the number of woken threads on success. There
	 * is nothing we can do with this information. Just ignoring it.
	 */
	(void)k_condvar_broadcast(&dev->pm->condvar);
}

static int pm_device_request(const struct device *dev,
			     uint32_t target_state, uint32_t pm_flags)
{
	struct k_mutex request_mutex;

	__ASSERT((target_state == PM_DEVICE_STATE_ACTIVE) ||
			(target_state == PM_DEVICE_STATE_SUSPEND),
			"Invalid device PM state requested");

	if (target_state == PM_DEVICE_STATE_ACTIVE) {
		if (atomic_inc(&dev->pm->usage) < 0) {
			return 0;
		}
	} else {
		if (atomic_dec(&dev->pm->usage) > 1) {
			return 0;
		}
	}

	if (k_is_pre_kernel()) {
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
			(void)pm_device_state_set(dev,
						  PM_DEVICE_STATE_ACTIVE,
						  NULL, NULL);
		} else if (dev->pm->usage == 0) {
			(void)pm_device_state_set(dev,
						  PM_DEVICE_STATE_SUSPEND,
						  NULL, NULL);
		}

		return 0;
	}

	(void)k_work_schedule(&dev->pm->work, K_NO_WAIT);

	/* Return in case of Async request */
	if (pm_flags & PM_DEVICE_ASYNC) {
		return 0;
	}

	k_mutex_init(&request_mutex);
	k_mutex_lock(&request_mutex, K_FOREVER);
	(void)k_condvar_wait(&dev->pm->condvar, &request_mutex, K_FOREVER);
	k_mutex_unlock(&request_mutex);

	/*
	 * dev->pm->state was set in device_pm_callback(). As the device
	 * may not have been properly changed to the target_state or another
	 * thread we check it here before returning.
	 */
	return target_state == atomic_get(&dev->pm->state) ? 0 : -EIO;
}

int pm_device_get(const struct device *dev)
{
	return pm_device_request(dev,
			PM_DEVICE_STATE_ACTIVE, PM_DEVICE_ASYNC);
}

int pm_device_get_sync(const struct device *dev)
{
	return pm_device_request(dev, PM_DEVICE_STATE_ACTIVE, 0);
}

int pm_device_put(const struct device *dev)
{
	return pm_device_request(dev,
			PM_DEVICE_STATE_SUSPEND, PM_DEVICE_ASYNC);
}

int pm_device_put_sync(const struct device *dev)
{
	return pm_device_request(dev, PM_DEVICE_STATE_SUSPEND, 0);
}

void pm_device_enable(const struct device *dev)
{
	k_spinlock_key_t key;

	if (k_is_pre_kernel()) {
		dev->pm->dev = dev;
		dev->pm->enable = true;
		atomic_set(&dev->pm->state, PM_DEVICE_STATE_SUSPEND);
		k_work_init_delayable(&dev->pm->work, pm_work_handler);
		return;
	}

	key = k_spin_lock(&dev->pm->lock);
	dev->pm->enable = true;

	/* During the driver init, device can set the
	 * PM state accordingly. For later cases we need
	 * to check the usage and set the device PM state.
	 */
	if (!dev->pm->dev) {
		dev->pm->dev = dev;
		atomic_set(&dev->pm->state, PM_DEVICE_STATE_SUSPEND);
		k_work_init_delayable(&dev->pm->work, pm_work_handler);
	} else {
		k_work_schedule(&dev->pm->work, K_NO_WAIT);
	}
	k_spin_unlock(&dev->pm->lock, key);
}

void pm_device_disable(const struct device *dev)
{
	k_spinlock_key_t key;

	__ASSERT(k_is_pre_kernel() == false, "Device should not be disabled "
		 "before kernel is initialized");

	key = k_spin_lock(&dev->pm->lock);
	dev->pm->enable = false;
	/* Bring up the device before disabling the Idle PM */
	k_work_schedule(&dev->pm->work, K_NO_WAIT);
	k_spin_unlock(&dev->pm->lock, key);
}
