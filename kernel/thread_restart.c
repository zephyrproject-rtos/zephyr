/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdint.h>

#include <zephyr/kernel.h>
#include <zephyr/kernel/thread_restart.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <kthread.h>

LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

/**
 * @brief Lock for the restart state of all descriptors
 *
 * Serializes the pending check and scheduling in k_thread_restart_schedule() and
 * k_thread_restart_poll_set() against the reschedule decision of z_thread_restart_handler().
 */
static struct k_spinlock restart_lock;

/**
 * @brief Upper bound of the poll interval in ticks
 *
 * Low enough for the doubling in z_thread_restart_handler() and for K_TICKS() to never
 * overflow @c k_ticks_t.
 */
#define POLL_TICKS_LIMIT ((k_ticks_t)(INT32_MAX / 2))

/**
 * @brief Convert a poll interval to ticks
 *
 * @param ms Poll interval in milliseconds
 *
 * @return Interval in ticks, rounded up, at least 1 and at most @c POLL_TICKS_LIMIT
 */
static k_ticks_t poll_ms_to_ticks(uint32_t ms)
{
	uint64_t ticks = k_ms_to_ticks_ceil64(ms);

	if (ticks == 0U) {
		return 1;
	}

	if (ticks > (uint64_t)POLL_TICKS_LIMIT) {
		return POLL_TICKS_LIMIT;
	}

	return (k_ticks_t)ticks;
}

/**
 * @brief Check whether a restart is pending
 *
 * @c rst->pending is set when a restart is scheduled and cleared by the handler once the thread
 * has exited, so a handler that is running but has not decided yet still counts as pending.
 * The work state covers a restart dropped by the workqueue, which leaves the flag set but the
 * work idle.
 *
 * Must be called with @c restart_lock held.
 *
 * @param rst Thread restart descriptor
 *
 * @retval true A restart is scheduled and the thread has not been re-created yet
 * @retval false No restart is pending
 */
static bool restart_pending(const struct k_thread_restart *rst)
{
	return rst->pending && k_work_delayable_is_pending(&rst->work);
}

/**
 * @brief Restart work handler
 *
 * Runs on the system workqueue. While the thread has not exited, reschedules itself with a
 * poll interval that doubles up to @c rst->poll_max_ms. Once the thread has exited, re-creates
 * it with a @c K_FOREVER start delay, sets its name, resource pool and memory domain, runs the
 * descriptor callback and starts it. The thread is aborted instead of started if it cannot be
 * added to its memory domain or if the callback vetoes the restart.
 *
 * @param work Work item of a thread restart descriptor
 */
void z_thread_restart_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct k_thread_restart *rst = CONTAINER_OF(dwork, struct k_thread_restart, work);
	k_spinlock_key_t key = k_spin_lock(&restart_lock);
	k_tid_t tid;
	int ret;

	ret = k_thread_join(rst->thread, K_NO_WAIT);
	if (ret == -EBUSY) {
		k_ticks_t max_ticks = poll_ms_to_ticks(rst->poll_max_ms);
		k_ticks_t poll = MIN(rst->poll_ticks, max_ticks);

		rst->poll_ticks = MIN(poll * 2, max_ticks);

		/* The delay is non-zero, so this is not submitted to the queue now; it returns
		 * 0 only if the work is already pending, which keeps the restart pending as well,
		 * or being cancelled.
		 */
		(void)k_work_schedule(dwork, K_TICKS(poll));
		k_spin_unlock(&restart_lock, key);

		return;
	}

	/* k_thread_join() with K_NO_WAIT returns only 0 or -EBUSY */
	__ASSERT_NO_MSG(ret == 0);

	/* From here on the re-created thread may schedule its next restart */
	rst->pending = false;
	k_spin_unlock(&restart_lock, key);

	LOG_INF("thread %p: restart %u", rst->thread, rst->restart_count);

	tid = k_thread_create(rst->thread, rst->stack, rst->stack_size, rst->entry, rst->p1,
			      rst->p2, rst->p3, rst->prio, rst->options, K_FOREVER);

	if (IS_ENABLED(CONFIG_THREAD_NAME) && (rst->name != NULL)) {
		ret = k_thread_name_set(tid, rst->name);
		if (ret != 0) {
			LOG_WRN("thread %p: cannot set name (%d)", tid, ret);
		}
	}

	/* Restore what k_thread_restart_schedule() recorded instead of inheriting it from the
	 * workqueue thread
	 */
	k_thread_heap_assign(tid, rst->resource_pool);

	if (IS_ENABLED(CONFIG_USERSPACE) && (rst->mem_domain != NULL)) {
		ret = k_mem_domain_add_thread(rst->mem_domain, tid);
		if (ret != 0) {
			LOG_ERR("thread %p: cannot add to memory domain (%d)", tid, ret);
			k_thread_abort(tid);

			return;
		}
	}

	if (rst->cb != NULL) {
		ret = rst->cb(rst, tid);
		if (ret != 0) {
			LOG_ERR("thread %p: restart vetoed (%d)", tid, ret);
			k_thread_abort(tid);

			return;
		}
	}

	/* Last access to rst: once started, the thread may fault and reschedule. */
	k_thread_start(tid);
}

