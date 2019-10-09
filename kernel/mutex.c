/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file @brief mutex kernel services
 *
 * This module contains routines for handling mutex locking and unlocking.
 *
 * Mutexes implement a priority inheritance algorithm that boosts the priority
 * level of the owning thread to match the priority level of the highest
 * priority thread waiting on the mutex.
 *
 * Each mutex that contributes to priority inheritance must be released in the
 * reverse order in which it was acquired.  Furthermore each subsequent mutex
 * that contributes to raising the owning thread's priority level must be
 * acquired at a point after the most recent "bumping" of the priority level.
 *
 * For example, if thread A has two mutexes contributing to the raising of its
 * priority level, the second mutex M2 must be acquired by thread A after
 * thread A's priority level was bumped due to owning the first mutex M1.
 * When releasing the mutex, thread A must release M2 before it releases M1.
 * Failure to follow this nested model may result in threads running at
 * unexpected priority levels (too high, or too low).
 */

#include <kernel.h>
#include <kernel_structs.h>
#include <toolchain.h>
#include <linker/sections.h>
#include <wait_q.h>
#include <sys/dlist.h>
#include <debug/object_tracing_common.h>
#include <errno.h>
#include <init.h>
#include <syscall_handler.h>
#include <debug/tracing.h>

/* We use a global spinlock here because some of the synchronization
 * is protecting things like owner thread priorities which aren't
 * "part of" a single k_mutex.  Should move those bits of the API
 * under the scheduler lock so we can break this up.
 */
static struct k_spinlock lock;

#ifdef CONFIG_OBJECT_TRACING

struct k_mutex *_trace_list_k_mutex;

/*
 * Complete initialization of statically defined mutexes.
 */
static int init_mutex_module(struct device *dev)
{
	ARG_UNUSED(dev);

	Z_STRUCT_SECTION_FOREACH(k_mutex, mutex) {
		SYS_TRACING_OBJ_INIT(k_mutex, mutex);
	}
	return 0;
}

SYS_INIT(init_mutex_module, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_OBJECTS);

#endif /* CONFIG_OBJECT_TRACING */

void z_impl_k_mutex_init(struct k_mutex *mutex)
{
	mutex->owner = NULL;
	mutex->lock_count = 0U;

	sys_trace_void(SYS_TRACE_ID_MUTEX_INIT);

	z_waitq_init(&mutex->wait_q);

	SYS_TRACING_OBJ_INIT(k_mutex, mutex);
	z_object_init(mutex);
	sys_trace_end_call(SYS_TRACE_ID_MUTEX_INIT);
}

#ifdef CONFIG_USERSPACE
static inline void z_vrfy_k_mutex_init(struct k_mutex *mutex)
{
	Z_OOPS(Z_SYSCALL_OBJ_INIT(mutex, K_OBJ_MUTEX));
	z_impl_k_mutex_init(mutex);
}
#include <syscalls/k_mutex_init_mrsh.c>
#endif

static s32_t new_prio_for_inheritance(s32_t target, s32_t limit)
{
	int new_prio = z_is_prio_higher(target, limit) ? target : limit;

	new_prio = z_get_new_prio_with_ceiling(new_prio);

	return new_prio;
}

static bool adjust_owner_prio(struct k_mutex *mutex, s32_t new_prio)
{
	if (mutex->owner->base.prio != new_prio) {

		K_DEBUG("%p (ready (y/n): %c) prio changed to %d (was %d)\n",
			mutex->owner, z_is_thread_ready(mutex->owner) ?
			'y' : 'n',
			new_prio, mutex->owner->base.prio);

		return z_set_prio(mutex->owner, new_prio);
	}
	return false;
}

