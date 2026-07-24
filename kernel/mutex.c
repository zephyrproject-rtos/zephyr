/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief mutex kernel services
 *
 * This module contains routines for handling mutex locking and unlocking.
 *
 * Mutexes implement a priority inheritance algorithm that boosts the priority
 * level of the owning thread to match the priority level of the highest
 * priority thread waiting on any mutex it holds.
 *
 * When a thread pends on a contended mutex, the priority boost propagates
 * through the entire ownership chain: if the mutex owner is itself blocked
 * on another mutex, that owner is also boosted, and so on until the end of
 * the chain or a deadlock cycle is detected.
 *
 * Each thread maintains a list of all mutexes it currently holds
 * (held_mutexes). On unlock, the thread's priority is recalculated by
 * scanning the wait queues of all remaining held mutexes, ensuring the
 * correct priority is maintained when multiple mutexes are involved.
 *
 * Known limitations:
 * - Deferred priority drop on timeout: when a waiter times out, the owner's
 *   priority is adjusted in the waiter's context after it resumes, not at ISR
 *   level. The owner may run at a slightly higher priority than it should for
 *   a short time, but all threads will eventually run correctly.
 *   Fixing this requires acquiring mutex_lock from the timer handler, which
 *   is not safe: on single-core, mutex_lock disables interrupts and the ISR
 *   would spin forever if a thread holds it; on SMP, it reverses the lock
 *   ordering in k_mutex_unlock (mutex_lock -> _sched_spinlock) and deadlocks.
 * - Chain priority-down not propagated on timeout: when a timeout occurs in a
 *   multi-hop ownership chain, only the immediate owner's priority is adjusted;
 *   deeper owners remain over-boosted until they release their mutexes. The
 *   impact and self-correcting nature are the same as above.
 *   Propagating the drop through the full chain would require a chain walk of
 *   unbounded length in the timer handler under the scheduler lock, adding
 *   unbounded ISR latency.
 */

#include <zephyr/kernel.h>
#include <zephyr/toolchain.h>
#include <ksched.h>
#include <scheduler.h>
#include <kthread.h>
#include <wait_q.h>
#include <errno.h>
#include <zephyr/init.h>
#include <zephyr/internal/syscall_handler.h>
#include <zephyr/tracing/tracing.h>
#include <zephyr/sys/check.h>
#include <zephyr/logging/log.h>
#include <zephyr/llext/symbol.h>
LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

/* We use a global spinlock here because some of the synchronization
 * is protecting things like owner thread priorities which aren't
 * "part of" a single k_mutex.  Should move those bits of the API
 * under the scheduler lock so we can break this up.
 */
static struct k_spinlock mutex_lock;

#ifdef CONFIG_OBJ_CORE_MUTEX
static struct k_obj_type obj_type_mutex;
#endif /* CONFIG_OBJ_CORE_MUTEX */

