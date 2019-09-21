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
#include <sys/dlist.h>
#include <ksched.h>
#include <init.h>
#include <syscall_handler.h>
#include <debug/tracing.h>

/* We use a system-wide lock to synchronize semaphores, which has
 * unfortunate performance impact vs. using a per-object lock
 * (semaphores are *very* widely used).  But per-object locks require
 * significant extra RAM.  A properly spin-aware semaphore
 * implementation would spin on atomic access to the count variable,
 * and not a spinlock per se.  Useful optimization for the future...
 */
static struct k_spinlock lock;

#ifdef CONFIG_OBJECT_TRACING

struct k_sem *_trace_list_k_sem;

/*
 * Complete initialization of statically defined semaphores.
 */
static int init_sem_module(struct device *dev)
{
	ARG_UNUSED(dev);

	Z_STRUCT_SECTION_FOREACH(k_sem, sem) {
		SYS_TRACING_OBJ_INIT(k_sem, sem);
	}
	return 0;
}

SYS_INIT(init_sem_module, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_OBJECTS);

#endif /* CONFIG_OBJECT_TRACING */

void z_impl_k_sem_init(struct k_sem *sem, unsigned int initial_count,
		      unsigned int limit)
{
	__ASSERT(limit != 0U, "limit cannot be zero");
	__ASSERT(initial_count <= limit, "count cannot be greater than limit");

	sys_trace_void(SYS_TRACE_ID_SEMA_INIT);
	sem->count = initial_count;
	sem->limit = limit;
	z_waitq_init(&sem->wait_q);
#if defined(CONFIG_POLL)
	sys_dlist_init(&sem->poll_events);
#endif

	SYS_TRACING_OBJ_INIT(k_sem, sem);

	z_object_init(sem);
	sys_trace_end_call(SYS_TRACE_ID_SEMA_INIT);
}

#ifdef CONFIG_USERSPACE
void z_vrfy_k_sem_init(struct k_sem *sem, unsigned int initial_count,
		      unsigned int limit)
{
	Z_OOPS(Z_SYSCALL_OBJ_INIT(sem, K_OBJ_SEM));
	Z_OOPS(Z_SYSCALL_VERIFY(limit != 0 && initial_count <= limit));
	z_impl_k_sem_init(sem, initial_count, limit);
}
#include <syscalls/k_sem_init_mrsh.c>
#endif

static inline void handle_poll_events(struct k_sem *sem)
{
#ifdef CONFIG_POLL
	z_handle_obj_poll_events(&sem->poll_events, K_POLL_STATE_SEM_AVAILABLE);
#else
	ARG_UNUSED(sem);
#endif
}

static inline void increment_count_up_to_limit(struct k_sem *sem)
{
	sem->count += (sem->count != sem->limit) ? 1U : 0U;
}

static void do_sem_give(struct k_sem *sem)
{
	struct k_thread *thread = z_unpend_first_thread(&sem->wait_q);

	if (thread != NULL) {
		z_ready_thread(thread);
		z_arch_thread_return_value_set(thread, 0);
	} else {
		increment_count_up_to_limit(sem);
		handle_poll_events(sem);
	}
}

void z_impl_k_sem_give(struct k_sem *sem)
{
	k_spinlock_key_t key = k_spin_lock(&lock);

	sys_trace_void(SYS_TRACE_ID_SEMA_GIVE);
	do_sem_give(sem);
	sys_trace_end_call(SYS_TRACE_ID_SEMA_GIVE);
	z_reschedule(&lock, key);
}

#ifdef CONFIG_USERSPACE
static inline void z_vrfy_k_sem_give(struct k_sem *sem)
{
	Z_OOPS(Z_SYSCALL_OBJ(sem, K_OBJ_SEM));
	z_impl_k_sem_give(sem);
}
#include <syscalls/k_sem_give_mrsh.c>
#endif

int z_impl_k_sem_take(struct k_sem *sem, s32_t timeout)
{
	__ASSERT(((z_arch_is_in_isr() == false) || (timeout == K_NO_WAIT)), "");

	sys_trace_void(SYS_TRACE_ID_SEMA_TAKE);
	k_spinlock_key_t key = k_spin_lock(&lock);

	if (likely(sem->count > 0U)) {
		sem->count--;
		k_spin_unlock(&lock, key);
		sys_trace_end_call(SYS_TRACE_ID_SEMA_TAKE);
		return 0;
	}

	if (timeout == K_NO_WAIT) {
		k_spin_unlock(&lock, key);
		sys_trace_end_call(SYS_TRACE_ID_SEMA_TAKE);
		return -EBUSY;
	}

	sys_trace_end_call(SYS_TRACE_ID_SEMA_TAKE);

	int ret = z_pend_curr(&lock, key, &sem->wait_q, timeout);
	return ret;
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_k_sem_take(struct k_sem *sem, s32_t timeout)
{
	Z_OOPS(Z_SYSCALL_OBJ(sem, K_OBJ_SEM));
	return z_impl_k_sem_take((struct k_sem *)sem, timeout);
}
#include <syscalls/k_sem_take_mrsh.c>

static inline void z_vrfy_k_sem_reset(struct k_sem *sem)
{
	Z_OOPS(Z_SYSCALL_OBJ(sem, K_OBJ_SEM));
	z_impl_k_sem_reset(sem);
}
#include <syscalls/k_sem_reset_mrsh.c>

static inline unsigned int z_vrfy_k_sem_count_get(struct k_sem *sem)
{
	Z_OOPS(Z_SYSCALL_OBJ(sem, K_OBJ_SEM));
	return z_impl_k_sem_count_get(sem);
}
#include <syscalls/k_sem_count_get_mrsh.c>

#endif