int z_impl_k_mutex_lock(struct k_mutex *mutex, s32_t timeout)
{
	int new_prio;
	k_spinlock_key_t key;
	bool resched = false;

	sys_trace_void(SYS_TRACE_ID_MUTEX_LOCK);
	key = k_spin_lock(&lock);

	if (likely((mutex->lock_count == 0U) || (mutex->owner == _current))) {

		mutex->owner_orig_prio = (mutex->lock_count == 0U) ?
					_current->base.prio :
					mutex->owner_orig_prio;

		mutex->lock_count++;
		mutex->owner = _current;

		K_DEBUG("%p took mutex %p, count: %d, orig prio: %d\n",
			_current, mutex, mutex->lock_count,
			mutex->owner_orig_prio);

		k_spin_unlock(&lock, key);
		sys_trace_end_call(SYS_TRACE_ID_MUTEX_LOCK);

		return 0;
	}

	if (unlikely(timeout == (s32_t)K_NO_WAIT)) {
		k_spin_unlock(&lock, key);
		sys_trace_end_call(SYS_TRACE_ID_MUTEX_LOCK);
		return -EBUSY;
	}

	new_prio = new_prio_for_inheritance(_current->base.prio,
					    mutex->owner->base.prio);

	K_DEBUG("adjusting prio up on mutex %p\n", mutex);

	if (z_is_prio_higher(new_prio, mutex->owner->base.prio)) {
		resched = adjust_owner_prio(mutex, new_prio);
	}

	int got_mutex = z_pend_curr(&lock, key, &mutex->wait_q, timeout);

	K_DEBUG("on mutex %p got_mutex value: %d\n", mutex, got_mutex);

	K_DEBUG("%p got mutex %p (y/n): %c\n", _current, mutex,
		got_mutex ? 'y' : 'n');

	if (got_mutex == 0) {
		sys_trace_end_call(SYS_TRACE_ID_MUTEX_LOCK);
		return 0;
	}

	/* timed out */

	K_DEBUG("%p timeout on mutex %p\n", _current, mutex);

	key = k_spin_lock(&lock);

	struct k_thread *waiter = z_waitq_head(&mutex->wait_q);

	new_prio = mutex->owner_orig_prio;
	new_prio = (waiter != NULL) ?
		new_prio_for_inheritance(waiter->base.prio, new_prio) :
		new_prio;

	K_DEBUG("adjusting prio down on mutex %p\n", mutex);

	resched = adjust_owner_prio(mutex, new_prio) || resched;

	if (resched) {
		z_reschedule(&lock, key);
	} else {
		k_spin_unlock(&lock, key);
	}

	sys_trace_end_call(SYS_TRACE_ID_MUTEX_LOCK);
	return -EAGAIN;
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_k_mutex_lock(struct k_mutex *mutex, s32_t timeout)
{
	Z_OOPS(Z_SYSCALL_OBJ(mutex, K_OBJ_MUTEX));
	return z_impl_k_mutex_lock(mutex, timeout);
}
#include <syscalls/k_mutex_lock_mrsh.c>
#endif

void z_impl_k_mutex_unlock(struct k_mutex *mutex)
{
	struct k_thread *new_owner;

	__ASSERT(mutex->lock_count > 0U, "");
	__ASSERT(mutex->owner == _current, "");

	sys_trace_void(SYS_TRACE_ID_MUTEX_UNLOCK);
	z_sched_lock();

	K_DEBUG("mutex %p lock_count: %d\n", mutex, mutex->lock_count);

	if (mutex->lock_count - 1U != 0U) {
		mutex->lock_count--;
		goto k_mutex_unlock_return;
	}

	k_spinlock_key_t key = k_spin_lock(&lock);

	adjust_owner_prio(mutex, mutex->owner_orig_prio);

	new_owner = z_unpend_first_thread(&mutex->wait_q);

	mutex->owner = new_owner;

	K_DEBUG("new owner of mutex %p: %p (prio: %d)\n",
		mutex, new_owner, new_owner ? new_owner->base.prio : -1000);

	if (new_owner != NULL) {
		z_ready_thread(new_owner);

		k_spin_unlock(&lock, key);

		z_arch_thread_return_value_set(new_owner, 0);

		/*
		 * new owner is already of higher or equal prio than first
		 * waiter since the wait queue is priority-based: no need to
		 * ajust its priority
		 */
		mutex->owner_orig_prio = new_owner->base.prio;
	} else {
		mutex->lock_count = 0U;
		k_spin_unlock(&lock, key);
	}


k_mutex_unlock_return:
	k_sched_unlock();
	sys_trace_end_call(SYS_TRACE_ID_MUTEX_UNLOCK);
}

#ifdef CONFIG_USERSPACE
static inline void z_vrfy_k_mutex_unlock(struct k_mutex *mutex)
{
	Z_OOPS(Z_SYSCALL_OBJ(mutex, K_OBJ_MUTEX));
	Z_OOPS(Z_SYSCALL_VERIFY(mutex->lock_count > 0));
	Z_OOPS(Z_SYSCALL_VERIFY(mutex->owner == _current));
	z_impl_k_mutex_unlock(mutex);
}
#include <syscalls/k_mutex_unlock_mrsh.c>
#endif