int z_impl_k_mutex_init(struct k_mutex *mutex)
{
	mutex->owner = NULL;
	mutex->lock_count = 0U;

	z_waitq_init(&mutex->wait_q);

#if (CONFIG_PRIORITY_CEILING < K_LOWEST_THREAD_PRIO)
	mutex->held_node.next = NULL;
#endif

	k_object_init(mutex);

#ifdef CONFIG_OBJ_CORE_MUTEX
	k_obj_core_init_and_link(K_OBJ_CORE(mutex), &obj_type_mutex);
#endif /* CONFIG_OBJ_CORE_MUTEX */

	SYS_PORT_TRACING_OBJ_INIT(k_mutex, mutex, 0);

	return 0;
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_k_mutex_init(struct k_mutex *mutex)
{
	K_OOPS(K_SYSCALL_OBJ_INIT(mutex, K_OBJ_MUTEX));
	return z_impl_k_mutex_init(mutex);
}
#include <zephyr/syscalls/k_mutex_init_mrsh.c>
#endif /* CONFIG_USERSPACE */

#if (CONFIG_PRIORITY_CEILING < K_LOWEST_THREAD_PRIO)
static int32_t new_prio_for_inheritance(int32_t target, int32_t limit)
{
	int new_prio = z_is_prio_higher(target, limit) ? target : limit;

	new_prio = z_get_new_prio_with_ceiling(new_prio);

	return new_prio;
}

static bool adjust_owner_prio(struct k_mutex *mutex, int32_t new_prio)
{
	if (mutex->owner->base.prio != new_prio) {

		LOG_DBG("%p (ready (y/n): %c) prio changed to %d (was %d)",
			mutex->owner, z_is_thread_ready(mutex->owner) ?
			'y' : 'n',
			new_prio, mutex->owner->base.prio);

		return z_thread_prio_set(mutex->owner, new_prio);
	}
	return false;
}

/*
 * Scan all mutexes held by a thread and return the highest priority
 * among all their waiters, floored at floor_prio.
 */
static int32_t held_mutexes_highest_waiter_prio(struct k_thread *thread,
						 int32_t floor_prio)
{
	sys_snode_t *node;
	int32_t prio = floor_prio;

	SYS_SLIST_FOR_EACH_NODE(&thread->held_mutexes, node) {
		struct k_mutex *m =
			CONTAINER_OF(node, struct k_mutex, held_node);
		struct k_thread *waiter = z_waitq_head(&m->wait_q);

		if (waiter != NULL &&
		    z_is_prio_higher(waiter->base.prio, prio)) {
			prio = waiter->base.prio;
		}
	}
	return prio;
}
#endif

int z_impl_k_mutex_lock(struct k_mutex *mutex, k_timeout_t timeout)
{
	k_spinlock_key_t key;
#if (CONFIG_PRIORITY_CEILING < K_LOWEST_THREAD_PRIO)
	bool resched = false;
#endif

	__ASSERT(!arch_is_in_isr(), "mutexes cannot be used inside ISRs");

	SYS_PORT_TRACING_OBJ_FUNC_ENTER(k_mutex, lock, mutex, timeout);

	key = k_spin_lock(&mutex_lock);

	if (likely((mutex->lock_count == 0U) || (mutex->owner == _current))) {

#if (CONFIG_PRIORITY_CEILING < K_LOWEST_THREAD_PRIO)
		mutex->owner_orig_prio = (mutex->lock_count == 0U) ?
					_current->base.prio :
					mutex->owner_orig_prio;
#endif

		mutex->lock_count++;
		mutex->owner = _current;

#if (CONFIG_PRIORITY_CEILING < K_LOWEST_THREAD_PRIO)
		LOG_DBG("%p took mutex %p, count: %d, orig prio: %d",
			_current, mutex, mutex->lock_count,
			mutex->owner_orig_prio);

		/* Add to held list on first acquisition; recursive locks skip this. */
		if (mutex->lock_count == 1U) {
			sys_slist_append(&_current->held_mutexes, &mutex->held_node);
		}
#else
		LOG_DBG("%p took mutex %p, count: %d",
			_current, mutex, mutex->lock_count);
#endif

		k_spin_unlock(&mutex_lock, key);

		SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_mutex, lock, mutex, timeout, 0);

		return 0;
	}

	if (unlikely(K_TIMEOUT_EQ(timeout, K_NO_WAIT))) {
		k_spin_unlock(&mutex_lock, key);

		SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_mutex, lock, mutex, timeout, -EBUSY);

		return -EBUSY;
	}

	SYS_PORT_TRACING_OBJ_FUNC_BLOCKING(k_mutex, lock, mutex, timeout);

