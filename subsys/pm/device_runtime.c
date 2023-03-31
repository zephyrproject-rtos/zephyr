/*
 * Copyright (c) 2018 Intel Corporation.
 * Copyright (c) 2021 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/sys/__assert.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(pm_device, CONFIG_PM_DEVICE_LOG_LEVEL);

#ifdef CONFIG_PM_DEVICE_POWER_DOMAIN
#define PM_DOMAIN(_pm) \
	(_pm)->domain
#else
#define PM_DOMAIN(_pm) NULL
#endif

#define EVENT_STATE_ACTIVE	BIT(PM_DEVICE_STATE_ACTIVE)
#define EVENT_STATE_SUSPENDED	BIT(PM_DEVICE_STATE_SUSPENDED)

#define EVENT_MASK		(EVENT_STATE_ACTIVE | EVENT_STATE_SUSPENDED)

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
 * @retval -EALREADY If device is already suspended (can only happen if get/put
 * calls are unbalanced).
 * @retval -EBUSY If the device is busy.
 * @retval -errno Other negative errno, result of the action callback.
 */
static int runtime_suspend(const struct device *dev, bool async)
{
	int ret = 0;
	struct pm_device *pm = dev->pm;

	/*
	 * Early return if device runtime is not enabled.
	 */
	if (!atomic_test_bit(&pm->flags, PM_DEVICE_FLAG_RUNTIME_ENABLED)) {
		return 0;
	}

	if (k_is_pre_kernel()) {
		async = false;
	} else {
		ret = k_sem_take(&pm->lock, k_is_in_isr() ? K_NO_WAIT : K_FOREVER);
		if (ret < 0) {
			return -EBUSY;
		}
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
		k_sem_give(&pm->lock);
	}

	return ret;
}

static void runtime_suspend_work(struct k_work *work)
{
	int ret;
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct pm_device *pm = CONTAINER_OF(dwork, struct pm_device, work);

	ret = pm->action_cb(pm->dev, PM_DEVICE_ACTION_SUSPEND);

	(void)k_sem_take(&pm->lock, K_FOREVER);
	if (ret < 0) {
		pm->usage++;
		pm->state = PM_DEVICE_STATE_ACTIVE;
	} else {
		pm->state = PM_DEVICE_STATE_SUSPENDED;
	}
	k_event_set(&pm->event, BIT(pm->state));
	k_sem_give(&pm->lock);

	/*
	 * On async put, we have to suspend the domain when the device
	 * finishes its operation
	 */
	if (PM_DOMAIN(pm) != NULL) {
		(void)pm_device_runtime_put(PM_DOMAIN(pm));
	}

	__ASSERT(ret == 0, "Could not suspend device (%d)", ret);
}

