/*
 * Copyright (c) 2018 Intel Corporation.
 * Copyright (c) 2021 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <pm/device.h>
#include <pm/device_runtime.h>
#include <sys/__assert.h>

#include <logging/log.h>
LOG_MODULE_DECLARE(pm_device, CONFIG_PM_DEVICE_LOG_LEVEL);

/**
 * @brief Suspend a device
 *
 * @note Asynchronous operations are not supported when in pre-kernel mode. In
 * this case, the async flag will be always forced to be false, and so the
 * the function will be blocking.
 *
 * @funcprops \pre_kernel_ok
 *
 * @param dev Device instance.
 * @param async Perform operation asynchronously.
 *
 * @retval 0 If device has been suspended or queued for suspend.
 * @retval -ENOSTUP If runtime PM is not enabled for the device.
 * @retval -EALREADY If device is already suspended (can only happen if get/put
 * calls are unbalanced).
 * @retval -errno Other negative errno, result of the action callback.
 */
static int runtime_suspend(const struct device *dev, bool async)
{
	int ret = 0;
	struct pm_device *pm = dev->pm;

	if (k_is_pre_kernel()) {
		async = false;
	} else {
		(void)k_mutex_lock(&pm->lock, K_FOREVER);
	}

	if ((pm->flags & BIT(PM_DEVICE_FLAG_RUNTIME_ENABLED)) == 0U) {
		ret = -ENOTSUP;
		goto unlock;
	}

	if (pm->usage == 0U) {
		LOG_WRN("Unbalanced suspend");
		ret = -EALREADY;
		goto unlock;
	}

	pm->usage--;
	if (pm->usage > 0U) {
		goto unlock;
	}

	if (async && !k_is_pre_kernel()) {
		/* queue suspend */
		pm->state = PM_DEVICE_STATE_SUSPENDING;
		(void)k_work_schedule(&pm->work, K_NO_WAIT);
	} else {
		/* suspend now */
		ret = pm->action_cb(pm->dev, PM_DEVICE_ACTION_SUSPEND);
		if (ret < 0) {
			pm->usage++;
			goto unlock;
		}

		pm->state = PM_DEVICE_STATE_SUSPENDED;
	}

unlock:
	if (!k_is_pre_kernel()) {
		k_mutex_unlock(&pm->lock);
	}

	return ret;
}

static void runtime_suspend_work(struct k_work *work)
{
	int ret;
	struct pm_device *pm = CONTAINER_OF(work, struct pm_device, work);

	ret = pm->action_cb(pm->dev, PM_DEVICE_ACTION_SUSPEND);

	(void)k_mutex_lock(&pm->lock, K_FOREVER);
	if (ret == 0) {
		pm->state = PM_DEVICE_STATE_SUSPENDED;
	}
	k_condvar_broadcast(&pm->condvar);
	k_mutex_unlock(&pm->lock);

	__ASSERT(ret == 0, "Could not suspend device (%d)", ret);
}

int pm_device_runtime_get(const struct device *dev)
{
	int ret = 0;
	struct pm_device *pm = dev->pm;

	SYS_PORT_TRACING_FUNC_ENTER(pm, device_runtime_get, dev);

	if (!k_is_pre_kernel()) {
		(void)k_mutex_lock(&pm->lock, K_FOREVER);
	}

	if ((pm->flags & BIT(PM_DEVICE_FLAG_RUNTIME_ENABLED)) == 0U) {
		ret = -ENOTSUP;
		goto unlock;
	}

	pm->usage++;

	if (!k_is_pre_kernel()) {
		/* wait until possible async suspend is completed */
		while (pm->state == PM_DEVICE_STATE_SUSPENDING) {
			(void)k_condvar_wait(&pm->condvar, &pm->lock, K_FOREVER);
		}
	}

	if (pm->usage > 1U) {
		goto unlock;
	}

	ret = pm->action_cb(pm->dev, PM_DEVICE_ACTION_RESUME);
	if (ret < 0) {
		pm->usage--;
		goto unlock;
	}

	pm->state = PM_DEVICE_STATE_ACTIVE;

unlock:
	if (!k_is_pre_kernel()) {
		k_mutex_unlock(&pm->lock);
	}

	SYS_PORT_TRACING_FUNC_EXIT(pm, device_runtime_get, dev, ret);

	return ret;
}

