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
#include <tracing.h>

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
	__ASSERT(initial_count <= limit, "count cannot be greater than limit");

	sys_trace_void(SYS_TRACE_ID_SEMA_INIT);
	sem->count = initial_count;
	sem->limit = limit;
	_waitq_init(&sem->wait_q);
#if defined(CONFIG_POLL)
	sys_dlist_init(&sem->poll_events);
#endif

	SYS_TRACING_OBJ_INIT(k_sem, sem);

	_k_object_init(sem);
	sys_trace_end_call(SYS_TRACE_ID_SEMA_INIT);
}

#ifdef CONFIG_USERSPACE
Z_SYSCALL_HANDLER(k_sem_init, sem, initial_count, limit)
{
	Z_OOPS(Z_SYSCALL_OBJ_INIT(sem, K_OBJ_SEM));
	Z_OOPS(Z_SYSCALL_VERIFY(limit != 0 && initial_count <= limit));
	_impl_k_sem_init((struct k_sem *)sem, initial_count, limit);
	return 0;
}
#endif

static inline void handle_poll_events(struct k_sem *sem)
{
#ifdef CONFIG_POLL
	_handle_obj_poll_events(&sem->poll_events, K_POLL_STATE_SEM_AVAILABLE);
#endif
}

static inline void increment_count_up_to_limit(struct k_sem *sem)
{
	sem->count += (sem->count != sem->limit);
}

static void do_sem_give(struct k_sem *sem)
{
	struct k_thread *thread = _unpend_first_thread(&sem->wait_q);

	if (thread) {
		_ready_thread(thread);
		_set_thread_return_value(thread, 0);
	} else {
		increment_count_up_to_limit(sem);
		handle_poll_events(sem);
	}
}

void _impl_k_sem_give(struct k_sem *sem)
{
	unsigned int key = irq_lock();

	sys_trace_void(SYS_TRACE_ID_SEMA_GIVE);
	do_sem_give(sem);
	sys_trace_end_call(SYS_TRACE_ID_SEMA_GIVE);
	_reschedule(key);
}

#ifdef CONFIG_USERSPACE
Z_SYSCALL_HANDLER1_SIMPLE_VOID(k_sem_give, K_OBJ_SEM, struct k_sem *);
#endif

int _impl_k_sem_take(struct k_sem *sem, s32_t timeout)
{
	__ASSERT(!_is_in_isr() || timeout == K_NO_WAIT, "");

	sys_trace_void(SYS_TRACE_ID_SEMA_TAKE);
	unsigned int key = irq_lock();

	if (likely(sem->count > 0)) {
		sem->count--;
		irq_unlock(key);
		sys_trace_end_call(SYS_TRACE_ID_SEMA_TAKE);
		return 0;
	}

	if (timeout == K_NO_WAIT) {
		irq_unlock(key);
		sys_trace_end_call(SYS_TRACE_ID_SEMA_TAKE);
		return -EBUSY;
	}

	sys_trace_end_call(SYS_TRACE_ID_SEMA_TAKE);

	return _pend_current_thread(key, &sem->wait_q, timeout);
}

#ifdef CONFIG_USERSPACE
Z_SYSCALL_HANDLER(k_sem_take, sem, timeout)
{
	Z_OOPS(Z_SYSCALL_OBJ(sem, K_OBJ_SEM));
	return _impl_k_sem_take((struct k_sem *)sem, timeout);
}

Z_SYSCALL_HANDLER1_SIMPLE_VOID(k_sem_reset, K_OBJ_SEM, struct k_sem *);
Z_SYSCALL_HANDLER1_SIMPLE(k_sem_count_get, K_OBJ_SEM, struct k_sem *);
#endif
