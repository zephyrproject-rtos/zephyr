/*
 * Copyright (c) 2016-2017 Wind River Systems, Inc.
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_KERNEL_INCLUDE_THREAD_H_
#define ZEPHYR_KERNEL_INCLUDE_THREAD_H_

#include <zephyr/kernel.h>
#include <kernel_internal.h>
#include <timeout_q.h>


#define Z_STATE_STR_DUMMY       "dummy"
#define Z_STATE_STR_PENDING     "pending"
#define Z_STATE_STR_SLEEPING    "sleeping"
#define Z_STATE_STR_DEAD        "dead"
#define Z_STATE_STR_SUSPENDED   "suspended"
#define Z_STATE_STR_ABORTING    "aborting"
#define Z_STATE_STR_SUSPENDING  "suspending"
#define Z_STATE_STR_QUEUED      "queued"

#ifdef CONFIG_THREAD_MONITOR
/* This lock protects the linked list of active threads; i.e. the
 * initial _kernel.threads pointer and the linked list made up of
 * thread->next_thread (until NULL)
 */
extern struct k_spinlock z_thread_monitor_lock;
#endif /* CONFIG_THREAD_MONITOR */

#ifdef CONFIG_MULTITHREADING
extern struct k_thread z_idle_threads[CONFIG_MP_MAX_NUM_CPUS];
#endif /* CONFIG_MULTITHREADING */

void idle(void *unused1, void *unused2, void *unused3);

/* clean up when a thread is aborted */

#if defined(CONFIG_THREAD_MONITOR)
void z_thread_monitor_exit(struct k_thread *thread);
#else
#define z_thread_monitor_exit(thread) \
	do {/* nothing */    \
	} while (false)
#endif /* CONFIG_THREAD_MONITOR */

void z_thread_abort(struct k_thread *thread);

static inline void thread_schedule_new(struct k_thread *thread, k_timeout_t delay)
{
#ifdef CONFIG_SYS_CLOCK_EXISTS
	if (K_TIMEOUT_EQ(delay, K_NO_WAIT)) {
		k_thread_start(thread);
	} else {
		z_add_thread_timeout(thread, delay);
	}
#else
	ARG_UNUSED(delay);
	k_thread_start(thread);
#endif /* CONFIG_SYS_CLOCK_EXISTS */
}

static inline int thread_is_preemptible(struct k_thread *thread)
{
	/* explanation in kernel_struct.h */
	return thread->base.preempt <= _PREEMPT_THRESHOLD;
}


static inline int thread_is_metairq(struct k_thread *thread)
{
#if CONFIG_NUM_METAIRQ_PRIORITIES > 0
	return (thread->base.prio - K_HIGHEST_THREAD_PRIO)
		< CONFIG_NUM_METAIRQ_PRIORITIES;
#else
	ARG_UNUSED(thread);
	return 0;
#endif /* CONFIG_NUM_METAIRQ_PRIORITIES */
}

#ifdef CONFIG_ASSERT
static inline bool is_thread_dummy(struct k_thread *thread)
{
	return (thread->base.thread_state & _THREAD_DUMMY) != 0U;
}
#endif /* CONFIG_ASSERT */


static inline bool z_is_thread_suspended(struct k_thread *thread)
{
	return (thread->base.thread_state & _THREAD_SUSPENDED) != 0U;
}

static inline bool z_is_thread_pending(struct k_thread *thread)
{
	return (thread->base.thread_state & _THREAD_PENDING) != 0U;
}

static inline bool z_is_thread_prevented_from_running(struct k_thread *thread)
{
	uint8_t state = thread->base.thread_state;

	return (state & (_THREAD_PENDING | _THREAD_SLEEPING | _THREAD_DEAD |
			 _THREAD_DUMMY | _THREAD_SUSPENDED)) != 0U;
}

static inline bool z_is_thread_timeout_active(struct k_thread *thread)
{
	return !z_is_inactive_timeout(&thread->base.timeout);
}

static inline bool z_is_thread_ready(struct k_thread *thread)
{
	return !z_is_thread_prevented_from_running(thread);
}

static inline bool z_is_thread_state_set(struct k_thread *thread, uint32_t state)
{
	return (thread->base.thread_state & state) != 0U;
}

