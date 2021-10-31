/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/__assert.h>
#include <pm/device_runtime.h>

#include <logging/log.h>
LOG_MODULE_DECLARE(pm_device, CONFIG_PM_DEVICE_LOG_LEVEL);

/* Device PM request type */
#define PM_DEVICE_SYNC          BIT(0)
#define PM_DEVICE_ASYNC         BIT(1)

static void pm_device_runtime_state_set(struct pm_device *pm)
{
	const struct device *dev = pm->dev;
	int ret = 0;

	/* Clear transitioning flags */
	atomic_clear_bit(&pm->flags, PM_DEVICE_FLAG_TRANSITIONING);

	switch (pm->state) {
	case PM_DEVICE_STATE_ACTIVE:
		if ((pm->usage == 0) && pm->enable) {
			ret = pm_device_state_set(dev, PM_DEVICE_STATE_SUSPENDED);
		}
		break;
	case PM_DEVICE_STATE_SUSPENDED:
		if ((pm->usage > 0) || !pm->enable) {
			ret = pm_device_state_set(dev, PM_DEVICE_STATE_ACTIVE);
		}
		break;
	default:
		LOG_ERR("Invalid state!!\n");
	}

	__ASSERT(ret == 0, "Set Power state error");

	/*
	 * This function returns the number of woken threads on success. There
	 * is nothing we can do with this information. Just ignoring it.
	 */
	(void)k_condvar_broadcast(&pm->condvar);
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
			enum pm_device_state state, uint32_t pm_flags)
{
	int ret = 0;
	struct pm_device *pm = dev->pm;

	SYS_PORT_TRACING_FUNC_ENTER(pm, device_request, dev, state);

	__ASSERT((state == PM_DEVICE_STATE_ACTIVE) ||
			(state == PM_DEVICE_STATE_SUSPENDED),
			"Invalid device PM state requested");

	if (k_is_pre_kernel()) {
		if (state == PM_DEVICE_STATE_ACTIVE) {
			pm->usage++;
		} else {
			pm->usage--;
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
		if (pm->usage == 1) {
			(void)pm_device_state_set(dev, PM_DEVICE_STATE_ACTIVE);
		} else if (pm->usage == 0) {
			(void)pm_device_state_set(dev, PM_DEVICE_STATE_SUSPENDED);
		}
		goto out;
	}

	(void)k_mutex_lock(&pm->lock, K_FOREVER);

	if (!pm->enable) {
		ret = -ENOTSUP;
		goto out_unlock;
	}

	if (state == PM_DEVICE_STATE_ACTIVE) {
		pm->usage++;
		if (pm->usage > 1) {
			goto out_unlock;
		}
	} else {
		/* Check if it is already 0 to avoid an underflow */
		if (pm->usage == 0) {
			goto out_unlock;
		}

		pm->usage--;
		if (pm->usage > 0) {
			goto out_unlock;
		}
	}


	/* Return in case of Async request */
	if (pm_flags & PM_DEVICE_ASYNC) {
		atomic_set_bit(&pm->flags, PM_DEVICE_FLAG_TRANSITIONING);
		(void)k_work_schedule(&pm->work, K_NO_WAIT);
		goto out_unlock;
	}

	while ((k_work_delayable_is_pending(&pm->work)) ||
	       atomic_test_bit(&pm->flags, PM_DEVICE_FLAG_TRANSITIONING)) {
		ret = k_condvar_wait(&pm->condvar, &pm->lock,
			       K_FOREVER);
		if (ret != 0) {
			break;
		}
	}

	pm_device_runtime_state_set(pm);

	/*
	 * pm->state was set in pm_device_runtime_state_set(). As the
	 * device may not have been properly changed to the state or
	 * another thread we check it here before returning.
	 */
	ret = state == pm->state ? 0 : -EIO;

out_unlock:
	(void)k_mutex_unlock(&pm->lock);
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
	return pm_device_request(dev, PM_DEVICE_STATE_SUSPENDED, 0);
}

int pm_device_put_async(const struct device *dev)
{
	return pm_device_request(dev, PM_DEVICE_STATE_SUSPENDED, PM_DEVICE_ASYNC);
}

void pm_device_enable(const struct device *dev)
{
	struct pm_device *pm = dev->pm;

	SYS_PORT_TRACING_FUNC_ENTER(pm, device_enable, dev);
	if (k_is_pre_kernel()) {
		pm->dev = dev;
		if (pm->action_cb != NULL) {
			pm->enable = true;
			pm->state = PM_DEVICE_STATE_SUSPENDED;
			k_work_init_delayable(&pm->work, pm_work_handler);
		}
		goto out;
	}

	(void)k_mutex_lock(&pm->lock, K_FOREVER);
	if (pm->action_cb == NULL) {
		pm->enable = false;
		goto out_unlock;
	}

	pm->enable = true;

	/* During the driver init, device can set the
	 * PM state accordingly. For later cases we need
	 * to check the usage and set the device PM state.
	 */
	if (!pm->dev) {
		pm->dev = dev;
		pm->state = PM_DEVICE_STATE_SUSPENDED;
		k_work_init_delayable(&pm->work, pm_work_handler);
	} else {
		k_work_schedule(&pm->work, K_NO_WAIT);
	}

out_unlock:
	(void)k_mutex_unlock(&pm->lock);
out:
	SYS_PORT_TRACING_FUNC_EXIT(pm, device_enable, dev);
}

void pm_device_disable(const struct device *dev)
{
	struct pm_device *pm = dev->pm;

	SYS_PORT_TRACING_FUNC_ENTER(pm, device_disable, dev);
	__ASSERT(k_is_pre_kernel() == false, "Device should not be disabled "
		 "before kernel is initialized");

	(void)k_mutex_lock(&pm->lock, K_FOREVER);
	if (pm->enable) {
		pm->enable = false;
		/* Bring up the device before disabling the Idle PM */
		k_work_schedule(&pm->work, K_NO_WAIT);
	}
	(void)k_mutex_unlock(&pm->lock);
	SYS_PORT_TRACING_FUNC_EXIT(pm, device_disable, dev);
}

int pm_device_wait(const struct device *dev, k_timeout_t timeout)
{
	int ret = 0;
	struct pm_device *pm = dev->pm;

	k_mutex_lock(&pm->lock, K_FOREVER);
	while ((k_work_delayable_is_pending(&pm->work)) ||
	       atomic_test_bit(&pm->flags, PM_DEVICE_FLAG_TRANSITIONING)) {
		ret = k_condvar_wait(&pm->condvar, &pm->lock,
			       timeout);
		if (ret != 0) {
			break;
		}
	}
	k_mutex_unlock(&pm->lock);

	return ret;
}
