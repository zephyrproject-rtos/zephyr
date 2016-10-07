/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _sched__h_
#define _sched__h_

#include <kernel.h>
#include <nano_private.h>
#include <atomic.h>
#include <misc/dlist.h>

extern k_tid_t const _main_thread;
extern k_tid_t const _idle_thread;

extern void _add_thread_to_ready_q(struct k_thread *thread);
extern void _remove_thread_from_ready_q(struct k_thread *thread);
extern void _reschedule_threads(int key);
extern void k_sched_unlock(void);
extern void _pend_thread(struct k_thread *thread,
			 _wait_q_t *wait_q, int32_t timeout);
extern void _pend_current_thread(_wait_q_t *wait_q, int32_t timeout);
extern void _move_thread_to_end_of_prio_q(struct k_thread *thread);
extern struct k_thread *_get_next_ready_thread(void);
extern int __must_switch_threads(void);
extern int32_t _ms_to_ticks(int32_t ms);

/*
 * The _is_prio_higher family: I created this because higher priorities are
 * lower numerically and I always found somewhat confusing seeing, e.g.:
 *
 *   if (t1.prio < t2.prio) /# is t1's priority higher then t2's priority ? #/
 *
 * in code. And the fact that most of the time that kind of code has this
 * exact comment warrants a function where it is embedded in the name.
 *
 * IMHO, feel free to remove them and do the comparison directly if this feels
 * like overkill.
 */

static inline int _is_prio1_higher_than_prio2(int prio1, int prio2)
{
	return prio1 < prio2;
}

static inline int _is_prio_higher(int prio, int test_prio)
{
	return _is_prio1_higher_than_prio2(prio, test_prio);
}

static inline int _is_t1_higher_prio_than_t2(struct k_thread *t1,
					     struct k_thread *t2)
{
	return _is_prio1_higher_than_prio2(t1->prio, t2->prio);
}

static inline int _is_higher_prio_than_current(struct k_thread *thread)
{
	return _is_t1_higher_prio_than_t2(thread, _nanokernel.current);
}

/* is thread currenlty cooperative ? */
static inline int _is_coop(struct k_thread *thread)
{
	return thread->prio < 0;
}

/* is thread currently preemptible ? */
static inline int _is_preempt(struct k_thread *thread)
{
	return !_is_coop(thread) && !atomic_get(&thread->sched_locked);
}

/* is current thread preemptible and we are not running in ISR context */
static inline int _is_current_execution_context_preemptible(void)
{
	return !_is_in_isr() && _is_preempt(_nanokernel.current);
}

/* find out if priority is under priority inheritance ceiling */
static inline int _is_under_prio_ceiling(int prio)
{
	return prio >= CONFIG_PRIORITY_CEILING;
}

/*
 * Find out what priority to set a thread to taking the prio ceiling into
 * consideration.
 */
static inline int _get_new_prio_with_ceiling(int prio)
{
	return _is_under_prio_ceiling(prio) ? prio : CONFIG_PRIORITY_CEILING;
}

/* find out the prio bitmap index for a given prio */
static inline int _get_ready_q_prio_bmap_index(int prio)
{
	return (prio + CONFIG_NUM_COOP_PRIORITIES) >> 5;
}

/* find out the prio bit for a given prio */
static inline int _get_ready_q_prio_bit(int prio)
{
	return (1 << ((prio + CONFIG_NUM_COOP_PRIORITIES) & 0x1f));
}

/* find out the ready queue array index for a given prio */
static inline int _get_ready_q_q_index(int prio)
{
	return prio + CONFIG_NUM_COOP_PRIORITIES;
}

#if (K_NUM_PRIORITIES > 32)
	#error not supported yet
#endif

/* find out the currently highest priority where a thread is ready to run */
/* interrupts must be locked */
static inline int _get_highest_ready_prio(void)
{
	uint32_t ready = _nanokernel.ready_q.prio_bmap[0];

	return find_lsb_set(ready) - 1 - CONFIG_NUM_COOP_PRIORITIES;
}

/*
 * Checks if current thread must be context-switched out. The caller must
 * already know that the execution context is a thread.
 */
static inline int _must_switch_threads(void)
{
	return _is_preempt(_current) && __must_switch_threads();
}

/*
 * Application API.
 *
 * lock the scheduler: prevents another thread from preempting the current one
 * except if the current thread does an operation that causes it to pend
 *
 * Can be called recursively.
 */
static inline void k_sched_lock(void)
{
	__ASSERT(!_is_in_isr(), "");

	atomic_inc(&_nanokernel.current->sched_locked);

	K_DEBUG("scheduler locked (%p:%d)\n",
		_current, _current->sched_locked);
}

/**
 * @brief Unlock the scheduler but do NOT reschedule
 *
 * It is incumbent upon the caller to ensure that the reschedule occurs
 * sometime after the scheduler is unlocked.
 */
