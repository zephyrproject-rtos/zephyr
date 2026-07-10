/*
 * Copyright (c) 2018 Intel Corporation.
 * Copyright (c) 2021 Nordic Semiconductor ASA.
 * Copyright (c) 2025 HubbleNetwork.
 * Copyright (c) 2025 NXP.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/sys/__assert.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(pm_device, CONFIG_PM_DEVICE_LOG_LEVEL);

#ifdef CONFIG_PM_DEVICE_POWER_DOMAIN
static bool has_power_domains(const struct pm_device_base *pm)
{
	return pm->domains[0] != NULL;
}

/* Release the domains preceding @p end, used to unwind a partial claim. */
static void domains_release(struct pm_device_base *pm, const struct device * const *end)
{
	for (const struct device * const *d = pm->domains; d < end; d++) {
		(void)pm_device_runtime_put(*d);
	}
}

/* Claim every power domain the device depends on. On failure the domains
 * already claimed are released again so the operation is all-or-nothing.
 */
static int domains_get(const struct device *dev)
{
	struct pm_device_base *pm = dev->pm_base;

	PM_DEVICE_FOREACH_DOMAIN(pm, d) {
		int ret = pm_device_runtime_get(*d);

		/* The domain may have failed to power this device up */
		if ((ret == 0) &&
		    atomic_test_bit(&pm->flags, PM_DEVICE_FLAG_TURN_ON_FAILED)) {
			(void)pm_device_runtime_put(*d);
			ret = -EAGAIN;
		}

		if (ret < 0) {
			domains_release(pm, d);
			return ret;
		}
	}

	return 0;
}

/* ISR safe variant: a device that runs from an ISR can only claim domains
 * that are themselves ISR safe.
 */
static int domains_get_isr(const struct device *dev)
{
	struct pm_device_base *pm = dev->pm_base;

	PM_DEVICE_FOREACH_DOMAIN(pm, d) {
		int ret;

		if (((*d)->pm_base->flags & BIT(PM_DEVICE_FLAG_ISR_SAFE)) == 0) {
			ret = -EWOULDBLOCK;
		} else {
			ret = pm_device_runtime_get(*d);
		}

		if (ret < 0) {
			domains_release(pm, d);
			return ret;
		}
	}

	return 0;
}

static void domains_put(const struct device *dev)
{
	struct pm_device_base *pm = dev->pm_base;

	PM_DEVICE_FOREACH_DOMAIN(pm, d) {
		(void)pm_device_runtime_put(*d);
	}
}
#else
static inline bool has_power_domains(const struct pm_device_base *pm)
{
	ARG_UNUSED(pm);
	return false;
}

static inline int domains_get(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}

static inline int domains_get_isr(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}

static inline void domains_put(const struct device *dev)
{
	ARG_UNUSED(dev);
}
#endif /* CONFIG_PM_DEVICE_POWER_DOMAIN */

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

#define EVENT_MASK		(EVENT_STATE_ACTIVE | EVENT_STATE_SUSPENDED)

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

		/* Now put the domains */
		if (atomic_test_bit(&dev->pm_base->flags, PM_DEVICE_FLAG_PD_CLAIMED)) {
			domains_put(dev);
			atomic_clear_bit(&dev->pm_base->flags, PM_DEVICE_FLAG_PD_CLAIMED);
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
	int ret;
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct pm_device *pm = CONTAINER_OF(dwork, struct pm_device, work);
	bool release_domain = false;

	ret = pm->base.action_cb(pm->dev, PM_DEVICE_ACTION_SUSPEND);

	(void)k_sem_take(&pm->lock, K_FOREVER);
	if (ret < 0) {
		runtime_usecount_inc(pm);
		pm->base.state = PM_DEVICE_STATE_ACTIVE;
	} else {
		pm->base.state = PM_DEVICE_STATE_SUSPENDED;
		release_domain = (pm->base.usage == 0U) &&
				 atomic_test_bit(&pm->base.flags, PM_DEVICE_FLAG_PD_CLAIMED);
	}
	k_event_set(&pm->event, BIT(pm->base.state));

	/*
	 * On async put, we have to suspend the domain when the device
	 * finishes its operation, unless a get arrived while the suspend was
	 * running and restored the device usage.
	 */
	if (release_domain) {
		domains_put(pm->dev);
		atomic_clear_bit(&pm->base.flags, PM_DEVICE_FLAG_PD_CLAIMED);
	}
	k_sem_give(&pm->lock);

	__ASSERT(ret == 0, "Could not suspend device (%d)", ret);
}
#endif /* CONFIG_PM_DEVICE_RUNTIME_ASYNC */

