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

#include <zephyr/kernel.h>
#include <zephyr/kernel_structs.h>

#include <zephyr/toolchain.h>
#include <wait_q.h>
#include <zephyr/sys/dlist.h>
#include <ksched.h>
#include <zephyr/init.h>
#include <zephyr/internal/syscall_handler.h>
#include <zephyr/tracing/tracing.h>
#include <zephyr/sys/check.h>

/* We use a system-wide lock to synchronize semaphores, which has
 * unfortunate performance impact vs. using a per-object lock
 * (semaphores are *very* widely used).  But per-object locks require
 * significant extra RAM.  A properly spin-aware semaphore
 * implementation would spin on atomic access to the count variable,
 * and not a spinlock per se.  Useful optimization for the future...
 */
static struct k_spinlock lock;

#ifdef CONFIG_OBJ_CORE_SEM
static struct k_obj_type obj_type_sem;
#endif /* CONFIG_OBJ_CORE_SEM */

int z_impl_k_sem_init(struct k_sem *sem, unsigned int initial_count,
		      unsigned int limit)
{
	/*
	 * Limit cannot be zero and count cannot be greater than limit
	 */
	CHECKIF(limit == 0U || initial_count > limit) {
		SYS_PORT_TRACING_OBJ_FUNC(k_sem, init, sem, -EINVAL);

		return -EINVAL;
	}

	sem->count = initial_count;
	sem->limit = limit;

	SYS_PORT_TRACING_OBJ_FUNC(k_sem, init, sem, 0);

	z_waitq_init(&sem->wait_q);
#if defined(CONFIG_POLL)
	sys_dlist_init(&sem->poll_events);
#endif /* CONFIG_POLL */
	k_object_init(sem);

#ifdef CONFIG_OBJ_CORE_SEM
	k_obj_core_init_and_link(K_OBJ_CORE(sem), &obj_type_sem);
#endif /* CONFIG_OBJ_CORE_SEM */

	return 0;
}

#ifdef CONFIG_USERSPACE
int z_vrfy_k_sem_init(struct k_sem *sem, unsigned int initial_count,
		      unsigned int limit)
{
	K_OOPS(K_SYSCALL_OBJ_INIT(sem, K_OBJ_SEM));
	return z_impl_k_sem_init(sem, initial_count, limit);
}
#include <zephyr/syscalls/k_sem_init_mrsh.c>
#endif /* CONFIG_USERSPACE */

static inline bool handle_poll_events(struct k_sem *sem)
{
#ifdef CONFIG_POLL
	z_handle_obj_poll_events(&sem->poll_events, K_POLL_STATE_SEM_AVAILABLE);
	return true;
#else
	ARG_UNUSED(sem);
	return false;
#endif /* CONFIG_POLL */
}

void z_impl_k_sem_give(struct k_sem *sem)
{
	k_spinlock_key_t key = k_spin_lock(&lock);
	struct k_thread *thread;
	bool resched = true;

	SYS_PORT_TRACING_OBJ_FUNC_ENTER(k_sem, give, sem);

	thread = z_unpend_first_thread(&sem->wait_q);

	if (thread != NULL) {
		arch_thread_return_value_set(thread, 0);
		z_ready_thread(thread);
	} else {
		sem->count += (sem->count != sem->limit) ? 1U : 0U;
		resched = handle_poll_events(sem);
	}

	if (resched) {
		z_reschedule(&lock, key);
	} else {
		k_spin_unlock(&lock, key);
	}

	SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_sem, give, sem);
}

#ifdef CONFIG_USERSPACE
static inline void z_vrfy_k_sem_give(struct k_sem *sem)
{
	K_OOPS(K_SYSCALL_OBJ(sem, K_OBJ_SEM));
	z_impl_k_sem_give(sem);
}
#include <zephyr/syscalls/k_sem_give_mrsh.c>
#endif /* CONFIG_USERSPACE */

int z_impl_k_sem_take(struct k_sem *sem, k_timeout_t timeout)
{
	int ret;

	__ASSERT(((arch_is_in_isr() == false) ||
		  K_TIMEOUT_EQ(timeout, K_NO_WAIT)), "");

	k_spinlock_key_t key = k_spin_lock(&lock);

	SYS_PORT_TRACING_OBJ_FUNC_ENTER(k_sem, take, sem, timeout);

	if (likely(sem->count > 0U)) {
		sem->count--;
		k_spin_unlock(&lock, key);
		ret = 0;
		goto out;
	}

	if (K_TIMEOUT_EQ(timeout, K_NO_WAIT)) {
		k_spin_unlock(&lock, key);
		ret = -EBUSY;
		goto out;
	}

	SYS_PORT_TRACING_OBJ_FUNC_BLOCKING(k_sem, take, sem, timeout);

	ret = z_pend_curr(&lock, key, &sem->wait_q, timeout);

out:
	SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_sem, take, sem, timeout, ret);

	return ret;
}

void z_impl_k_sem_reset(struct k_sem *sem)
{
	struct k_thread *thread;
	k_spinlock_key_t key = k_spin_lock(&lock);

	while (true) {
		thread = z_unpend_first_thread(&sem->wait_q);
		if (thread == NULL) {
			break;
		}
		arch_thread_return_value_set(thread, -EAGAIN);
		z_ready_thread(thread);
	}
	sem->count = 0;

	SYS_PORT_TRACING_OBJ_FUNC(k_sem, reset, sem);

	handle_poll_events(sem);

	z_reschedule(&lock, key);
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_k_sem_take(struct k_sem *sem, k_timeout_t timeout)
{
	K_OOPS(K_SYSCALL_OBJ(sem, K_OBJ_SEM));
	return z_impl_k_sem_take(sem, timeout);
}
#include <zephyr/syscalls/k_sem_take_mrsh.c>

static inline void z_vrfy_k_sem_reset(struct k_sem *sem)
{
	K_OOPS(K_SYSCALL_OBJ(sem, K_OBJ_SEM));
	z_impl_k_sem_reset(sem);
}
#include <zephyr/syscalls/k_sem_reset_mrsh.c>

static inline unsigned int z_vrfy_k_sem_count_get(struct k_sem *sem)
{
	K_OOPS(K_SYSCALL_OBJ(sem, K_OBJ_SEM));
	return z_impl_k_sem_count_get(sem);
}
#include <zephyr/syscalls/k_sem_count_get_mrsh.c>

#endif /* CONFIG_USERSPACE */

#ifdef CONFIG_OBJ_CORE_SEM
static int init_sem_obj_core_list(void)
{
	/* Initialize semaphore object type */

	z_obj_type_init(&obj_type_sem, K_OBJ_TYPE_SEM_ID,
			offsetof(struct k_sem, obj_core));

	/* Initialize and link statically defined semaphores */

	STRUCT_SECTION_FOREACH(k_sem, sem) {
		k_obj_core_init_and_link(K_OBJ_CORE(sem), &obj_type_sem);
	}

	return 0;
}

SYS_INIT(init_sem_obj_core_list, PRE_KERNEL_1,
	 CONFIG_KERNEL_INIT_PRIORITY_OBJECTS);
#endif /* CONFIG_OBJ_CORE_SEM */