#if (CONFIG_PRIORITY_CEILING < K_LOWEST_THREAD_PRIO)
	/* Record the mutex this thread is blocking on for priority inheritance. */
	_current->mutex_pended_on = mutex;

	/*
	 * Walk the ownership chain, boosting each owner's priority to match
	 * the highest-priority waiter. Detects deadlock cycles along the way.
	 */
	{
		int32_t boost_prio = new_prio_for_inheritance(
					_current->base.prio,
					mutex->owner->base.prio);
		struct k_mutex  *chain_mutex = mutex;
		struct k_thread *chain_owner = mutex->owner;

		LOG_DBG("chain boosting prio on mutex %p", mutex);

		while (chain_owner != NULL) {
#if defined(CONFIG_MUTEX_DEADLOCK_DETECT)
			/*
			 * Deadlock detection: only for K_FOREVER waits.
			 * Finite-timeout waits recover naturally via -EAGAIN.
			 * Guard with z_is_thread_pending() so that a timed-out
			 * thread whose mutex_pended_on has not yet been cleared
			 * does not produce a false positive.
			 */
			if (K_TIMEOUT_EQ(timeout, K_FOREVER) &&
			    chain_owner->mutex_pended_on != NULL &&
			    z_is_thread_pending(chain_owner) &&
			    chain_owner->mutex_pended_on->owner == _current) {
				/*
				 * Release mutex_lock before asserting so that
				 * the lock is not held when the assert fires.
				 */
				_current->mutex_pended_on = NULL;
				k_spin_unlock(&mutex_lock, key);
				__ASSERT(false,
					"mutex deadlock: thread %p waiting on "
					"mutex %p (owner %p)",
					_current, mutex, mutex->owner);
				/*
				 * In release builds (assert is no-op):
				 * re-acquire the lock and block forever.
				 */
				key = k_spin_lock(&mutex_lock);
				break;
			}
#endif /* CONFIG_MUTEX_DEADLOCK_DETECT */

			if (z_is_prio_higher(boost_prio, chain_owner->base.prio)) {
				resched = adjust_owner_prio(chain_mutex, boost_prio)
					  || resched;
			}

			/*
			 * Stop the chain walk if the owner is not actually
			 * pending — its mutex_pended_on may be stale (e.g. it
			 * just timed out but has not yet cleared the field).
			 * Always boost the current owner first, then decide
			 * whether to continue.
			 */
			if (chain_owner->mutex_pended_on == NULL ||
			    !z_is_thread_pending(chain_owner)) {
				break;
			}

			chain_mutex = chain_owner->mutex_pended_on;
			chain_owner = chain_mutex->owner;
		}
	}
#endif

	int got_mutex = z_pend_curr(&mutex_lock, key, &mutex->wait_q, timeout);

	LOG_DBG("on mutex %p got_mutex value: %d", mutex, got_mutex);

	LOG_DBG("%p got mutex %p (y/n): %c", _current, mutex,
		got_mutex ? 'y' : 'n');

#if (CONFIG_PRIORITY_CEILING < K_LOWEST_THREAD_PRIO)
	if (got_mutex == 0) {
		/* Lock granted; clear the pending mutex pointer. */
		_current->mutex_pended_on = NULL;
		SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_mutex, lock, mutex, timeout, 0);
		return 0;
	}

	/* timed out */

	LOG_DBG("%p timeout on mutex %p", _current, mutex);

	key = k_spin_lock(&mutex_lock);

	/*
	 * Clear mutex_pended_on under mutex_lock so no concurrent chain walk
	 * can observe the stale pointer and detect a false deadlock cycle.
	 */
	_current->mutex_pended_on = NULL;

	/*
	 * Check if mutex was unlocked after this thread was unpended.
	 * If so, skip adjusting owner's priority down.
	 */
	if (likely(mutex->owner != NULL)) {
		/*
		 * Recalculate the owner's priority across all mutexes it still
		 * holds; another held mutex may still justify a partial boost.
		 */
		int32_t new_prio = held_mutexes_highest_waiter_prio(
						mutex->owner,
						mutex->owner_orig_prio);

		LOG_DBG("adjusting prio down on mutex %p", mutex);

		resched = adjust_owner_prio(mutex, new_prio) || resched;
	}

	if (resched) {
		z_reschedule(&mutex_lock, key);
	} else {
		k_spin_unlock(&mutex_lock, key);
	}

	SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_mutex, lock, mutex, timeout, -EAGAIN);

	return -EAGAIN;
#else
	SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_mutex, lock, mutex, timeout, got_mutex);

	return got_mutex;
#endif
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_k_mutex_lock(struct k_mutex *mutex,
				      k_timeout_t timeout)
{
	K_OOPS(K_SYSCALL_OBJ(mutex, K_OBJ_MUTEX));
	return z_impl_k_mutex_lock(mutex, timeout);
}
#include <zephyr/syscalls/k_mutex_lock_mrsh.c>
#endif /* CONFIG_USERSPACE */