int pm_device_runtime_get(const struct device *dev)
{
	int ret = 0;
	struct pm_device *pm = dev->pm;

	if (pm == NULL) {
		return 0;
	}

	SYS_PORT_TRACING_FUNC_ENTER(pm, device_runtime_get, dev);

	/*
	 * Early return if device runtime is not enabled.
	 */
	if (!atomic_test_bit(&pm->flags, PM_DEVICE_FLAG_RUNTIME_ENABLED)) {
		return 0;
	}

	if (!k_is_pre_kernel()) {
		(void)k_sem_take(&pm->lock, K_FOREVER);
	}

	/*
	 * If the device is under a power domain, the domain has to be get
	 * first.
	 */
	if (PM_DOMAIN(pm) != NULL) {
		ret = pm_device_runtime_get(PM_DOMAIN(pm));
		if (ret != 0) {
			goto unlock;
		}
		/* Check if powering up this device failed */
		if (atomic_test_bit(&pm->flags, PM_DEVICE_FLAG_TURN_ON_FAILED)) {
			(void)pm_device_runtime_put(PM_DOMAIN(pm));
			ret = -EAGAIN;
			goto unlock;
		}
		/* Power domain successfully claimed */
		atomic_set_bit(&pm->flags, PM_DEVICE_FLAG_PD_CLAIMED);
	}

	pm->usage++;

	if (!k_is_pre_kernel()) {
		/* wait until possible async suspend is completed */
		while (pm->state == PM_DEVICE_STATE_SUSPENDING) {
			k_sem_give(&pm->lock);

			k_event_wait(&pm->event, EVENT_MASK, true, K_FOREVER);

			(void)k_sem_take(&pm->lock, K_FOREVER);
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
		k_sem_give(&pm->lock);
	}

	SYS_PORT_TRACING_FUNC_EXIT(pm, device_runtime_get, dev, ret);

	return ret;
}

int pm_device_runtime_put(const struct device *dev)
{
	int ret;

	__ASSERT(!k_is_in_isr(), "use pm_device_runtime_put_async() in ISR");

	if (dev->pm == NULL) {
		return 0;
	}

	SYS_PORT_TRACING_FUNC_ENTER(pm, device_runtime_put, dev);
	ret = runtime_suspend(dev, false);

	/*
	 * Now put the domain
	 */
	if ((ret == 0) &&
	    atomic_test_and_clear_bit(&dev->pm->flags, PM_DEVICE_FLAG_PD_CLAIMED)) {
		ret = pm_device_runtime_put(PM_DOMAIN(dev->pm));
	}
	SYS_PORT_TRACING_FUNC_EXIT(pm, device_runtime_put, dev, ret);

	return ret;
}

int pm_device_runtime_put_async(const struct device *dev)
{
	int ret;

	if (dev->pm == NULL) {
		return 0;
	}

	SYS_PORT_TRACING_FUNC_ENTER(pm, device_runtime_put_async, dev);
	ret = runtime_suspend(dev, true);
	SYS_PORT_TRACING_FUNC_EXIT(pm, device_runtime_put_async, dev, ret);

	return ret;
}

__boot_func
int pm_device_runtime_auto_enable(const struct device *dev)
{
	struct pm_device *pm = dev->pm;

	/* No action needed if PM_DEVICE_FLAG_RUNTIME_AUTO is not enabled */
	if (!pm || !atomic_test_bit(&pm->flags, PM_DEVICE_FLAG_RUNTIME_AUTO)) {
		return 0;
	}
	return pm_device_runtime_enable(dev);
}

int pm_device_runtime_enable(const struct device *dev)
{
	int ret = 0;
	struct pm_device *pm = dev->pm;

	if (pm == NULL) {
		return -ENOTSUP;
	}

	SYS_PORT_TRACING_FUNC_ENTER(pm, device_runtime_enable, dev);

	if (pm_device_state_is_locked(dev)) {
		ret = -EPERM;
		goto end;
	}

	if (!k_is_pre_kernel()) {
		(void)k_sem_take(&pm->lock, K_FOREVER);
	}

	if (atomic_test_bit(&pm->flags, PM_DEVICE_FLAG_RUNTIME_ENABLED)) {
		goto unlock;
	}

	/* lazy init of PM fields */
	if (pm->dev == NULL) {
		pm->dev = dev;
		k_work_init_delayable(&pm->work, runtime_suspend_work);
	}

	if (pm->state == PM_DEVICE_STATE_ACTIVE) {
		ret = pm->action_cb(pm->dev, PM_DEVICE_ACTION_SUSPEND);
		if (ret < 0) {
			goto unlock;
		}
		pm->state = PM_DEVICE_STATE_SUSPENDED;
	}

	pm->usage = 0U;

	atomic_set_bit(&pm->flags, PM_DEVICE_FLAG_RUNTIME_ENABLED);

unlock:
	if (!k_is_pre_kernel()) {
		k_sem_give(&pm->lock);
	}

end:
	SYS_PORT_TRACING_FUNC_EXIT(pm, device_runtime_enable, dev, ret);
	return ret;
}

int pm_device_runtime_disable(const struct device *dev)
{
	int ret = 0;
	struct pm_device *pm = dev->pm;

	if (pm == NULL) {
		return -ENOTSUP;
	}

	SYS_PORT_TRACING_FUNC_ENTER(pm, device_runtime_disable, dev);

	if (!k_is_pre_kernel()) {
		(void)k_sem_take(&pm->lock, K_FOREVER);
	}

	if (!atomic_test_bit(&pm->flags, PM_DEVICE_FLAG_RUNTIME_ENABLED)) {
		goto unlock;
	}

	/* wait until possible async suspend is completed */
	if (!k_is_pre_kernel()) {
		while (pm->state == PM_DEVICE_STATE_SUSPENDING) {
			k_sem_give(&pm->lock);

			k_event_wait(&pm->event, EVENT_MASK, true, K_FOREVER);

			(void)k_sem_take(&pm->lock, K_FOREVER);
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
		k_sem_give(&pm->lock);
	}

	SYS_PORT_TRACING_FUNC_EXIT(pm, device_runtime_disable, dev, ret);

	return ret;
}

bool pm_device_runtime_is_enabled(const struct device *dev)
{
	struct pm_device *pm = dev->pm;

	return pm && atomic_test_bit(&pm->flags, PM_DEVICE_FLAG_RUNTIME_ENABLED);
}
