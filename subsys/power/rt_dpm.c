/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <ksched.h>
#include <wait_q.h>
#include <spinlock.h>
#include <power/rt_dpm.h>

/* States defined for device runtime power management.
 */
enum rt_dpm_state {
	RT_DPM_ACTIVE,
	RT_DPM_RESUMING,
	RT_DPM_SUSPENDED,
	RT_DPM_SUSPENDING
};

/* Try to suspend the given device, if successfully make
 * the state transition, asynchronously release its parent
 * to avoid holding caller context long time.
 */
static int rt_dpm_release_helper(struct rt_dpm *rt_pm)
{
	int ret = 0;
	k_spinlock_key_t key;
	struct k_thread *thread;
	struct device *dev = CONTAINER_OF(rt_pm, struct device, rt_pm);

	key = k_spin_lock(&rt_pm->lock);

	if (rt_pm->disable_count > 0) {
		k_spin_unlock(&rt_pm->lock, key);
		return -EACCES;
	} else if (rt_pm->usage_count > 0 ||
		   rt_pm->state == RT_DPM_SUSPENDED) {
		k_spin_unlock(&rt_pm->lock, key);
		return 0;
	}

	rt_pm->state = RT_DPM_SUSPENDING;

	if (rt_pm->ops && rt_pm->ops->pre_suspend) {
		k_spin_unlock(&rt_pm->lock, key);
		ret = (rt_pm->ops->pre_suspend)(dev);
		key = k_spin_lock(&rt_pm->lock);
	}

	if (ret) {
		rt_pm->state = RT_DPM_ACTIVE;
	} else {
		if (rt_pm->ops && rt_pm->ops->suspend) {
			(rt_pm->ops->suspend)(dev);
		}
		rt_pm->state = RT_DPM_SUSPENDED;
	}

	/* resume the pended thread for the possible concurrency */
	thread = z_unpend_first_thread(&rt_pm->wait_q);
	if (thread) {
		arch_thread_return_value_set(thread, 0);
		z_ready_thread(thread);
		z_reschedule(&rt_pm->lock, key);
	} else {
		k_spin_unlock(&rt_pm->lock, key);
	}

	if (ret == 0) {
		DEVICE_PARENT_FOREACH(dev, parent) {
			rt_dpm_release_async(parent);
		}
	}

	return ret;
}

/* Try to resume the given device after successfully resume all
 * its parent.
 */
static int rt_dpm_claim_helper(struct rt_dpm *rt_pm)
{
	k_spinlock_key_t key;
	struct k_thread *thread;
	int ret = 0, claimed_parent_count = 0;
	struct device *dev = CONTAINER_OF(rt_pm, struct device, rt_pm);

	key = k_spin_lock(&rt_pm->lock);

again:
	if (rt_pm->disable_count > 0) {
		k_spin_unlock(&rt_pm->lock, key);
		return -EACCES;
	} else if (rt_pm->state == RT_DPM_ACTIVE) {
		k_spin_unlock(&rt_pm->lock, key);
		return 0;
	}

	/* pend current thread for the ongoing state transition
	 * and wait previous one completes.
	 */
	if (rt_pm->state == RT_DPM_SUSPENDING
	    || rt_pm->state == RT_DPM_RESUMING) {
		do {
			z_pend_curr(&rt_pm->lock,
				    key, &rt_pm->wait_q, K_FOREVER);
			key = k_spin_lock(&rt_pm->lock);
		} while (rt_pm->state == RT_DPM_SUSPENDING
			 || rt_pm->state == RT_DPM_RESUMING);
		goto again;
	}

	rt_pm->state = RT_DPM_RESUMING;

	/* claim parent first */
	k_spin_unlock(&rt_pm->lock, key);
	DEVICE_PARENT_FOREACH(dev, parent) {
		ret = rt_dpm_claim(parent);
		claimed_parent_count++;
		if (ret) {
			goto out;
		}
	}
	key = k_spin_lock(&rt_pm->lock);

	if (rt_pm->ops && rt_pm->ops->resume) {
		(rt_pm->ops->resume)(dev);
	}

	if (rt_pm->ops && rt_pm->ops->post_resume) {
		k_spin_unlock(&rt_pm->lock, key);
		ret = (rt_pm->ops->post_resume)(dev);
		key = k_spin_lock(&rt_pm->lock);
	}

out:
	rt_pm->state = ret ? RT_DPM_SUSPENDED : RT_DPM_ACTIVE;

	/* resume the pended thread for the possible concurrency */
	thread = z_unpend_first_thread(&rt_pm->wait_q);
	if (thread) {
		arch_thread_return_value_set(thread, 0);
		z_ready_thread(thread);
		z_reschedule(&rt_pm->lock, key);
	} else {
		k_spin_unlock(&rt_pm->lock, key);
	}

	/* release the claimed parent if any resume failed */
	if (ret) {
		DEVICE_PARENT_FOREACH(dev, parent) {
			if (claimed_parent_count) {
				rt_dpm_release_async(parent);
				claimed_parent_count--;
			} else {
				return ret;
			}
		}
	}

	return ret;
}

