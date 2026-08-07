/*
 * Copyright (c) 2018 Intel Corporation.
 * Copyright (c) 2021 Nordic Semiconductor ASA.
 * Copyright (c) 2025 HubbleNetwork.
 * Copyright (c) 2025-2026 NXP.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/pm/device_runtime_internal.h>
#include <zephyr/sys/__assert.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(pm_device, CONFIG_PM_DEVICE_LOG_LEVEL);

#ifdef CONFIG_PM_DEVICE_POWER_DOMAIN
#define PM_DOMAIN(_pm) \
	(_pm)->domain
#else
#define PM_DOMAIN(_pm) NULL
#endif

/*
 * Serializes all pm->base.usage updates for devices without the ISR-safe
 * flag. The get/put fast paths read and update the usage counter under this
 * lock, possibly from interrupt context, so the slow paths must not modify
 * it under the per-device semaphore alone.
 */
static struct k_spinlock lock;
#ifdef CONFIG_PM_DEVICE_RUNTIME_ASYNC
#ifdef CONFIG_PM_DEVICE_RUNTIME_USE_DEDICATED_WQ
K_THREAD_STACK_DEFINE(pm_device_runtime_stack, CONFIG_PM_DEVICE_RUNTIME_DEDICATED_WQ_STACK_SIZE);
static struct k_work_q pm_device_runtime_wq;
#endif /* CONFIG_PM_DEVICE_RUNTIME_USE_DEDICATED_WQ */
#endif /* CONFIG_PM_DEVICE_RUNTIME_ASYNC */

#define EVENT_STATE_ACTIVE	BIT(PM_DEVICE_STATE_ACTIVE)
#define EVENT_STATE_SUSPENDED	BIT(PM_DEVICE_STATE_SUSPENDED)
#define EVENT_STATE_OFF         BIT(PM_DEVICE_STATE_OFF)

#define EVENT_MASK (EVENT_STATE_ACTIVE | EVENT_STATE_SUSPENDED | EVENT_STATE_OFF)

#ifdef CONFIG_TEST_PM_DEVICE_RUNTIME_HOOKS
#define RUNTIME_TEST_HOOK(dev, hook) z_pm_device_runtime_test_hook(dev, hook)
#else
#define RUNTIME_TEST_HOOK(dev, hook)                                                               \
	do {                                                                                       \
	} while (false)
#endif /* CONFIG_TEST_PM_DEVICE_RUNTIME_HOOKS */

/* Increment the usage counter of a device under the global lock. */
static void runtime_usecount_inc(struct pm_device *pm)
{
	__ASSERT_NO_MSG(pm != NULL);

	K_SPINLOCK(&lock) {
		pm->base.usage++;
	}
}

/* Decrement the usage counter of a device under the global lock. */
static void runtime_usecount_dec(struct pm_device *pm)
{
	__ASSERT_NO_MSG(pm != NULL);

	K_SPINLOCK(&lock) {
		pm->base.usage--;
	}
}

/*
 * Release one usage reference under the global lock, unless this is the
 * last reference. Last-user handling needs the per-device semaphore and
 * is left to the caller.
 *
 * @retval true If the reference was released.
 * @retval false If this is the last reference.
 */
static bool runtime_usage_put_fast(struct pm_device *pm)
{
	bool released = false;

	__ASSERT_NO_MSG(pm != NULL);

	K_SPINLOCK(&lock) {
		if (pm->base.usage > 1U) {
			pm->base.usage--;
			released = true;
		}
	}

	return released;
}

/*
 * Release one usage reference under the global lock, with the per-device
 * semaphore held by the caller.
 *
 * @retval 1 If this was the last reference and the device has to be
 * suspended.
 * @retval 0 If other references remain and nothing else has to be done.
 * @retval -EALREADY If the usage counter was already zero.
 */
static int runtime_usage_put(struct pm_device *pm)
{
	int ret = 0;

	__ASSERT_NO_MSG(pm != NULL);
	/* Caller must hold the per-device semaphore, except in pre-kernel
	 * mode where it is never taken (single-threaded boot).
	 */
	__ASSERT(k_is_pre_kernel() || (k_sem_count_get(&pm->lock) == 0),
		 "pm lock not owned");

	K_SPINLOCK(&lock) {
		if (pm->base.usage == 0U) {
			ret = -EALREADY;
		} else {
			pm->base.usage--;
			ret = (pm->base.usage == 0U) ? 1 : 0;
		}
	}

	return ret;
}

