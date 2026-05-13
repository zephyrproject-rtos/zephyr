/*
 * Copyright (c) 2016-2017 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_KERNEL_INCLUDE_KSCHED_H_
#define ZEPHYR_KERNEL_INCLUDE_KSCHED_H_

#include <zephyr/kernel_structs.h>
#include <kernel_internal.h>
#include <timeout_q.h>
#include <kthread.h>
#include <zephyr/tracing/tracing.h>
#include <stdbool.h>
#include <priority_q.h>

BUILD_ASSERT(K_LOWEST_APPLICATION_THREAD_PRIO
	     >= K_HIGHEST_APPLICATION_THREAD_PRIO);

#ifdef CONFIG_MULTITHREADING
#define Z_VALID_PRIO(prio, entry_point)				     \
	(((prio) == K_IDLE_PRIO && z_is_idle_thread_entry(entry_point)) || \
	 ((K_LOWEST_APPLICATION_THREAD_PRIO			     \
	   >= K_HIGHEST_APPLICATION_THREAD_PRIO)		     \
	  && (prio) >= K_HIGHEST_APPLICATION_THREAD_PRIO	     \
	  && (prio) <= K_LOWEST_APPLICATION_THREAD_PRIO))

#define Z_ASSERT_VALID_PRIO(prio, entry_point) do { \
	__ASSERT(Z_VALID_PRIO((prio), (entry_point)), \
		 "invalid priority (%d); allowed range: %d to %d", \
		 (prio), \
		 K_LOWEST_APPLICATION_THREAD_PRIO, \
		 K_HIGHEST_APPLICATION_THREAD_PRIO); \
	} while (false)
#else
#define Z_VALID_PRIO(prio, entry_point) ((prio) == -1)
#define Z_ASSERT_VALID_PRIO(prio, entry_point) __ASSERT((prio) == -1, "")
#endif /* CONFIG_MULTITHREADING */

#if (CONFIG_MP_MAX_NUM_CPUS == 1)
#define LOCK_SCHED_SPINLOCK
#else
#define LOCK_SCHED_SPINLOCK   K_SPINLOCK(&_sched_spinlock)
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern struct k_spinlock _sched_spinlock;

extern struct k_thread _thread_dummy;

/**
 * @brief Unpend thread, do not abort its timeout (if it exists).
 */
void z_unpend_thread_no_timeout(struct k_thread *thread);

/**
 * @brief Unpend thread and abort its timeout (if it exists).
 */
void z_unpend_thread(struct k_thread *thread);
struct k_thread *z_unpend1_no_timeout(_wait_q_t *wait_q);
int z_pend_curr(struct k_spinlock *lock, k_spinlock_key_t key,
	       _wait_q_t *wait_q, k_timeout_t timeout);
void z_pend_thread(struct k_thread *thread, _wait_q_t *wait_q,
		   k_timeout_t timeout);
void z_reschedule(struct k_spinlock *lock, k_spinlock_key_t key);
void z_reschedule_irqlock(uint32_t key);
int z_unpend_all(_wait_q_t *wait_q);
bool z_thread_prio_set(struct k_thread *thread, int prio);
void *z_get_next_switch_handle(void *interrupted);
void z_thread_suspend_current(struct k_thread *thread);

/* Wrapper around z_get_next_switch_handle() for the benefit of
 * non-SMP platforms that always pass a NULL interrupted handle.
 * Exposes the (extremely) common early exit case in a context that
 * can be (much) better optimized in surrounding C code.  Takes a
 * _current pointer that the outer scope surely already has to avoid
 * having to refetch it across a lock.
 *
 * Mystic arts for cycle nerds, basically.  Replace with
 * z_get_next_switch_handle() if this becomes a maintenance hassle.
 */
static ALWAYS_INLINE void *z_sched_next_handle(struct k_thread *curr)
{
#ifndef CONFIG_SMP
	if (curr == _kernel.ready_q.cache) {
		return NULL;
	}
#endif
	return z_get_next_switch_handle(NULL);
}

void z_sched_start(struct k_thread *thread);
void z_ready_thread(struct k_thread *thread);
struct k_thread *z_swap_next_thread(void);
void move_current_to_end_of_prio_q(void);

/*
 * Internal scheduler functions exposed for use by thread lifecycle code
 * (thread.c). These operate under _sched_spinlock and must not be called
 * without holding it.
 */

/**
 * @brief Halt a thread, suspending or terminating it.
 *
 * Shared implementation for k_thread_suspend() and k_thread_abort(). The
 * caller must hold _sched_spinlock; this function releases it before
 * returning (possibly after a context switch).
 *
 * @param thread  Thread to halt.
 * @param key     Scheduler spinlock key held by the caller.
 * @param terminate true to abort (kill) the thread, false to suspend it.
 */
