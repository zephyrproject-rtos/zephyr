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
#include <wait_q.h>
#include <sys/dlist.h>
#include <ksched.h>
#include <kswap.h>
#include <init.h>
#include <syscall_handler.h>
#include <tracing/tracing.h>
#include <sys/check.h>

/* We use a system-wide lock to synchronize semaphores, which has
 * unfortunate performance impact vs. using a per-object lock
 * (semaphores are *very* widely used).  But per-object locks require
 * significant extra RAM.  A properly spin-aware semaphore
 * implementation would spin on atomic access to the count variable,
 * and not a spinlock per se.  Useful optimization for the future...
 */
#if defined(CONFIG_SEM_IMPL_SINGLE_SPINLOCK)
static struct k_spinlock lock;
#define LOCK_FOR_SEM(sem) (&lock)
#elif defined(CONFIG_SEM_IMPL_SPINLOCK_PER_SEMAPHORE)
#define LOCK_FOR_SEM(sem) (&sem->lock)
#endif

#if defined(CONFIG_POLL) && defined(CONFIG_SEM_IMPL_ATOMIC)
static struct k_spinlock poll_lock;
#endif

#ifdef CONFIG_OBJECT_TRACING

struct k_sem *_trace_list_k_sem;

/*
 * Complete initialization of statically defined semaphores.
 */
static int init_sem_module(const struct device *dev)
{
	ARG_UNUSED(dev);

	Z_STRUCT_SECTION_FOREACH(k_sem, sem) {
		SYS_TRACING_OBJ_INIT(k_sem, sem);
	}
	return 0;
}

SYS_INIT(init_sem_module, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_OBJECTS);

#endif /* CONFIG_OBJECT_TRACING */

int z_impl_k_sem_init(struct k_sem *sem, unsigned int initial_count,
		      unsigned int limit)
{
	/*
	 * Limit cannot be zero and count cannot be greater than limit
	 */
	CHECKIF(limit == 0U || limit > K_SEM_MAX_LIMIT || initial_count > limit) {
		return -EINVAL;
	}

	sem->count = initial_count;
	sem->limit = limit;
	sys_trace_semaphore_init(sem);
	z_waitq_init(&sem->wait_q);
#if defined(CONFIG_SEM_IMPL_SPINLOCK_PER_SEMAPHORE)
	struct k_spinlock reinit = {};

	sem->lock = reinit;
#endif
#if defined(CONFIG_POLL)
	sys_dlist_init(&sem->poll_events);
#endif

	SYS_TRACING_OBJ_INIT(k_sem, sem);

	z_object_init(sem);
	sys_trace_end_call(SYS_TRACE_ID_SEMA_INIT);

	return 0;
}

#ifdef CONFIG_USERSPACE
int z_vrfy_k_sem_init(struct k_sem *sem, unsigned int initial_count,
		      unsigned int limit)
{
	Z_OOPS(Z_SYSCALL_OBJ_INIT(sem, K_OBJ_SEM));
	return z_impl_k_sem_init(sem, initial_count, limit);
}
#include <syscalls/k_sem_init_mrsh.c>
#endif

static inline void handle_poll_events(struct k_sem *sem)
{
#ifdef CONFIG_POLL
#ifdef CONFIG_SEM_IMPL_ATOMIC
	/* The polling API requires a lock for now. */
	k_spinlock_key_t key = k_spin_lock(&poll_lock);

#endif
	z_handle_obj_poll_events(&sem->poll_events, K_POLL_STATE_SEM_AVAILABLE);
#ifdef CONFIG_SEM_IMPL_ATOMIC
	k_spin_unlock(&poll_lock, key);
#endif
#else
	ARG_UNUSED(sem);
#endif
}

#ifndef CONFIG_SEM_IMPL_ATOMIC
/* Also see sem_spinlock.tla */
static inline void sem_give(struct k_sem *sem)
{
	struct k_thread *thread;
	k_spinlock_key_t key = k_spin_lock(LOCK_FOR_SEM(sem));

	thread = z_unpend_first_thread(&sem->wait_q);

	if (thread != NULL) {
		arch_thread_return_value_set(thread, 0);
		z_ready_thread(thread);
	} else {
		sem->count += (sem->count != sem->limit) ? 1U : 0U;
		handle_poll_events(sem);
	}

	z_reschedule(LOCK_FOR_SEM(sem), key);
}

static inline int sem_take(struct k_sem *sem, k_timeout_t timeout)
{
	k_spinlock_key_t key = k_spin_lock(LOCK_FOR_SEM(sem));

	if (likely(sem->count > 0U)) {
		sem->count--;
		k_spin_unlock(LOCK_FOR_SEM(sem), key);
		return 0;
	}

	if (K_TIMEOUT_EQ(timeout, K_NO_WAIT)) {
		k_spin_unlock(LOCK_FOR_SEM(sem), key);
		return -EBUSY;
	}

	return z_pend_curr(LOCK_FOR_SEM(sem), key, &sem->wait_q, timeout);
}