int z_impl_k_mutex_unlock(struct k_mutex *mutex)
{
	struct k_thread *new_owner = NULL;

	__ASSERT(!arch_is_in_isr(), "mutexes cannot be used inside ISRs");

	SYS_PORT_TRACING_OBJ_FUNC_ENTER(k_mutex, unlock, mutex);

	CHECKIF(mutex->owner == NULL) {
		SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_mutex, unlock, mutex, -EINVAL);

		return -EINVAL;
	}
	/*
	 * The current thread does not own the mutex.
	 */
	CHECKIF(mutex->owner != _current) {
		SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_mutex, unlock, mutex, -EPERM);

		return -EPERM;
	}

	/*
	 * Attempt to unlock a mutex which is unlocked. mutex->lock_count
	 * cannot be zero if the current thread is equal to mutex->owner,
	 * therefore no underflow check is required. Use assert to catch
	 * undefined behavior.
	 */
	__ASSERT_NO_MSG(mutex->lock_count > 0U);

	LOG_DBG("mutex %p lock_count: %d", mutex, mutex->lock_count);

	/*
	 * If we are the owner and count is greater than 1, then decrement
	 * the count and return and keep current thread as the owner.
	 */
	if (mutex->lock_count > 1U) {
		mutex->lock_count--;
		goto k_mutex_unlock_return;
	}

	k_spinlock_key_t key = k_spin_lock(&mutex_lock);

#if (CONFIG_PRIORITY_CEILING < K_LOWEST_THREAD_PRIO)
	/* Remove this mutex from the owner's list of held mutexes */
	sys_slist_find_and_remove(&_current->held_mutexes, &mutex->held_node);

	/*
	 * If the thread's priority was boosted, recalculate it by scanning
	 * the remaining held mutexes. The released mutex has already been
	 * removed from held_mutexes, so it is not included in the scan.
	 */
	if (_current->base.prio != mutex->owner_orig_prio) {
		int32_t new_prio = held_mutexes_highest_waiter_prio(
					_current, mutex->owner_orig_prio);

		adjust_owner_prio(mutex, new_prio);
	}
#endif

	/* Pick the new owner (if any) and complete the wake atomically
	 * under _sched_spinlock, so a racing in-flight timeout handler
	 * cannot observe a half-initialized wake-up.
	 */
	LOCK_SCHED_SPINLOCK {
		new_owner = z_unpend_first_thread_locked(&mutex->wait_q);
		mutex->owner = new_owner;

		LOG_DBG("new owner of mutex %p: %p (prio: %d)",
			mutex, new_owner, new_owner ? new_owner->base.prio : -1000);

		if (unlikely(new_owner != NULL)) {
			/*
			 * new owner is already of higher or equal prio than first
			 * waiter since the wait queue is priority-based: no need to
			 * adjust its priority
			 */
#if (CONFIG_PRIORITY_CEILING < K_LOWEST_THREAD_PRIO)
			mutex->owner_orig_prio = new_owner->base.prio;
			sys_slist_append(&new_owner->held_mutexes, &mutex->held_node);
			new_owner->mutex_pended_on = NULL;
#endif
			arch_thread_return_value_set(new_owner, 0);
			z_sched_ready_locked(new_owner);
		} else {
			mutex->lock_count = 0U;
		}
	}

	if (unlikely(new_owner != NULL)) {
		z_reschedule(&mutex_lock, key);
	} else {
		k_spin_unlock(&mutex_lock, key);
	}


k_mutex_unlock_return:
	SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_mutex, unlock, mutex, 0);

	return 0;
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_k_mutex_unlock(struct k_mutex *mutex)
{
	K_OOPS(K_SYSCALL_OBJ(mutex, K_OBJ_MUTEX));
	return z_impl_k_mutex_unlock(mutex);
}
#include <zephyr/syscalls/k_mutex_unlock_mrsh.c>
#endif /* CONFIG_USERSPACE */

#ifdef CONFIG_OBJ_CORE_MUTEX
K_OBJ_TYPE_DEFINE(obj_type_mutex, k_mutex, K_OBJ_TYPE_MUTEX_ID, NULL);
#endif /* CONFIG_OBJ_CORE_MUTEX */