void k_thread_restart_init(struct k_thread_restart *rst, struct k_thread *thread,
			   k_thread_stack_t *stack, size_t stack_size, k_thread_entry_t entry,
			   void *p1, void *p2, void *p3, int prio, uint32_t options,
			   const char *name, uint32_t max_restarts, k_thread_restart_cb_t cb)
{
	__ASSERT_NO_MSG(rst != NULL);

	rst->thread = thread;
	rst->stack = stack;
	rst->stack_size = stack_size;
	rst->entry = entry;
	rst->p1 = p1;
	rst->p2 = p2;
	rst->p3 = p3;
	rst->prio = prio;
	rst->options = options;
	rst->name = name;
	rst->max_restarts = max_restarts;
	rst->cb = cb;

	rst->poll_min_ms = CONFIG_THREAD_RESTART_ON_FATAL_ERROR_POLL_MIN_MS;
	rst->poll_max_ms = CONFIG_THREAD_RESTART_ON_FATAL_ERROR_POLL_MAX_MS;
	rst->resource_pool = NULL;
	rst->mem_domain = NULL;
	rst->restart_count = 0U;
	rst->poll_ticks = 0;
	rst->pending = false;
	k_work_init_delayable(&rst->work, z_thread_restart_handler);
}

int k_thread_restart_poll_set(struct k_thread_restart *rst, uint32_t min_ms, uint32_t max_ms)
{
	k_spinlock_key_t key;
	int ret = -EBUSY;

	if ((rst == NULL) || (min_ms == 0U) || (min_ms > max_ms)) {
		return -EINVAL;
	}

	key = k_spin_lock(&restart_lock);

	if (!restart_pending(rst)) {
		rst->poll_min_ms = min_ms;
		rst->poll_max_ms = max_ms;
		ret = 0;
	}

	k_spin_unlock(&restart_lock, key);

	return ret;
}

int k_thread_restart_schedule(struct k_thread_restart *rst)
{
	k_spinlock_key_t key;
	int ret;

	/* K_INHERIT_PERMS would grant the workqueue thread's object permissions */
	if ((rst == NULL) || (rst->thread == NULL) || (rst->thread != _current) ||
	    ((rst->options & K_INHERIT_PERMS) != 0U)) {
		return -EINVAL;
	}

	if ((rst->thread == k_work_queue_thread_get(&k_sys_work_q)) ||
	    z_is_thread_essential(rst->thread) || k_thread_is_user_thread(rst->thread) ||
	    ((rst->options & (K_USER | K_ESSENTIAL)) != 0U)) {
		return -ENOTSUP;
	}

	key = k_spin_lock(&restart_lock);

	if (restart_pending(rst)) {
		ret = -EALREADY;
	} else if ((rst->max_restarts != 0U) && (rst->restart_count >= rst->max_restarts)) {
		ret = -ENOSPC;
	} else {
		rst->poll_ticks = poll_ms_to_ticks(rst->poll_min_ms);

		/* The handler only reads these after taking restart_lock */
		rst->resource_pool = rst->thread->resource_pool;
#ifdef CONFIG_USERSPACE
		rst->mem_domain = rst->thread->mem_domain_info.mem_domain;
#endif /* CONFIG_USERSPACE */

		ret = k_work_schedule(&rst->work, K_NO_WAIT);
		if (ret > 0) {
			rst->pending = true;
			rst->restart_count++;
			ret = 0;
		} else if (ret == 0) {
			ret = -EALREADY;
		} else {
			/* Error from k_work_schedule() */
		}
	}

	k_spin_unlock(&restart_lock, key);

	return ret;
}