int pm_device_runtime_put(const struct device *dev)
{
	int ret;

	SYS_PORT_TRACING_FUNC_ENTER(pm, device_runtime_put, dev);
	ret = runtime_suspend(dev, false);
	SYS_PORT_TRACING_FUNC_EXIT(pm, device_runtime_put, dev, ret);

	return ret;
}

int pm_device_runtime_put_async(const struct device *dev)
{
	int ret;

	SYS_PORT_TRACING_FUNC_ENTER(pm, device_runtime_put_async, dev);
	ret = runtime_suspend(dev, true);
	SYS_PORT_TRACING_FUNC_EXIT(pm, device_runtime_put_async, dev, ret);

	return ret;
}

int pm_device_runtime_enable(const struct device *dev)
{
	int ret = 0;
	struct pm_device *pm = dev->pm;

	SYS_PORT_TRACING_FUNC_ENTER(pm, device_runtime_enable, dev);

	if (pm_device_state_is_locked(dev)) {
		ret = -EPERM;
		goto end;
	}

	if (!k_is_pre_kernel()) {
		(void)k_mutex_lock(&pm->lock, K_FOREVER);
	}

	if ((pm->flags & BIT(PM_DEVICE_FLAG_RUNTIME_ENABLED)) != 0U) {
		goto unlock;
	}

	/* lazy init of PM fields */
	if (pm->dev == NULL) {
		pm->dev = dev;
		k_work_init_delayable(&pm->work, runtime_suspend_work);
	}
	pm->state = PM_DEVICE_STATE_SUSPENDED;
	pm->usage = 0U;

	atomic_set_bit(&pm->flags, PM_DEVICE_FLAG_RUNTIME_ENABLED);

unlock:
	if (!k_is_pre_kernel()) {
		k_mutex_unlock(&pm->lock);
	}

end:
	SYS_PORT_TRACING_FUNC_EXIT(pm, device_runtime_enable, dev, ret);
	return ret;
}

int pm_device_runtime_disable(const struct device *dev)
{
	int ret = 0;
	struct pm_device *pm = dev->pm;

	SYS_PORT_TRACING_FUNC_ENTER(pm, device_runtime_disable, dev);

	if (!k_is_pre_kernel()) {
		(void)k_mutex_lock(&pm->lock, K_FOREVER);
	}

	if ((pm->flags & BIT(PM_DEVICE_FLAG_RUNTIME_ENABLED)) == 0U) {
		goto unlock;
	}

	/* wait until possible async suspend is completed */
	if (!k_is_pre_kernel()) {
		while (pm->state == PM_DEVICE_STATE_SUSPENDING) {
			(void)k_condvar_wait(&pm->condvar, &pm->lock,
					     K_FOREVER);
		}
	}

	/* wake up the device if suspended */
	if (pm->state == PM_DEVICE_STATE_SUSPENDED) {
		ret = pm->action_cb(pm->dev, PM_DEVICE_ACTION_RESUME);
		if (ret < 0) {
			goto unlock;
		}

		pm->state = PM_DEVICE_STATE_ACTIVE;
	}

	atomic_clear_bit(&pm->flags, PM_DEVICE_FLAG_RUNTIME_ENABLED);

unlock:
	if (!k_is_pre_kernel()) {
		k_mutex_unlock(&pm->lock);
	}

	SYS_PORT_TRACING_FUNC_EXIT(pm, device_runtime_disable, dev, ret);

	return ret;
}

bool pm_device_runtime_is_enabled(const struct device *dev)
{
	struct pm_device *pm = dev->pm;

	return atomic_test_bit(&pm->flags, PM_DEVICE_FLAG_RUNTIME_ENABLED);
}
