/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <kernel.h>
#include <device.h>
#include <sys/__assert.h>

#define LOG_LEVEL CONFIG_SYS_PM_LOG_LEVEL /* From power module Kconfig */
#include <logging/log.h>
LOG_MODULE_DECLARE(power);

/* Device PM request type */
#define DEVICE_PM_SYNC			(0 << 0)
#define DEVICE_PM_ASYNC			(1 << 0)

static void device_pm_callback(struct device *dev,
			       int retval, void *context, void *arg)
{
	__ASSERT(retval == 0, "Device set power state failed");

	/* Set the fsm_state */
	if (*((u32_t *)context) == DEVICE_PM_ACTIVE_STATE) {
		atomic_set(&dev->config->pm->fsm_state,
				DEVICE_PM_FSM_STATE_ACTIVE);
	} else {
		atomic_set(&dev->config->pm->fsm_state,
				DEVICE_PM_FSM_STATE_SUSPENDED);
	}

	k_work_submit(&dev->config->pm->work);
}

static void pm_work_handler(struct k_work *work)
{
	struct device_pm *pm = CONTAINER_OF(work,
					struct device_pm, work);
	struct device *dev = pm->dev;
	int ret = 0;
	u8_t pm_state;

	switch (atomic_get(&dev->config->pm->fsm_state)) {
	case DEVICE_PM_FSM_STATE_ACTIVE:
		if ((atomic_get(&dev->config->pm->usage) == 0) &&
					dev->config->pm->enable) {
			atomic_set(&dev->config->pm->fsm_state,
					DEVICE_PM_FSM_STATE_SUSPENDING);
			ret = device_set_power_state(dev,
						DEVICE_PM_SUSPEND_STATE,
						device_pm_callback, NULL);
		} else {
			pm_state = DEVICE_PM_ACTIVE_STATE;
			goto fsm_out;
		}
		break;
	case DEVICE_PM_FSM_STATE_SUSPENDED:
		if ((atomic_get(&dev->config->pm->usage) > 0) ||
					!dev->config->pm->enable) {
			atomic_set(&dev->config->pm->fsm_state,
					DEVICE_PM_FSM_STATE_RESUMING);
			ret = device_set_power_state(dev,
						DEVICE_PM_ACTIVE_STATE,
						device_pm_callback, NULL);
		} else {
			pm_state = DEVICE_PM_SUSPEND_STATE;
			goto fsm_out;
		}
		break;
	case DEVICE_PM_FSM_STATE_SUSPENDING:
	case DEVICE_PM_FSM_STATE_RESUMING:
		/* Do nothing: We are waiting for device_pm_callback() */
		break;
	default:
		LOG_ERR("Invalid FSM state!!\n");
	}

	__ASSERT(ret == 0, "Set Power state error");

	return;

fsm_out:
	k_poll_signal_raise(&dev->config->pm->signal, pm_state);
}

static int device_pm_request(struct device *dev,
			     u32_t target_state, u32_t pm_flags)
{
	int result, signaled = 0;

	__ASSERT((target_state == DEVICE_PM_ACTIVE_STATE) ||
			(target_state == DEVICE_PM_SUSPEND_STATE),
			"Invalid device PM state requested");

	if (target_state == DEVICE_PM_ACTIVE_STATE) {
		if (atomic_inc(&dev->config->pm->usage) < 0) {
			return 0;
		}
	} else {
		if (atomic_dec(&dev->config->pm->usage) > 1) {
			return 0;
		}
	}

	k_work_submit(&dev->config->pm->work);

	/* Return in case of Async request */
	if (pm_flags & DEVICE_PM_ASYNC) {
		return 0;
	}

	/* Incase of Sync request wait for completion event */
	do {
		(void)k_poll(&dev->config->pm->event, 1, K_FOREVER);
		k_poll_signal_check(&dev->config->pm->signal,
						&signaled, &result);
	} while (!signaled);

	dev->config->pm->event.state = K_POLL_STATE_NOT_READY;
	k_poll_signal_reset(&dev->config->pm->signal);


	return result == target_state ? 0 : -EIO;
}

int device_pm_get(struct device *dev)
{
	return device_pm_request(dev,
			DEVICE_PM_ACTIVE_STATE, DEVICE_PM_ASYNC);
}

int device_pm_get_sync(struct device *dev)
{
	return device_pm_request(dev, DEVICE_PM_ACTIVE_STATE, 0);
}

int device_pm_put(struct device *dev)
{
	return device_pm_request(dev,
			DEVICE_PM_SUSPEND_STATE, DEVICE_PM_ASYNC);
}

int device_pm_put_sync(struct device *dev)
{
	return device_pm_request(dev, DEVICE_PM_SUSPEND_STATE, 0);
}

void device_pm_enable(struct device *dev)
{
	k_sem_take(&dev->config->pm->lock, K_FOREVER);
	dev->config->pm->enable = true;

	/* During the driver init, device can set the
	 * PM state accordingly. For later cases we need
	 * to check the usage and set the device PM state.
	 */
	if (!dev->config->pm->dev) {
		dev->config->pm->dev = dev;
		atomic_set(&dev->config->pm->fsm_state,
					DEVICE_PM_FSM_STATE_SUSPENDED);
		k_work_init(&dev->config->pm->work, pm_work_handler);
	} else {
		k_work_submit(&dev->config->pm->work);
	}
	k_sem_give(&dev->config->pm->lock);
}

void device_pm_disable(struct device *dev)
{
	k_sem_take(&dev->config->pm->lock, K_FOREVER);
	dev->config->pm->enable = false;
	/* Bring up the device before disabling the Idle PM */
	k_work_submit(&dev->config->pm->work);
	k_sem_give(&dev->config->pm->lock);
}