static int get_sync_locked(const struct device *dev)
{
	int ret;
	struct pm_device_isr *pm = dev->pm_isr;
	uint32_t flags = pm->base.flags;

	if (pm->base.usage == 0) {
		if (((flags & BIT(PM_DEVICE_FLAG_PD_CLAIMED)) == 0) &&
		    has_power_domains(&pm->base)) {
			ret = domains_get_isr(dev);
			if (ret < 0) {
				return ret;
			}
			/* Power domains successfully claimed */
			pm->base.flags |= BIT(PM_DEVICE_FLAG_PD_CLAIMED);
		}

		ret = pm->base.action_cb(dev, PM_DEVICE_ACTION_RESUME);
		if (ret < 0) {
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
	int ret = 0;
	struct pm_device *pm = dev->pm;

	if (pm == NULL) {
		return 0;
	}

	SYS_PORT_TRACING_FUNC_ENTER(pm, device_runtime_get, dev);

	/*
	 * Early return if device runtime is not enabled.
	 */
	if (!atomic_test_bit(&pm->base.flags, PM_DEVICE_FLAG_RUNTIME_ENABLED)) {
		return 0;
	}

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

	if (k_is_in_isr() && (pm->base.state == PM_DEVICE_STATE_SUSPENDING)) {
		ret = -EWOULDBLOCK;
		goto unlock;
	}

	/*
	 * If the device is under a power domain, the domain has to be get
	 * first.
	 */
	if (has_power_domains(&pm->base) &&
	    !atomic_test_bit(&dev->pm_base->flags, PM_DEVICE_FLAG_PD_CLAIMED)) {
		ret = domains_get(dev);
		if (ret != 0) {
			goto unlock;
		}
		/* Power domains successfully claimed */
		atomic_set_bit(&pm->base.flags, PM_DEVICE_FLAG_PD_CLAIMED);
	}

	runtime_usecount_inc(pm);

#ifdef CONFIG_PM_DEVICE_RUNTIME_ASYNC
	/*
	 * Check if the device has a pending suspend operation (not started
	 * yet) and cancel it. This way we avoid unnecessary operations because
	 * the device is actually active.
	 */
	if ((pm->base.state == PM_DEVICE_STATE_SUSPENDING) &&
		((k_work_cancel_delayable(&pm->work) & K_WORK_RUNNING) == 0)) {
		pm->base.state = PM_DEVICE_STATE_ACTIVE;
		goto unlock;
	}

	if (!k_is_pre_kernel()) {
		/*
		 * If the device is already suspending there is
		 * nothing else we can do but wait until it finishes.
		 */
		while (pm->base.state == PM_DEVICE_STATE_SUSPENDING) {
			k_event_clear(&pm->event, EVENT_MASK);
			k_sem_give(&pm->lock);

			k_event_wait(&pm->event, EVENT_MASK, false, K_FOREVER);

			(void)k_sem_take(&pm->lock, K_FOREVER);
		}
	}
#endif /* CONFIG_PM_DEVICE_RUNTIME_ASYNC */

	if (pm->base.usage > 1U) {
		goto unlock;
	}

	ret = pm->base.action_cb(pm->dev, PM_DEVICE_ACTION_RESUME);
	if (ret < 0) {
		runtime_usecount_dec(pm);
		if (atomic_test_bit(&dev->pm_base->flags, PM_DEVICE_FLAG_PD_CLAIMED)) {
			domains_put(dev);
			atomic_clear_bit(&dev->pm_base->flags, PM_DEVICE_FLAG_PD_CLAIMED);
		}
		goto unlock;
	}

	pm->base.state = PM_DEVICE_STATE_ACTIVE;

unlock:
	if (!k_is_pre_kernel()) {
		k_sem_give(&pm->lock);
	}

end:
	SYS_PORT_TRACING_FUNC_EXIT(pm, device_runtime_get, dev, ret);

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
#ifdef CONFIG_PM_DEVICE_POWER_DOMAIN
			int pd_ret = 0;

			PM_DEVICE_FOREACH_DOMAIN(&pm->base, d) {
				int r = put_sync_locked(*d);

				if ((r < 0) && (pd_ret == 0)) {
					pd_ret = r;
				}
			}

			/* Keep the domains claimed if any release failed. */
			if (pd_ret == 0) {
				pm->base.flags &= ~BIT(PM_DEVICE_FLAG_PD_CLAIMED);
			} else {
				ret = pd_ret;
			}
#else
			pm->base.flags &= ~BIT(PM_DEVICE_FLAG_PD_CLAIMED);
#endif /* CONFIG_PM_DEVICE_POWER_DOMAIN */
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

	if (atomic_test_bit(&dev->pm_base->flags, PM_DEVICE_FLAG_ISR_SAFE)) {
		struct pm_device_isr *pm_sync = dev->pm_isr;
		k_spinlock_key_t k = k_spin_lock(&pm_sync->lock);

		ret = put_sync_locked(dev);

		k_spin_unlock(&pm_sync->lock, k);
	} else {
		ret = runtime_suspend(dev, false, K_NO_WAIT);
	}
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
	if (atomic_test_bit(&dev->pm_base->flags, PM_DEVICE_FLAG_ISR_SAFE)) {
		struct pm_device_isr *pm_sync = dev->pm_isr;
		k_spinlock_key_t k = k_spin_lock(&pm_sync->lock);

		ret = put_sync_locked(dev);

		k_spin_unlock(&pm_sync->lock, k);
	} else {
		ret = runtime_suspend(dev, true, delay);
	}
	SYS_PORT_TRACING_FUNC_EXIT(pm, device_runtime_put_async, dev, delay, ret);

	return ret;
#else
	LOG_WRN("Function not available");
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

		/* wait until possible async suspend is completed */
		while (pm->base.state == PM_DEVICE_STATE_SUSPENDING) {
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