/**
 * @brief Suspend a device
 *
 * @note Asynchronous operations are not supported when in pre-kernel mode. In
 * this case, the async flag will be always forced to be false, and so the
 * function will be blocking.
 *
 * @pre_kernel_ok
 *
 * @param dev Device instance.
 * @param async Perform operation asynchronously.
 * @param delay Period to delay the asynchronous operation.
 *
 * @retval 0 If device has been suspended or queued for suspend.
 * @retval -EALREADY If device is already suspended (can only happen if get/put
 * calls are unbalanced).
 * @retval -EBUSY If the device is busy.
 * @retval -errno Other negative errno, result of the action callback.
 */
static int runtime_suspend(const struct device *dev, bool async,
			k_timeout_t delay)
{
	int ret = 0;
	struct pm_device *pm = dev->pm;

	/*
	 * Early return if device runtime is not enabled.
	 */
	if (!atomic_test_bit(&pm->base.flags, PM_DEVICE_FLAG_RUNTIME_ENABLED)) {
		return 0;
	}

	/* If we are not the last user, return. */
	if (runtime_usage_put_fast(pm)) {
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

	ret = runtime_usage_put(pm);
	if (ret <= 0) {
		if (ret < 0) {
			LOG_WRN("Unbalanced suspend: %s", dev->name);
		}
		goto unlock;
	}
	ret = 0;

	if (async) {
		/* queue suspend */
#ifdef CONFIG_PM_DEVICE_RUNTIME_ASYNC
		pm->base.state = PM_DEVICE_STATE_SUSPENDING;
#ifdef CONFIG_PM_DEVICE_RUNTIME_USE_SYSTEM_WQ
		(void)k_work_schedule(&pm->work, delay);
#else
		(void)k_work_schedule_for_queue(&pm_device_runtime_wq, &pm->work, delay);
#endif /* CONFIG_PM_DEVICE_RUNTIME_USE_SYSTEM_WQ */
#endif /* CONFIG_PM_DEVICE_RUNTIME_ASYNC */
	} else {
		/* suspend now */
		ret = pm->base.action_cb(pm->dev, PM_DEVICE_ACTION_SUSPEND);
		if (ret < 0) {
			runtime_usecount_inc(pm);
			goto unlock;
		}

		pm->base.state = PM_DEVICE_STATE_SUSPENDED;

		/* Now put the domain */
		if (atomic_test_bit(&dev->pm_base->flags, PM_DEVICE_FLAG_PD_CLAIMED)) {
			ret = pm_device_runtime_put(PM_DOMAIN(dev->pm_base));
			if (ret == 0) {
				atomic_clear_bit(&dev->pm_base->flags, PM_DEVICE_FLAG_PD_CLAIMED);
			} else {
				atomic_set_bit(&dev->pm_base->flags, PM_DEVICE_FLAG_PD_CLAIMED);
			}
		}
	}

unlock:
	if (!k_is_pre_kernel()) {
		k_sem_give(&pm->lock);
	}

	return ret;
}

#ifdef CONFIG_PM_DEVICE_RUNTIME_ASYNC
static void runtime_suspend_work(struct k_work *work)
{
	int domain_ret = 0;
	bool release_domain = false;
	int ret;
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct pm_device *pm = CONTAINER_OF(dwork, struct pm_device, work);
	const struct device *domain = PM_DOMAIN(&pm->base);

	ret = pm->base.action_cb(pm->dev, PM_DEVICE_ACTION_SUSPEND);

	(void)k_sem_take(&pm->lock, K_FOREVER);
	if (ret < 0) {
		runtime_usecount_inc(pm);
		pm->base.state = PM_DEVICE_STATE_ACTIVE;
	} else {
		pm->base.state = PM_DEVICE_STATE_SUSPENDED;
		if (atomic_test_bit(&pm->base.flags, PM_DEVICE_FLAG_PD_CLAIMED)) {
			atomic_set_bit(&pm->base.flags, PM_DEVICE_FLAG_PD_RELEASING);
			release_domain = true;
		}
	}
	k_sem_give(&pm->lock);

	if (release_domain) {
		domain_ret = pm_device_runtime_put(domain);
	}

	(void)k_sem_take(&pm->lock, K_FOREVER);
	if (release_domain) {
		if (domain_ret == 0) {
			atomic_clear_bit(&pm->base.flags, PM_DEVICE_FLAG_PD_CLAIMED);
		}
		atomic_clear_bit(&pm->base.flags, PM_DEVICE_FLAG_PD_RELEASING);
	}
	k_event_set(&pm->event, BIT(pm->base.state));
	k_sem_give(&pm->lock);

	if (ret < 0) {
		LOG_ERR("Could not suspend device %s (%d)", pm->dev->name, ret);
	}
	if (domain_ret < 0) {
		LOG_ERR("Could not release power domain for %s (%d)", pm->dev->name, domain_ret);
	}
}
#endif /* CONFIG_PM_DEVICE_RUNTIME_ASYNC */

static int get_sync_locked(const struct device *dev)
{
	int domain_ret;
	int ret;
	struct pm_device_isr *pm = dev->pm_isr;
	const struct device *domain = PM_DOMAIN(&pm->base);

	if ((pm->base.state == PM_DEVICE_STATE_OFF) &&
	    ((domain == NULL) ||
	     atomic_test_bit(&pm->base.flags, PM_DEVICE_FLAG_PD_CLAIMED))) {
		return -EAGAIN;
	}

	if (pm->base.usage == 0) {
		if (!atomic_test_bit(&pm->base.flags, PM_DEVICE_FLAG_PD_CLAIMED)) {
			if (domain != NULL) {
				if ((domain->pm_base->flags & BIT(PM_DEVICE_FLAG_ISR_SAFE)) != 0) {
					ret = pm_device_runtime_get(domain);
					if (ret < 0) {
						return ret;
					}
					/* Power domain successfully claimed */
					pm->base.flags |= BIT(PM_DEVICE_FLAG_PD_CLAIMED);
				} else {
					return -EWOULDBLOCK;
				}
			}
		}

		if ((pm->base.state == PM_DEVICE_STATE_OFF) ||
		    atomic_test_bit(&pm->base.flags,
				    PM_DEVICE_FLAG_TURN_ON_FAILED)) {
			if ((domain != NULL) &&
			    atomic_test_bit(&pm->base.flags, PM_DEVICE_FLAG_PD_CLAIMED)) {
				domain_ret = pm_device_runtime_put(domain);
				if (domain_ret == 0) {
					atomic_clear_bit(&pm->base.flags,
							 PM_DEVICE_FLAG_PD_CLAIMED);
				} else {
					atomic_set_bit(&pm->base.flags, PM_DEVICE_FLAG_PD_CLAIMED);
				}
			}
			return -EAGAIN;
		}

		ret = pm->base.action_cb(dev, PM_DEVICE_ACTION_RESUME);
		if (ret < 0) {
			if ((domain != NULL) &&
			    atomic_test_bit(&pm->base.flags, PM_DEVICE_FLAG_PD_CLAIMED)) {
				domain_ret = pm_device_runtime_put(domain);
				if (domain_ret == 0) {
					atomic_clear_bit(&pm->base.flags,
							 PM_DEVICE_FLAG_PD_CLAIMED);
				} else {
					atomic_set_bit(&pm->base.flags, PM_DEVICE_FLAG_PD_CLAIMED);
				}
			}
			return ret;
		}
		pm->base.state = PM_DEVICE_STATE_ACTIVE;
	} else {
		ret = 0;
	}

	pm->base.usage++;

	return ret;
}

int pm_device_runtime_get(const struct device *dev)
{
	bool waited_for_transition = false;
	const struct device *domain;
	int domain_ret;
	int ret = 0;
	struct pm_device *pm = dev->pm;

	if (pm == NULL) {
		return 0;
	}
	domain = PM_DOMAIN(&pm->base);

	SYS_PORT_TRACING_FUNC_ENTER(pm, device_runtime_get, dev);

	/*
	 * Early return if device runtime is not enabled.
	 */
	if (!atomic_test_bit(&pm->base.flags, PM_DEVICE_FLAG_RUNTIME_ENABLED)) {
		return 0;
	}

	RUNTIME_TEST_HOOK(dev, Z_PM_DEVICE_RUNTIME_HOOK_BEFORE_GET);

	if (atomic_test_bit(&dev->pm_base->flags, PM_DEVICE_FLAG_ISR_SAFE)) {
		struct pm_device_isr *pm_sync = dev->pm_isr;
		k_spinlock_key_t k = k_spin_lock(&pm_sync->lock);

		ret = get_sync_locked(dev);
		k_spin_unlock(&pm_sync->lock, k);
		goto end;
	}

	if (!k_is_pre_kernel()) {
		bool early_exit = false;

		K_SPINLOCK(&lock) {
			/* If we are not the first user and device is active, return. */
			if ((pm->base.usage > 0U) && (pm->base.state == PM_DEVICE_STATE_ACTIVE)) {
				pm->base.usage++;
				early_exit = true;
			}
		}

		if (early_exit == true) {
			ret = 0;
			goto end;
		}

		ret = k_sem_take(&pm->lock, k_is_in_isr() ? K_NO_WAIT : K_FOREVER);
		if (ret < 0) {
			return -EWOULDBLOCK;
		}
	}

#ifdef CONFIG_PM_DEVICE_RUNTIME_ASYNC
retry:
#endif /* CONFIG_PM_DEVICE_RUNTIME_ASYNC */
	if ((pm->base.state == PM_DEVICE_STATE_OFF) &&
	    (waited_for_transition || (domain == NULL) ||
	     atomic_test_bit(&pm->base.flags, PM_DEVICE_FLAG_PD_CLAIMED))) {
		ret = -EAGAIN;
		goto unlock;
	}

	if (k_is_in_isr() && ((pm->base.state == PM_DEVICE_STATE_SUSPENDING) ||
			      atomic_test_bit(&pm->base.flags, PM_DEVICE_FLAG_PD_RELEASING))) {
		ret = -EWOULDBLOCK;
		goto unlock;
	}

#ifdef CONFIG_PM_DEVICE_RUNTIME_ASYNC
	if ((pm->base.state == PM_DEVICE_STATE_SUSPENDING) &&
	    ((k_work_cancel_delayable(&pm->work) & K_WORK_RUNNING) == 0)) {
		pm->base.state = PM_DEVICE_STATE_ACTIVE;
	}

	if (!k_is_pre_kernel() && ((pm->base.state == PM_DEVICE_STATE_SUSPENDING) ||
				   atomic_test_bit(&pm->base.flags, PM_DEVICE_FLAG_PD_RELEASING))) {
		waited_for_transition = true;
		k_event_clear(&pm->event, EVENT_MASK);
		k_sem_give(&pm->lock);

		k_event_wait(&pm->event, EVENT_MASK, false, K_FOREVER);

		(void)k_sem_take(&pm->lock, K_FOREVER);
		goto retry;
	}
#endif /* CONFIG_PM_DEVICE_RUNTIME_ASYNC */

	/*
	 * If the device is under a power domain, the domain has to be get
	 * first.
	 */
	if (domain != NULL && !atomic_test_bit(&dev->pm_base->flags, PM_DEVICE_FLAG_PD_CLAIMED)) {
		ret = pm_device_runtime_get(domain);
		if (ret != 0) {
			goto unlock;
		}
		/* Power domain successfully claimed */
		atomic_set_bit(&pm->base.flags, PM_DEVICE_FLAG_PD_CLAIMED);
	}
	/* Check if powering up this device failed */
	if ((pm->base.state == PM_DEVICE_STATE_OFF) ||
	    atomic_test_bit(&pm->base.flags, PM_DEVICE_FLAG_TURN_ON_FAILED)) {
		if (domain != NULL) {
			domain_ret = pm_device_runtime_put(domain);
			if (domain_ret == 0) {
				atomic_clear_bit(&pm->base.flags, PM_DEVICE_FLAG_PD_CLAIMED);
			} else {
				atomic_set_bit(&pm->base.flags, PM_DEVICE_FLAG_PD_CLAIMED);
			}
		}
		ret = -EAGAIN;
		goto unlock;
	}

	runtime_usecount_inc(pm);
	if (pm->base.state == PM_DEVICE_STATE_ACTIVE) {
		goto unlock;
	}

	if (pm->base.usage > 1U) {
		goto unlock;
	}

	ret = pm->base.action_cb(pm->dev, PM_DEVICE_ACTION_RESUME);
	if (ret < 0) {
		runtime_usecount_dec(pm);
		if (domain != NULL) {
			domain_ret = pm_device_runtime_put(domain);
			if (domain_ret == 0) {
				atomic_clear_bit(&dev->pm_base->flags, PM_DEVICE_FLAG_PD_CLAIMED);
			} else {
				atomic_set_bit(&dev->pm_base->flags, PM_DEVICE_FLAG_PD_CLAIMED);
			}
		}
		goto unlock;
	}

	pm->base.state = PM_DEVICE_STATE_ACTIVE;

unlock:
	if (!k_is_pre_kernel()) {
		k_sem_give(&pm->lock);
	}

end:
	RUNTIME_TEST_HOOK(dev, Z_PM_DEVICE_RUNTIME_HOOK_AFTER_GET);
	SYS_PORT_TRACING_FUNC_EXIT(pm, device_runtime_get, dev, ret);

	return ret;
}

static int runtime_power_domain_action_sync(const struct device *dev, enum pm_device_action action)
{
	struct pm_device_isr *pm = dev->pm_isr;
	const struct device *domain = PM_DOMAIN(&pm->base);
	bool release_domain = false;
	k_spinlock_key_t key;
	int domain_ret = 0;
	int action_ret;
	int ret = 0;

	key = k_spin_lock(&pm->lock);
	if (action == PM_DEVICE_ACTION_TURN_ON) {
		if (pm->base.state == PM_DEVICE_STATE_OFF) {
			pm->base.usage = 0U;
		}
		ret = pm_device_action_run(dev, action);
		k_spin_unlock(&pm->lock, key);
		return ret;
	}

	pm->base.usage = 0U;
	if (pm->base.state == PM_DEVICE_STATE_ACTIVE) {
		action_ret = pm->base.action_cb(dev, PM_DEVICE_ACTION_SUSPEND);
		if (action_ret < 0) {
			ret = action_ret;
		}
		pm->base.state = PM_DEVICE_STATE_SUSPENDED;
	}

	if (atomic_test_bit(&pm->base.flags, PM_DEVICE_FLAG_PD_CLAIMED)) {
		atomic_set_bit(&pm->base.flags, PM_DEVICE_FLAG_PD_RELEASING);
		release_domain = true;
	}

	if (pm->base.state == PM_DEVICE_STATE_SUSPENDED) {
		action_ret = pm_device_action_run(dev, PM_DEVICE_ACTION_TURN_OFF);
		if ((action_ret < 0) && (ret == 0)) {
			ret = action_ret;
		}
	}
	atomic_clear_bit(&pm->base.flags, PM_DEVICE_FLAG_TURN_ON_FAILED);
	k_spin_unlock(&pm->lock, key);

	if (release_domain) {
		domain_ret = pm_device_runtime_put(domain);

		key = k_spin_lock(&pm->lock);
		if (domain_ret == 0) {
			atomic_clear_bit(&pm->base.flags, PM_DEVICE_FLAG_PD_CLAIMED);
		}
		atomic_clear_bit(&pm->base.flags, PM_DEVICE_FLAG_PD_RELEASING);
		k_spin_unlock(&pm->lock, key);
	}

	return (ret != 0) ? ret : domain_ret;
}

static int runtime_power_domain_action(const struct device *dev, enum pm_device_action action)
{
	struct pm_device *pm = dev->pm;
	const struct device *domain = PM_DOMAIN(&pm->base);
	bool release_domain = false;
	bool suspend = false;
	int domain_ret = 0;
	int action_ret;
	int ret = 0;

	(void)k_sem_take(&pm->lock, K_FOREVER);
	if ((action == PM_DEVICE_ACTION_TURN_ON) &&
	    (pm->base.state == PM_DEVICE_STATE_SUSPENDING)) {
		ret = -ENOTSUP;
		goto unlock;
	}

#ifdef CONFIG_PM_DEVICE_RUNTIME_ASYNC
retry:
	if ((pm->base.state == PM_DEVICE_STATE_SUSPENDING) &&
	    ((k_work_cancel_delayable(&pm->work) & K_WORK_RUNNING) == 0)) {
		pm->base.state = PM_DEVICE_STATE_ACTIVE;
	}

	if ((pm->base.state == PM_DEVICE_STATE_SUSPENDING) ||
	    atomic_test_bit(&pm->base.flags, PM_DEVICE_FLAG_PD_RELEASING)) {
		k_event_clear(&pm->event, EVENT_MASK);
		k_sem_give(&pm->lock);

		k_event_wait(&pm->event, EVENT_MASK, false, K_FOREVER);

		(void)k_sem_take(&pm->lock, K_FOREVER);
		goto retry;
	}
#endif /* CONFIG_PM_DEVICE_RUNTIME_ASYNC */

	if (action == PM_DEVICE_ACTION_TURN_ON) {
		if (pm->base.state == PM_DEVICE_STATE_OFF) {
			K_SPINLOCK(&lock) {
				pm->base.usage = 0U;
			}
		}
		ret = pm_device_action_run(dev, action);
		goto unlock;
	}

	K_SPINLOCK(&lock) {
		pm->base.usage = 0U;
		if (pm->base.state == PM_DEVICE_STATE_ACTIVE) {
			pm->base.state = PM_DEVICE_STATE_SUSPENDING;
			suspend = true;
		}
	}

	if (suspend) {
		action_ret = pm->base.action_cb(dev, PM_DEVICE_ACTION_SUSPEND);
		if (action_ret < 0) {
			ret = action_ret;
		}
		pm->base.state = PM_DEVICE_STATE_SUSPENDED;
	}

	if (atomic_test_bit(&pm->base.flags, PM_DEVICE_FLAG_PD_CLAIMED)) {
		atomic_set_bit(&pm->base.flags, PM_DEVICE_FLAG_PD_RELEASING);
		release_domain = true;
	}

	if (pm->base.state == PM_DEVICE_STATE_SUSPENDED) {
		action_ret = pm_device_action_run(dev, PM_DEVICE_ACTION_TURN_OFF);
		if ((action_ret < 0) && (ret == 0)) {
			ret = action_ret;
		}
	}
	atomic_clear_bit(&pm->base.flags, PM_DEVICE_FLAG_TURN_ON_FAILED);

unlock:
	k_sem_give(&pm->lock);

	if (release_domain) {
		domain_ret = pm_device_runtime_put(domain);

		(void)k_sem_take(&pm->lock, K_FOREVER);
		if (domain_ret == 0) {
			atomic_clear_bit(&pm->base.flags, PM_DEVICE_FLAG_PD_CLAIMED);
		}
		atomic_clear_bit(&pm->base.flags, PM_DEVICE_FLAG_PD_RELEASING);
#ifdef CONFIG_PM_DEVICE_RUNTIME_ASYNC
		k_event_set(&pm->event, BIT(pm->base.state));
#endif /* CONFIG_PM_DEVICE_RUNTIME_ASYNC */
		k_sem_give(&pm->lock);
	}

	return (ret != 0) ? ret : domain_ret;
}

int z_pm_device_runtime_power_domain_action_run(const struct device *dev,
						enum pm_device_action action)
{
	int ret;

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	if ((action != PM_DEVICE_ACTION_TURN_ON) && (action != PM_DEVICE_ACTION_TURN_OFF)) {
		return -EINVAL;
	}

	if (dev->pm_base == NULL) {
		return -ENOTSUP;
	}

	RUNTIME_TEST_HOOK(dev, Z_PM_DEVICE_RUNTIME_HOOK_BEFORE_PD_ACTION);
	if (atomic_test_bit(&dev->pm_base->flags, PM_DEVICE_FLAG_ISR_SAFE)) {
		ret = runtime_power_domain_action_sync(dev, action);
	} else {
		ret = runtime_power_domain_action(dev, action);
	}
	RUNTIME_TEST_HOOK(dev, Z_PM_DEVICE_RUNTIME_HOOK_AFTER_PD_ACTION);

	return ret;
}

static int put_sync_locked(const struct device *dev)
{
	int ret;
	struct pm_device_isr *pm = dev->pm_isr;
	uint32_t flags = pm->base.flags;

	if (!(flags & BIT(PM_DEVICE_FLAG_RUNTIME_ENABLED))) {
		return 0;
	}

	if (pm->base.usage == 0U) {
		return -EALREADY;
	}

	pm->base.usage--;
	if (pm->base.usage == 0U) {
		ret = pm->base.action_cb(dev, PM_DEVICE_ACTION_SUSPEND);
		if (ret < 0) {
			pm->base.usage++;
			return ret;
		}
		pm->base.state = PM_DEVICE_STATE_SUSPENDED;

		if (flags & BIT(PM_DEVICE_FLAG_PD_CLAIMED)) {
			const struct device *domain = PM_DOMAIN(&pm->base);

			if (domain->pm_base->flags & BIT(PM_DEVICE_FLAG_ISR_SAFE)) {
				ret = put_sync_locked(domain);
				if (ret == 0) {
					pm->base.flags &= ~BIT(PM_DEVICE_FLAG_PD_CLAIMED);
				} else {
					pm->base.flags |= BIT(PM_DEVICE_FLAG_PD_CLAIMED);
				}
			} else {
				ret = -EWOULDBLOCK;
			}
		}
	} else {
		ret = 0;
	}

	return ret;
}

int pm_device_runtime_put(const struct device *dev)
{
	int ret;

	if (dev->pm_base == NULL) {
		return 0;
	}

	SYS_PORT_TRACING_FUNC_ENTER(pm, device_runtime_put, dev);

	RUNTIME_TEST_HOOK(dev, Z_PM_DEVICE_RUNTIME_HOOK_BEFORE_PUT);
	if (atomic_test_bit(&dev->pm_base->flags, PM_DEVICE_FLAG_ISR_SAFE)) {
		struct pm_device_isr *pm_sync = dev->pm_isr;
		k_spinlock_key_t k = k_spin_lock(&pm_sync->lock);

		ret = put_sync_locked(dev);

		k_spin_unlock(&pm_sync->lock, k);
	} else {
		ret = runtime_suspend(dev, false, K_NO_WAIT);
	}
	RUNTIME_TEST_HOOK(dev, Z_PM_DEVICE_RUNTIME_HOOK_AFTER_PUT);
	SYS_PORT_TRACING_FUNC_EXIT(pm, device_runtime_put, dev, ret);

	return ret;
}

int pm_device_runtime_put_async(const struct device *dev, k_timeout_t delay)
{
#ifdef CONFIG_PM_DEVICE_RUNTIME_ASYNC
	int ret;

	if (dev->pm_base == NULL) {
		return 0;
	}

	SYS_PORT_TRACING_FUNC_ENTER(pm, device_runtime_put_async, dev, delay);
	RUNTIME_TEST_HOOK(dev, Z_PM_DEVICE_RUNTIME_HOOK_BEFORE_PUT);
	if (atomic_test_bit(&dev->pm_base->flags, PM_DEVICE_FLAG_ISR_SAFE)) {
		struct pm_device_isr *pm_sync = dev->pm_isr;
		k_spinlock_key_t k = k_spin_lock(&pm_sync->lock);

		ret = put_sync_locked(dev);

		k_spin_unlock(&pm_sync->lock, k);
	} else {
		ret = runtime_suspend(dev, true, delay);
	}
	RUNTIME_TEST_HOOK(dev, Z_PM_DEVICE_RUNTIME_HOOK_AFTER_PUT);
	SYS_PORT_TRACING_FUNC_EXIT(pm, device_runtime_put_async, dev, delay, ret);

	return ret;
#else
	return -ENOSYS;
#endif /* CONFIG_PM_DEVICE_RUNTIME_ASYNC */
}

__boot_func
int pm_device_runtime_auto_enable(const struct device *dev)
{
	struct pm_device_base *pm = dev->pm_base;

	if (!pm) {
		return 0;
	}

	if (!IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME_DEFAULT_ENABLE) &&
	    !atomic_test_bit(&pm->flags, PM_DEVICE_FLAG_RUNTIME_AUTO)) {
		return 0;
	}

	return pm_device_runtime_enable(dev);
}