void z_thread_halt(struct k_thread *thread, k_spinlock_key_t key, bool terminate);

/**
 * @brief Ready a thread while the scheduler spinlock is already held.
 *
 * Equivalent to the internal ready_thread() helper. Callers must hold
 * _sched_spinlock.
 *
 * @param thread Thread to make ready.
 */
void z_sched_ready_locked(struct k_thread *thread);

/**
 * @brief Add a thread to a wait queue while the scheduler spinlock is held.
 *
 * Moves the thread out of the run queue and onto the specified wait queue.
 * Callers must hold _sched_spinlock.
 *
 * @param thread  Thread to pend.
 * @param wait_q  Wait queue to add the thread to (may be NULL).
 */
void z_sched_add_to_waitq_locked(struct k_thread *thread, _wait_q_t *wait_q);

/**
 * @brief Remove a thread from the run queue (scheduler spinlock must be held).
 *
 * Called by sleep.c to unready the current thread before arming its wakeup
 * timeout.  Callers must already hold _sched_spinlock.
 *
 * @param thread Thread to remove from the run queue.
 */
void z_sched_unready_locked(struct k_thread *thread);

/**
 * @brief Yield the current thread's remaining time slice.
 *
 * Moves _current to the end of its priority group in the run queue, updates
 * the scheduler cache, and context-switches away.  Equivalent to the body
 * of k_yield(); exposed so that thread.c can implement k_yield() without
 * depending on sched.c internals.
 */
void z_sched_yield(void);

static inline void z_reschedule_unlocked(void)
{
	(void) z_reschedule_irqlock(arch_irq_lock());
}

static inline bool z_is_under_prio_ceiling(int prio)
{
	return prio >= CONFIG_PRIORITY_CEILING;
}

static inline int z_get_new_prio_with_ceiling(int prio)
{
	return z_is_under_prio_ceiling(prio) ? prio : CONFIG_PRIORITY_CEILING;
}

static inline bool z_is_prio1_higher_than_or_equal_to_prio2(int prio1, int prio2)
{
	return prio1 <= prio2;
}

static inline bool z_is_prio_higher_or_equal(int prio1, int prio2)
{
	return z_is_prio1_higher_than_or_equal_to_prio2(prio1, prio2);
}

static inline bool z_is_prio1_lower_than_or_equal_to_prio2(int prio1, int prio2)
{
	return prio1 >= prio2;
}

static inline bool z_is_prio1_higher_than_prio2(int prio1, int prio2)
{
	return prio1 < prio2;
}

static inline bool z_is_prio_higher(int prio, int test_prio)
{
	return z_is_prio1_higher_than_prio2(prio, test_prio);
}

static inline bool z_is_prio_lower_or_equal(int prio1, int prio2)
{
	return z_is_prio1_lower_than_or_equal_to_prio2(prio1, prio2);
}

static inline bool _is_valid_prio(int prio, k_thread_entry_t entry_point)
{
	if ((prio == K_IDLE_PRIO) && z_is_idle_thread_entry(entry_point)) {
		return true;
	}

	if (!z_is_prio_higher_or_equal(prio,
				       K_LOWEST_APPLICATION_THREAD_PRIO)) {
		return false;
	}

	if (!z_is_prio_lower_or_equal(prio,
				      K_HIGHEST_APPLICATION_THREAD_PRIO)) {
		return false;
	}

	return true;
}

static ALWAYS_INLINE _wait_q_t *pended_on_thread(struct k_thread *thread)
{
	__ASSERT_NO_MSG(thread->base.pended_on);

	return thread->base.pended_on;
}


static inline void unpend_thread_no_timeout(struct k_thread *thread)
{
	_priq_wait_remove(&pended_on_thread(thread)->waitq, thread);
	z_mark_thread_as_not_pending(thread);
	thread->base.pended_on = NULL;
}

/*
 * In a multiprocessor system, z_unpend_first_thread() must lock the scheduler
 * spinlock _sched_spinlock. However, in a uniprocessor system, that is not
 * necessary as the caller has already taken precautions (in the form of
 * locking interrupts).
 */
static ALWAYS_INLINE struct k_thread *z_unpend_first_thread(_wait_q_t *wait_q)
{
	struct k_thread *thread = NULL;

	__ASSERT_EVAL(, int key = arch_irq_lock(); arch_irq_unlock(key),
		      !arch_irq_unlocked(key), "");

	LOCK_SCHED_SPINLOCK {
		thread = _priq_wait_best(&wait_q->waitq);
		if (unlikely(thread != NULL)) {
			unpend_thread_no_timeout(thread);
			z_abort_thread_timeout(thread);
		}
	}

	return thread;
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_KERNEL_INCLUDE_KSCHED_H_ */