static inline bool z_is_thread_queued(struct k_thread *thread)
{
	return z_is_thread_state_set(thread, _THREAD_QUEUED);
}

static inline void z_mark_thread_as_queued(struct k_thread *thread)
{
	thread->base.thread_state |= _THREAD_QUEUED;
}

static inline void z_mark_thread_as_not_queued(struct k_thread *thread)
{
	thread->base.thread_state &= ~_THREAD_QUEUED;
}

static inline void z_mark_thread_as_suspended(struct k_thread *thread)
{
	thread->base.thread_state |= _THREAD_SUSPENDED;

	SYS_PORT_TRACING_FUNC(k_thread, sched_suspend, thread);
}

static inline void z_mark_thread_as_not_suspended(struct k_thread *thread)
{
	thread->base.thread_state &= ~_THREAD_SUSPENDED;

	SYS_PORT_TRACING_FUNC(k_thread, sched_resume, thread);
}

static inline void z_mark_thread_as_pending(struct k_thread *thread)
{
	thread->base.thread_state |= _THREAD_PENDING;
}

static inline void z_mark_thread_as_not_pending(struct k_thread *thread)
{
	thread->base.thread_state &= ~_THREAD_PENDING;
}

static inline bool z_is_thread_sleeping(struct k_thread *thread)
{
	return (thread->base.thread_state & _THREAD_SLEEPING) != 0U;
}

static inline void z_mark_thread_as_sleeping(struct k_thread *thread)
{
	thread->base.thread_state |= _THREAD_SLEEPING;
}

static inline void z_mark_thread_as_not_sleeping(struct k_thread *thread)
{
	thread->base.thread_state &= ~_THREAD_SLEEPING;
}

/*
 * This function tags the current thread as essential to system operation.
 * Exceptions raised by this thread will be treated as a fatal system error.
 */
static inline void z_thread_essential_set(struct k_thread *thread)
{
	thread->base.user_options |= K_ESSENTIAL;
}

/*
 * This function tags the current thread as not essential to system operation.
 * Exceptions raised by this thread may be recoverable.
 * (This is the default tag for a thread.)
 */
static inline void z_thread_essential_clear(struct k_thread *thread)
{
	thread->base.user_options &= ~K_ESSENTIAL;
}

/*
 * This routine indicates if the current thread is an essential system thread.
 *
 * Returns true if current thread is essential, false if it is not.
 */
static inline bool z_is_thread_essential(struct k_thread *thread)
{
	return (thread->base.user_options & K_ESSENTIAL) == K_ESSENTIAL;
}


static ALWAYS_INLINE bool should_preempt(struct k_thread *thread,
					 int preempt_ok)
{
	/* Preemption is OK if it's being explicitly allowed by
	 * software state (e.g. the thread called k_yield())
	 */
	if (preempt_ok != 0) {
		return true;
	}

	__ASSERT(_current != NULL, "");

	/* Or if we're pended/suspended/dummy (duh) */
	if (z_is_thread_prevented_from_running(_current)) {
		return true;
	}

	/* Otherwise we have to be running a preemptible thread or
	 * switching to a metairq
	 */
	if (thread_is_preemptible(_current) || thread_is_metairq(thread)) {
		return true;
	}

	/* Edge case on ARM where a thread can be pended out of an
	 * interrupt handler before the "synchronous" swap starts
	 * context switching.  Platforms with atomic swap can never
	 * hit this.
	 */
	if (IS_ENABLED(CONFIG_SWAP_NONATOMIC)
	    && z_is_thread_timeout_active(thread)) {
		return true;
	}

	return false;
}


static inline bool z_is_idle_thread_entry(k_thread_entry_t entry_point)
{
	return entry_point == idle;
}

static inline bool z_is_idle_thread_object(struct k_thread *thread)
{
#ifdef CONFIG_MULTITHREADING
#ifdef CONFIG_SMP
	return thread->base.is_idle;
#else
	return thread == &z_idle_threads[0];
#endif /* CONFIG_SMP */
#else
	return false;
#endif /* CONFIG_MULTITHREADING */
}


#endif /* ZEPHYR_KERNEL_INCLUDE_THREAD_H_ */