static int runtime_enable_sync(const struct device *dev)
{
	int ret;
	struct pm_device_isr *pm = dev->pm_isr;
	k_spinlock_key_t k = k_spin_lock(&pm->lock);

	if (pm->base.state == PM_DEVICE_STATE_ACTIVE) {
		ret = pm->base.action_cb(dev, PM_DEVICE_ACTION_SUSPEND);
		if (ret < 0) {
			goto unlock;
		}

		pm->base.state = PM_DEVICE_STATE_SUSPENDED;
	} else {
		ret = 0;
	}

	pm->base.flags |= BIT(PM_DEVICE_FLAG_RUNTIME_ENABLED);
	pm->base.usage = 0U;
unlock:
	k_spin_unlock(&pm->lock, k);
	return ret;
}

int pm_device_runtime_enable(const struct device *dev)
{
	int ret = 0;
	struct pm_device *pm = dev->pm;

	SYS_PORT_TRACING_FUNC_ENTER(pm, device_runtime_enable, dev);

	if (pm == NULL) {
		ret = -ENOTSUP;
		goto end;
	}

	if (atomic_test_bit(&pm->base.flags, PM_DEVICE_FLAG_RUNTIME_ENABLED)) {
		goto end;
	}

	if (pm_device_is_busy(dev)) {
		ret = -EBUSY;
		goto end;
	}

	if (atomic_test_bit(&dev->pm_base->flags, PM_DEVICE_FLAG_ISR_SAFE)) {
		ret = runtime_enable_sync(dev);
		goto end;
	}

	if (!k_is_pre_kernel()) {
		(void)k_sem_take(&pm->lock, K_FOREVER);
	}

	/* lazy init of PM fields */
	if (pm->dev == NULL) {
		pm->dev = dev;
#ifdef CONFIG_PM_DEVICE_RUNTIME_ASYNC
		k_work_init_delayable(&pm->work, runtime_suspend_work);
#endif /* CONFIG_PM_DEVICE_RUNTIME_ASYNC */
	}

	if (pm->base.state == PM_DEVICE_STATE_ACTIVE) {
		ret = pm->base.action_cb(pm->dev, PM_DEVICE_ACTION_SUSPEND);
		if (ret < 0) {
			goto unlock;
		}
		pm->base.state = PM_DEVICE_STATE_SUSPENDED;
	}

	pm->base.usage = 0U;

	atomic_set_bit(&pm->base.flags, PM_DEVICE_FLAG_RUNTIME_ENABLED);

unlock:
	if (!k_is_pre_kernel()) {
		k_sem_give(&pm->lock);
	}

end:
	SYS_PORT_TRACING_FUNC_EXIT(pm, device_runtime_enable, dev, ret);
	return ret;
}