static inline void sem_reset(struct k_sem *sem)
{
	struct k_thread *thread;
	k_spinlock_key_t key = k_spin_lock(LOCK_FOR_SEM(sem));

	while (true) {
		thread = z_unpend_first_thread(&sem->wait_q);
		if (thread == NULL) {
			break;
		}
		arch_thread_return_value_set(thread, -EAGAIN);
		z_ready_thread(thread);
	}
	sem->count = 0;
	handle_poll_events(sem);

	k_spin_unlock(LOCK_FOR_SEM(sem), key);
	/* z_reschedule(LOCK_FOR_SEM(sem), key); */
}

#else
/* Also see sem_atomic.tla */
static inline void sem_give(struct k_sem *sem)
{
	/* compiler wasn't hoisting this out of the loop. */
	atomic_t sem_limit = sem->limit;
	struct k_thread *thread;
	unsigned int key = arch_irq_lock();

	do {
		atomic_t count = atomic_get(&sem->count);

		if ((count >= sem_limit) ||
			(count >= 0 && atomic_cas(&sem->count, count, count + 1))) {
			handle_poll_events(sem);
			arch_irq_unlock(key);
			goto out;
		}

		thread = z_unpend_first_thread(&sem->wait_q);
	} while (thread == NULL);

	atomic_inc(&sem->count);
	arch_thread_return_value_set(thread, 0);
	z_ready_thread(thread);
	z_reschedule_no_lock(key);
out:
	return;
}

static int sem_take(struct k_sem *sem, k_timeout_t timeout)
{
	int ret;
	unsigned int key = arch_irq_lock();

	if (K_TIMEOUT_EQ(timeout, K_NO_WAIT)) {
		atomic_t count;
		/* this doesn't truly need to be duplicated, but
		 * we want as little as possible in the CASloop.
		 */
		do {
			count = atomic_get(&sem->count);
			if (count <= 0) {
				ret = -EBUSY;
				goto out;
			}
		} while (!atomic_cas(&sem->count, count, count - 1));
		ret = 0;
		goto out;
	}

	if (atomic_dec(&sem->count) > 0) {
		ret = 0;
		goto out;
	}
	ret = z_pend_curr_with_arch_lock(&sem->wait_q, timeout);

	if (ret == -EAGAIN) {
		/* We timed out, and now the semaphore value
		 * is off by one because we're not in wait_q.
		 * Give back our semaphore reservation.
		 */
		/* There is a lingering concern here with thread priorities.
		 * If a bunch of low-priority threads do sem_take with a timeout,
		 * and have enough cpu time to take the semaphore but not enough
		 * to return it later, a high-priority thread may not be able
		 * to take the semaphore until one of said low-priority threads
		 * run. That being said, if you want priority inheritance you should
		 * probably be using a mutex instead...
		 * If this is an issue, we can fix it (at a performance penalty,
		 * especially with the current APIs as there's no way to share the lock
		 * call with the z_pend_curr call, and we'd likely be rescheduling twice
		 * for much the same reason) by bumping this thread's priority for the
		 * z_pend_curr call above.
		 */
		/*
		 * Perhaps counterintuitively, this
		 * should _not_ do a normal semaphore give.
		 */
		atomic_inc(&sem->count);
	}
out:
	arch_irq_unlock(key);
	return ret;
}

static inline void sem_reset(struct k_sem *sem)
{
	unsigned int key = arch_irq_lock();

	while (true) {
		atomic_t count = atomic_get(&sem->count);

		if (count == 0) {
			break;
		}
		if (count > 0) {
			if (!atomic_cas(&sem->count, count, 0)) {
				continue;
			}
			break;
		}
		struct k_thread *thread = z_unpend_first_thread(&sem->wait_q);

		if (thread != NULL) {
			arch_thread_return_value_set(thread, -EAGAIN);
			z_ready_thread(thread);
			/* cannot overflow */
			atomic_inc(&sem->count);
		}
	}

	handle_poll_events(sem);
	/* z_reschedule_no_lock(key); */
	arch_irq_unlock(key);
}
#endif

void z_impl_k_sem_give(struct k_sem *sem)
{
	sys_trace_semaphore_give(sem);

	sem_give(sem);

	sys_trace_end_call(SYS_TRACE_ID_SEMA_GIVE);
}

#ifdef CONFIG_USERSPACE
static inline void z_vrfy_k_sem_give(struct k_sem *sem)
{
	Z_OOPS(Z_SYSCALL_OBJ(sem, K_OBJ_SEM));
	z_impl_k_sem_give(sem);
}
#include <syscalls/k_sem_give_mrsh.c>
#endif

int z_impl_k_sem_take(struct k_sem *sem, k_timeout_t timeout)
{
	int ret;

	__ASSERT(((arch_is_in_isr() == false) ||
		  K_TIMEOUT_EQ(timeout, K_NO_WAIT)), "");
	sys_trace_semaphore_take(sem);
	ret = sem_take(sem, timeout);
	sys_trace_end_call(SYS_TRACE_ID_SEMA_TAKE);
	return ret;
}

void z_impl_k_sem_reset(struct k_sem *sem)
{
	sem_reset(sem);
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_k_sem_take(struct k_sem *sem, k_timeout_t timeout)
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
