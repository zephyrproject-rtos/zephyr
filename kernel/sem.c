/*
 * Copyright (c) 2010-2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Kernel semaphore object.
 *
 * The semaphores are of the 'counting' type, i.e. each 'give' operation will
 * increment the internal count by 1, if no thread is pending on it. The 'init'
 * call initializes the count to 'initial_count'. Following multiple 'give'
 * operations, the same number of 'take' operations can be performed without
 * the calling thread having to pend on the semaphore, or the calling task
 * having to poll.
 */

#include <kernel.h>
#include <kernel_structs.h>
#include <debug/object_tracing_common.h>
#include <toolchain.h>
#include <linker/sections.h>
#include <wait_q.h>
#include <misc/dlist.h>
#include <ksched.h>
#include <init.h>
#include <syscall_handler.h>

extern struct k_sem _k_sem_list_start[];
extern struct k_sem _k_sem_list_end[];

#ifdef CONFIG_OBJECT_TRACING

struct k_sem *_trace_list_k_sem;

/*
 * Complete initialization of statically defined semaphores.
 */
static int init_sem_module(struct device *dev)
{
	ARG_UNUSED(dev);

	struct k_sem *sem;

	for (sem = _k_sem_list_start; sem < _k_sem_list_end; sem++) {
		SYS_TRACING_OBJ_INIT(k_sem, sem);
	}
	return 0;
}

SYS_INIT(init_sem_module, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_OBJECTS);

#endif /* CONFIG_OBJECT_TRACING */

void _impl_k_sem_init(struct k_sem *sem, unsigned int initial_count,
		      unsigned int limit)
{
	__ASSERT(limit != 0, "limit cannot be zero");

	sem->count = initial_count;
	sem->limit = limit;
	sys_dlist_init(&sem->wait_q);
#if defined(CONFIG_POLL)
	sys_dlist_init(&sem->poll_events);
#endif

	SYS_TRACING_OBJ_INIT(k_sem, sem);

	_k_object_init(sem);
}

#ifdef CONFIG_USERSPACE
_SYSCALL_HANDLER(k_sem_init, sem, initial_count, limit)
{
	_SYSCALL_OBJ_INIT(sem, K_OBJ_SEM);
	_SYSCALL_VERIFY(limit != 0);
	_impl_k_sem_init((struct k_sem *)sem, initial_count, limit);
	return 0;
}
#endif

/* returns 1 if a reschedule must take place, 0 otherwise */
static inline int handle_poll_events(struct k_sem *sem)
{
#ifdef CONFIG_POLL
	u32_t state = K_POLL_STATE_SEM_AVAILABLE;

	return _handle_obj_poll_events(&sem->poll_events, state);
#else
	return 0;
#endif
}

static inline void increment_count_up_to_limit(struct k_sem *sem)
{
	sem->count += (sem->count != sem->limit);
}

/* returns 1 if _Swap() will need to be invoked, 0 otherwise */
static int do_sem_give(struct k_sem *sem)
{
	struct k_thread *thread = _unpend_first_thread(&sem->wait_q);

	if (!thread) {
		increment_count_up_to_limit(sem);
		return handle_poll_events(sem);
	}
	(void)_abort_thread_timeout(thread);
	_ready_thread(thread);
	_set_thread_return_value(thread, 0);

	return !_is_in_isr() && _must_switch_threads();
}

/*
 * This function is meant to be called only by
 * _sys_event_logger_put_non_preemptible(), which itself is really meant to be
 * called only by _sys_k_event_logger_context_switch(), used within a context
 * switch to log the event.
 *
 * WARNING:
 * It must be called with interrupts already locked.
 * It cannot be called for a sempahore part of a group.
 */
void _sem_give_non_preemptible(struct k_sem *sem)
{
	struct k_thread *thread;

	thread = _unpend_first_thread(&sem->wait_q);
	if (!thread) {
		increment_count_up_to_limit(sem);
		return;
	}

	_abort_thread_timeout(thread);

	_ready_thread(thread);
	_set_thread_return_value(thread, 0);
}

void _impl_k_sem_give(struct k_sem *sem)
{
	unsigned int key;

	key = irq_lock();

	if (do_sem_give(sem)) {
		_Swap(key);
	} else {
		irq_unlock(key);
	}
}

#ifdef CONFIG_USERSPACE
_SYSCALL_HANDLER1_SIMPLE_VOID(k_sem_give, K_OBJ_SEM, struct k_sem *);
#endif

int _impl_k_sem_take(struct k_sem *sem, s32_t timeout)
{
	__ASSERT(!_is_in_isr() || timeout == K_NO_WAIT, "");

	unsigned int key = irq_lock();

	if (likely(sem->count > 0)) {
		sem->count--;
		irq_unlock(key);
		return 0;
	}

	if (timeout == K_NO_WAIT) {
		irq_unlock(key);
		return -EBUSY;
	}

	_pend_current_thread(&sem->wait_q, timeout);

	return _Swap(key);
}

#ifdef CONFIG_USERSPACE
_SYSCALL_HANDLER(k_sem_take, sem, timeout)
{
	_SYSCALL_OBJ(sem, K_OBJ_SEM);
	return _impl_k_sem_take((struct k_sem *)sem, timeout);
}

_SYSCALL_HANDLER1_SIMPLE_VOID(k_sem_reset, K_OBJ_SEM, struct k_sem *);
_SYSCALL_HANDLER1_SIMPLE(k_sem_count_get, K_OBJ_SEM, struct k_sem *);
#endif