/* Device runtime power management work handler */
static void rt_dpm_work_handler(struct k_work *work)
{
	struct rt_dpm *rt_pm = CONTAINER_OF(work, struct rt_dpm, work);

	rt_dpm_release_helper(rt_pm);
}

/* Atomically decrease usage count of given device.
 *
 * Return 0 if more than one user is holding the device currently.
 * Return 1 to indicate further process needed when usage count is
 * crossing 1-0 boundary. Assert will happen if it's not releasing
 * previous claim.
 */
static int decrease_usage_count(struct rt_dpm *rt_pm)
{
	atomic_val_t pre_usage_count;

	pre_usage_count = atomic_dec(&rt_pm->usage_count);
	if (pre_usage_count > 1) {
		return 0;
	}
	__ASSERT_NO_MSG(pre_usage_count == 1);

	return 1;
}

int rt_dpm_release_async(struct device *dev)
{
	int ret;
	struct rt_dpm *rt_pm = dev->rt_pm;

	ret = decrease_usage_count(rt_pm);
	if (ret == 0) {
		return 0;
	}

	k_work_submit(&rt_pm->work);

	return 1;
}

int rt_dpm_release(struct device *dev)
{
	int ret;
	struct rt_dpm *rt_pm = dev->rt_pm;

	ret = decrease_usage_count(rt_pm);
	if (ret == 0) {
		return 0;
	}

	return rt_dpm_release_helper(rt_pm);
}

int rt_dpm_claim(struct device *dev)
{
	struct rt_dpm *rt_pm = dev->rt_pm;

	atomic_inc(&rt_pm->usage_count);

	return rt_dpm_claim_helper(rt_pm);
}

void rt_dpm_enable(struct device *dev)
{
	k_spinlock_key_t key;
	struct rt_dpm *rt_pm = dev->rt_pm;

	key = k_spin_lock(&rt_pm->lock);

	if (rt_pm->disable_count > 0) {
		rt_pm->disable_count--;
	}

	k_spin_unlock(&rt_pm->lock, key);
}

void rt_dpm_disable(struct device *dev)
{
	k_spinlock_key_t key;
	struct rt_dpm *rt_pm = dev->rt_pm;

	key = k_spin_lock(&rt_pm->lock);

	if (rt_pm->disable_count == UINT32_MAX) {
		goto out;
	} else if (rt_pm->disable_count > 0) {
		rt_pm->disable_count++;
		goto out;
	}

	rt_pm->disable_count++;

	/* wait util previous state transition completes */
	if (rt_pm->state == RT_DPM_SUSPENDING
	    || rt_pm->state == RT_DPM_RESUMING) {
		do {
			z_pend_curr(&rt_pm->lock,
				    key, &rt_pm->wait_q, K_FOREVER);
			key = k_spin_lock(&rt_pm->lock);
		} while (rt_pm->state == RT_DPM_SUSPENDING
			 || rt_pm->state == RT_DPM_RESUMING);
	}

out:
	k_spin_unlock(&rt_pm->lock, key);
}

void rt_dpm_init(struct device *dev, bool is_suspend)
{
	k_spinlock_key_t key;
	struct rt_dpm *rt_pm = dev->rt_pm;

	key = k_spin_lock(&rt_pm->lock);

	rt_pm->usage_count = 0;
	rt_pm->disable_count = 0;
	k_work_init(&rt_pm->work, rt_dpm_work_handler);
	z_waitq_init(&rt_pm->wait_q);
	rt_pm->state = is_suspend ? RT_DPM_SUSPENDED : RT_DPM_ACTIVE;

	k_spin_unlock(&rt_pm->lock, key);
}

bool rt_dpm_is_active_state(struct device *dev);
{
	struct rt_dpm *rt_pm = dev->rt_pm;

	return rt_pm->state == RT_DPM_ACTIVE;
}

bool rt_dpm_is_suspend_state(struct device *dev)
{
	struct rt_dpm *rt_pm = dev->rt_pm;

	return rt_pm->state == RT_DPM_SUSPENDED;
}