static int runtime_disable_sync(const struct device *dev)
{
	struct pm_device_isr *pm = dev->pm_isr;
	int ret;
	k_spinlock_key_t k = k_spin_lock(&pm->lock);

	if (pm->base.state == PM_DEVICE_STATE_SUSPENDED) {
		ret = pm->base.action_cb(dev, PM_DEVICE_ACTION_RESUME);
		if (ret < 0) {
			goto unlock;
		}

		pm->base.state = PM_DEVICE_STATE_ACTIVE;
	} else {
		ret = 0;
	}

	pm->base.flags &= ~BIT(PM_DEVICE_FLAG_RUNTIME_ENABLED);
unlock:
	k_spin_unlock(&pm->lock, k);
	return ret;
}

int pm_device_runtime_disable(const struct device *dev)
{
	int ret = 0;
	struct pm_device *pm = dev->pm;

	SYS_PORT_TRACING_FUNC_ENTER(pm, device_runtime_disable, dev);

	if (pm == NULL) {
		ret = -ENOTSUP;
		goto end;
	}

	if (!atomic_test_bit(&pm->base.flags, PM_DEVICE_FLAG_RUNTIME_ENABLED)) {
		goto end;
	}

	if (atomic_test_bit(&dev->pm_base->flags, PM_DEVICE_FLAG_ISR_SAFE)) {
		ret = runtime_disable_sync(dev);
		goto end;
	}

	if (!k_is_pre_kernel()) {
		(void)k_sem_take(&pm->lock, K_FOREVER);
	}

#ifdef CONFIG_PM_DEVICE_RUNTIME_ASYNC
	if (!k_is_pre_kernel()) {
		if ((pm->base.state == PM_DEVICE_STATE_SUSPENDING) &&
			((k_work_cancel_delayable(&pm->work) & K_WORK_RUNNING) == 0)) {
			pm->base.state = PM_DEVICE_STATE_ACTIVE;
			goto clear_bit;
		}

		/* wait until possible async suspend and domain release are completed */
		while ((pm->base.state == PM_DEVICE_STATE_SUSPENDING) ||
		       atomic_test_bit(&pm->base.flags, PM_DEVICE_FLAG_PD_RELEASING)) {
			k_event_clear(&pm->event, EVENT_MASK);
			k_sem_give(&pm->lock);

			k_event_wait(&pm->event, EVENT_MASK, false, K_FOREVER);

			(void)k_sem_take(&pm->lock, K_FOREVER);
		}
	}
#endif /* CONFIG_PM_DEVICE_RUNTIME_ASYNC */

	/* wake up the device if suspended */
	if (pm->base.state == PM_DEVICE_STATE_SUSPENDED) {
		ret = pm->base.action_cb(dev, PM_DEVICE_ACTION_RESUME);
		if (ret < 0) {
			goto unlock;
		}

		pm->base.state = PM_DEVICE_STATE_ACTIVE;
	}
#ifdef CONFIG_PM_DEVICE_RUNTIME_ASYNC
clear_bit:
#endif
	atomic_clear_bit(&pm->base.flags, PM_DEVICE_FLAG_RUNTIME_ENABLED);

unlock:
	if (!k_is_pre_kernel()) {
		k_sem_give(&pm->lock);
	}

end:
	SYS_PORT_TRACING_FUNC_EXIT(pm, device_runtime_disable, dev, ret);

	return ret;
}

