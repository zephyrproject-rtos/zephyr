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

#define LOG_LEVEL CONFIG_PM_LOG_LEVEL /* From power module Kconfig */
#include <logging/log.h>
LOG_MODULE_DECLARE(power);

/* Device PM states */
enum device_pm_state {
	PM_DEVICE_STATE_ACTIVE = 1,
	PM_DEVICE_STATE_SUSPENDED,
	PM_DEVICE_STATE_SUSPENDING,
	PM_DEVICE_STATE_RESUMING,
};

/* Device PM request type */
#define PM_DEVICE_SYNC			(0 << 0)
#define PM_DEVICE_ASYNC			(1 << 0)

static void device_pm_callback(const struct device *dev,
			       int retval, void *context, void *arg)
{
	__ASSERT(retval == 0, "Device set power state failed");

	/* Set the fsm_state */
	if (*((uint32_t *)context) == PM_DEVICE_ACTIVE_STATE) {
		atomic_set(&dev->pm->fsm_state,
			   PM_DEVICE_STATE_ACTIVE);
	} else {
		atomic_set(&dev->pm->fsm_state,
			   PM_DEVICE_STATE_SUSPENDED);
	}

	k_work_submit(&dev->pm->work);
}

static void pm_work_handler(struct k_work *work)
{
	struct pm_device *pm = CONTAINER_OF(work,
					struct pm_device, work);
	const struct device *dev = pm->dev;
	int ret = 0;
	uint8_t pm_state;

	switch (atomic_get(&dev->pm->fsm_state)) {
	case PM_DEVICE_STATE_ACTIVE:
		if ((atomic_get(&dev->pm->usage) == 0) &&
					dev->pm->enable) {
			atomic_set(&dev->pm->fsm_state,
				   PM_DEVICE_STATE_SUSPENDING);
			ret = pm_device_state_set(dev, PM_DEVICE_SUSPEND_STATE,
						  device_pm_callback, NULL);
		} else {
			pm_state = PM_DEVICE_ACTIVE_STATE;
			goto fsm_out;
		}
		break;
	case PM_DEVICE_STATE_SUSPENDED:
		if ((atomic_get(&dev->pm->usage) > 0) ||
					!dev->pm->enable) {
			atomic_set(&dev->pm->fsm_state,
				   PM_DEVICE_STATE_RESUMING);
			ret = pm_device_state_set(dev, PM_DEVICE_ACTIVE_STATE,
						  device_pm_callback, NULL);
		} else {
			pm_state = PM_DEVICE_SUSPEND_STATE;
			goto fsm_out;
		}
		break;
	case PM_DEVICE_STATE_SUSPENDING:
	case PM_DEVICE_STATE_RESUMING:
		/* Do nothing: We are waiting for device_pm_callback() */
		break;
	default:
		LOG_ERR("Invalid FSM state!!\n");
	}

	__ASSERT(ret == 0, "Set Power state error");

	return;

fsm_out:
	k_poll_signal_raise(&dev->pm->signal, pm_state);
}

static int pm_device_request(const struct device *dev,
			     uint32_t target_state, uint32_t pm_flags)
{
	int result, signaled = 0;

	__ASSERT((target_state == PM_DEVICE_ACTIVE_STATE) ||
			(target_state == PM_DEVICE_SUSPEND_STATE),
			"Invalid device PM state requested");

	if (target_state == PM_DEVICE_ACTIVE_STATE) {
		if (atomic_inc(&dev->pm->usage) < 0) {
			return 0;
		}
	} else {
		if (atomic_dec(&dev->pm->usage) > 1) {
			return 0;
		}
	}

	k_work_submit(&dev->pm->work);

	/* Return in case of Async request */
	if (pm_flags & PM_DEVICE_ASYNC) {
		return 0;
	}

	/* Incase of Sync request wait for completion event */
	do {
		(void)k_poll(&dev->pm->event, 1, K_FOREVER);
		k_poll_signal_check(&dev->pm->signal,
						&signaled, &result);
	} while (!signaled);

	dev->pm->event.state = K_POLL_STATE_NOT_READY;
	k_poll_signal_reset(&dev->pm->signal);


	return result == target_state ? 0 : -EIO;
}

int pm_device_get(const struct device *dev)
{
	return pm_device_request(dev,
			PM_DEVICE_ACTIVE_STATE, PM_DEVICE_ASYNC);
}

int pm_device_get_sync(const struct device *dev)
{
	return pm_device_request(dev, PM_DEVICE_ACTIVE_STATE, 0);
}

int pm_device_put(const struct device *dev)
{
	return pm_device_request(dev,
			PM_DEVICE_SUSPEND_STATE, PM_DEVICE_ASYNC);
}

int pm_device_put_sync(const struct device *dev)
{
	return pm_device_request(dev, PM_DEVICE_SUSPEND_STATE, 0);
}

void pm_device_enable(const struct device *dev)
{
	k_sem_take(&dev->pm->lock, K_FOREVER);
	dev->pm->enable = true;

	/* During the driver init, device can set the
	 * PM state accordingly. For later cases we need
	 * to check the usage and set the device PM state.
	 */
	if (!dev->pm->dev) {
		dev->pm->dev = dev;
		atomic_set(&dev->pm->fsm_state,
			   PM_DEVICE_STATE_SUSPENDED);
		k_work_init(&dev->pm->work, pm_work_handler);
	} else {
		k_work_submit(&dev->pm->work);
	}
	k_sem_give(&dev->pm->lock);
}

void pm_device_disable(const struct device *dev)
{
	k_sem_take(&dev->pm->lock, K_FOREVER);
	dev->pm->enable = false;
	/* Bring up the device before disabling the Idle PM */
	k_work_submit(&dev->pm->work);
	k_sem_give(&dev->pm->lock);
}