static inline void _sched_unlock_no_reschedule(void)
{
	__ASSERT(!_is_in_isr(), "");

	atomic_dec(&_nanokernel.current->sched_locked);
}

static inline void _set_thread_states(struct k_thread *thread, uint32_t states)
{
	thread->flags |= states;
}

static inline void _reset_thread_states(struct k_thread *thread,
					uint32_t states)
{
	thread->flags &= ~states;
}

/* mark a thread as being suspended */
static inline void _mark_thread_as_suspended(struct k_thread *thread)
{
	thread->flags |= K_SUSPENDED;
}

/* mark a thread as not being suspended */
static inline void _mark_thread_as_not_suspended(struct k_thread *thread)
{
	thread->flags &= ~K_SUSPENDED;
}

/* mark a thread as being in the timer queue */
static inline void _mark_thread_as_timing(struct k_thread *thread)
{
	thread->flags |= K_TIMING;
}

/* mark a thread as not being in the timer queue */
static inline void _mark_thread_as_not_timing(struct k_thread *thread)
{
	thread->flags &= ~K_TIMING;
}

/* check if a thread is on the timer queue */
static inline int _is_thread_timing(struct k_thread *thread)
{
	return !!(thread->flags & K_TIMING);
}

static inline int _has_thread_started(struct k_thread *thread)
{
	return !(thread->flags & K_PRESTART);
}

/* check if a thread is ready */
static inline int _is_thread_ready(struct k_thread *thread)
{
	return (thread->flags & K_EXECUTION_MASK) == K_READY;
}

/* mark a thread as pending in its TCS */
static inline void _mark_thread_as_pending(struct k_thread *thread)
{
	thread->flags |= K_PENDING;
}

/* mark a thread as not pending in its TCS */
static inline void _mark_thread_as_not_pending(struct k_thread *thread)
{
	thread->flags &= ~K_PENDING;
}

/* check if a thread is pending */
static inline int _is_thread_pending(struct k_thread *thread)
{
	return !!(thread->flags & K_PENDING);
}

/*
 * Mark the thread as not being in the timer queue. If this makes it ready,
 * then add it to the ready queue according to its priority.
 */
/* must be called with interrupts locked */
static inline void _ready_thread(struct k_thread *thread)
{
	__ASSERT(_is_prio_higher(thread->prio, K_LOWEST_THREAD_PRIO) ||
		 ((thread->prio == K_LOWEST_THREAD_PRIO) &&
		  (thread == _idle_thread)),
		 "thread %p prio too low (is %d, cannot be lower than %d)",
		 thread, thread->prio,
		 thread == _idle_thread ? K_LOWEST_THREAD_PRIO :
					  K_LOWEST_APPLICATION_THREAD_PRIO);

	__ASSERT(!_is_prio_higher(thread->prio, K_HIGHEST_THREAD_PRIO),
		 "thread %p prio too high (id %d, cannot be higher than %d)",
		 thread, thread->prio, K_HIGHEST_THREAD_PRIO);

	/* K_PRESTART is needed to handle the start-with-delay case */
	_reset_thread_states(thread, K_TIMING|K_PRESTART);

	if (_is_thread_ready(thread)) {
		_add_thread_to_ready_q(thread);
	}
}

/**
 * @brief Mark a thread as started
 *
 * This routine must be called with interrupts locked.
 */
static inline void _mark_thread_as_started(struct k_thread *thread)
{
	thread->flags &= ~K_PRESTART;
}

/**
 * @brief Mark thread as dead
 *
 * This routine must be called with interrupts locked.
 */
static inline void _mark_thread_as_dead(struct k_thread *thread)
{
	thread->flags |= K_DEAD;
}

/*
 * Set a thread's priority. If the thread is ready, place it in the correct
 * queue.
 */
/* must be called with interrupts locked */
static inline void _thread_priority_set(struct k_thread *thread, int prio)
{
	if (_is_thread_ready(thread)) {
		_remove_thread_from_ready_q(thread);
		thread->prio = prio;
		_add_thread_to_ready_q(thread);
	} else {
		thread->prio = prio;
	}
}

/* check if thread is a thread pending on a particular wait queue */
static inline struct k_thread *_peek_first_pending_thread(_wait_q_t *wait_q)
{
	return (struct k_thread *)sys_dlist_peek_head(wait_q);
}

/* unpend the first thread from a wait queue */
static inline struct k_thread *_unpend_first_thread(_wait_q_t *wait_q)
{
	struct k_thread *thread = (struct k_thread *)sys_dlist_get(wait_q);

	if (thread) {
		_mark_thread_as_not_pending(thread);
	}

	return thread;
}

/* Unpend a thread from the wait queue it is on. Thread must be pending. */
/* must be called with interrupts locked */
static inline void _unpend_thread(struct k_thread *thread)
{
	__ASSERT(thread->flags & K_PENDING, "");

	sys_dlist_remove(&thread->k_q_node);
	_mark_thread_as_not_pending(thread);
}

#endif /* _sched__h_ */