bool pm_device_runtime_is_enabled(const struct device *dev)
{
	struct pm_device_base *pm = dev->pm_base;

	return pm && atomic_test_bit(&pm->flags, PM_DEVICE_FLAG_RUNTIME_ENABLED);
}

int pm_device_runtime_usage(const struct device *dev)
{
	if (!pm_device_runtime_is_enabled(dev)) {
		return -ENOTSUP;
	}

	return dev->pm_base->usage;
}

#ifdef CONFIG_PM_DEVICE_RUNTIME_ASYNC
#ifdef CONFIG_PM_DEVICE_RUNTIME_USE_DEDICATED_WQ

static int pm_device_runtime_wq_init(void)
{
	const struct k_work_queue_config cfg = {.name = "PM DEVICE RUNTIME WQ"};

	k_work_queue_init(&pm_device_runtime_wq);

	k_work_queue_start(&pm_device_runtime_wq, pm_device_runtime_stack,
			   K_THREAD_STACK_SIZEOF(pm_device_runtime_stack),
			   CONFIG_PM_DEVICE_RUNTIME_DEDICATED_WQ_PRIO, &cfg);

	return 0;
}

SYS_INIT(pm_device_runtime_wq_init, POST_KERNEL,
	CONFIG_PM_DEVICE_RUNTIME_DEDICATED_WQ_INIT_PRIO);

#endif /* CONFIG_PM_DEVICE_RUNTIME_USE_DEDICATED_WQ */
#endif /* CONFIG_PM_DEVICE_RUNTIME_ASYNC */
