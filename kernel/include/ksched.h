/*
 * Copyright (c) 2016-2017 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ksched__h_
#define _ksched__h_

#include <kernel_structs.h>

#ifdef CONFIG_KERNEL_EVENT_LOGGER
#include <logging/kernel_event_logger.h>
#endif /* CONFIG_KERNEL_EVENT_LOGGER */

extern k_tid_t const _main_thread;
extern k_tid_t const _idle_thread;

extern void _add_thread_to_ready_q(struct k_thread *thread);
extern void _remove_thread_from_ready_q(struct k_thread *thread);
extern void _reschedule_threads(int key);
extern void k_sched_unlock(void);
extern void _pend_thread(struct k_thread *thread,
			 _wait_q_t *wait_q, s32_t timeout);
extern void _pend_current_thread(_wait_q_t *wait_q, s32_t timeout);
extern void _move_thread_to_end_of_prio_q(struct k_thread *thread);
extern int __must_switch_threads(void);
extern int _is_thread_time_slicing(struct k_thread *thread);
extern void _update_time_slice_before_swap(void);
#ifdef _NON_OPTIMIZED_TICKS_PER_SEC
extern s32_t _ms_to_ticks(s32_t ms);
#endif
extern void idle(void *, void *, void *);

/* find which one is the next thread to run */
/* must be called with interrupts locked */
static ALWAYS_INLINE struct k_thread *_get_next_ready_thread(void)
{
	return _ready_q.cache;
}

static inline int _is_idle_thread(void *entry_point)
{
	return entry_point == idle;
}

static inline int _is_idle_thread_ptr(k_tid_t thread)
{
	return thread == _idle_thread;
}

#ifdef CONFIG_MULTITHREADING
#define _VALID_PRIO(prio, entry_point) \
	(((prio) == K_IDLE_PRIO && _is_idle_thread(entry_point)) || \
		 (_is_prio_higher_or_equal((prio), \
			K_LOWEST_APPLICATION_THREAD_PRIO) && \
		  _is_prio_lower_or_equal((prio), \
			K_HIGHEST_APPLICATION_THREAD_PRIO)))

#define _ASSERT_VALID_PRIO(prio, entry_point) do { \
	__ASSERT(_VALID_PRIO((prio), (entry_point)), \
		 "invalid priority (%d); allowed range: %d to %d", \
		 (prio), \
		 K_LOWEST_APPLICATION_THREAD_PRIO, \
		 K_HIGHEST_APPLICATION_THREAD_PRIO); \
	} while ((0))
#else
#define _VALID_PRIO(prio, entry_point) ((prio) == -1)
#define _ASSERT_VALID_PRIO(prio, entry_point) __ASSERT((prio) == -1, "")
#endif

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

static inline int _is_prio1_higher_than_or_equal_to_prio2(int prio1, int prio2)
{
	return prio1 <= prio2;
}

static inline int _is_prio_higher_or_equal(int prio1, int prio2)
{
	return _is_prio1_higher_than_or_equal_to_prio2(prio1, prio2);
}

static inline int _is_prio1_higher_than_prio2(int prio1, int prio2)
{
	return prio1 < prio2;
}

static inline int _is_prio_higher(int prio, int test_prio)
{
	return _is_prio1_higher_than_prio2(prio, test_prio);
}

static inline int _is_prio1_lower_than_or_equal_to_prio2(int prio1, int prio2)
{
	return prio1 >= prio2;
}

static inline int _is_prio_lower_or_equal(int prio1, int prio2)
{
	return _is_prio1_lower_than_or_equal_to_prio2(prio1, prio2);
}

static inline int _is_prio1_lower_than_prio2(int prio1, int prio2)
{
	return prio1 > prio2;
}

static inline int _is_prio_lower(int prio1, int prio2)
{
	return _is_prio1_lower_than_prio2(prio1, prio2);
}

static inline int _is_t1_higher_prio_than_t2(struct k_thread *t1,
					     struct k_thread *t2)
{
	return _is_prio1_higher_than_prio2(t1->base.prio, t2->base.prio);
}

static inline int _is_higher_prio_than_current(struct k_thread *thread)
{
	return _is_t1_higher_prio_than_t2(thread, _current);
}

/* is thread currenlty cooperative ? */
static inline int _is_coop(struct k_thread *thread)
{
#if defined(CONFIG_PREEMPT_ENABLED) && defined(CONFIG_COOP_ENABLED)
	return thread->base.prio < 0;
#elif defined(CONFIG_COOP_ENABLED)
	return 1;
#elif defined(CONFIG_PREEMPT_ENABLED)
	return 0;
#else
#error "Impossible configuration"
#endif
}

/* is thread currently preemptible ? */
static inline int _is_preempt(struct k_thread *thread)
{
#ifdef CONFIG_PREEMPT_ENABLED
	/* explanation in kernel_struct.h */
	return thread->base.preempt <= _PREEMPT_THRESHOLD;
#else
	return 0;
#endif
}

/* is current thread preemptible and we are not running in ISR context */
static inline int _is_current_execution_context_preemptible(void)
{
#ifdef CONFIG_PREEMPT_ENABLED
	return !_is_in_isr() && _is_preempt(_current);
#else
	return 0;
#endif
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
	return (prio + _NUM_COOP_PRIO) >> 5;
}

/* find out the prio bit for a given prio */
static inline int _get_ready_q_prio_bit(int prio)
{
	return (1 << ((prio + _NUM_COOP_PRIO) & 0x1f));
}

/* find out the ready queue array index for a given prio */
static inline int _get_ready_q_q_index(int prio)
{
	return prio + _NUM_COOP_PRIO;
}

/* find out the currently highest priority where a thread is ready to run */
/* interrupts must be locked */
static inline int _get_highest_ready_prio(void)
{
	int bitmap = 0;
	u32_t ready_range;

#if (K_NUM_PRIORITIES <= 32)
	ready_range = _ready_q.prio_bmap[0];
#else
	for (;; bitmap++) {

		__ASSERT(bitmap < K_NUM_PRIO_BITMAPS, "prio out-of-range\n");

		if (_ready_q.prio_bmap[bitmap]) {
			ready_range = _ready_q.prio_bmap[bitmap];
			break;
		}
	}
#endif

	int abs_prio = (find_lsb_set(ready_range) - 1) + (bitmap << 5);

	__ASSERT(abs_prio < K_NUM_PRIORITIES, "prio out-of-range\n");

	return abs_prio - _NUM_COOP_PRIO;
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
 * Called directly by other internal kernel code.
 * Exposed to applications via k_sched_lock(), which just calls this
 */
static inline void _sched_lock(void)
{
#ifdef CONFIG_PREEMPT_ENABLED
	__ASSERT(!_is_in_isr(), "");
	__ASSERT(_current->base.sched_locked != 1, "");

	--_current->base.sched_locked;

	compiler_barrier();

	K_DEBUG("scheduler locked (%p:%d)\n",
		_current, _current->base.sched_locked);
#endif
}

/**
 * @brief Unlock the scheduler but do NOT reschedule
 *
 * It is incumbent upon the caller to ensure that the reschedule occurs
 * sometime after the scheduler is unlocked.
 */
static ALWAYS_INLINE void _sched_unlock_no_reschedule(void)
{
#ifdef CONFIG_PREEMPT_ENABLED
	__ASSERT(!_is_in_isr(), "");
	__ASSERT(_current->base.sched_locked != 0, "");

	compiler_barrier();

	++_current->base.sched_locked;
#endif
}

static inline void _set_thread_states(struct k_thread *thread, u32_t states)
{
	thread->base.thread_state |= states;
}

static inline void _reset_thread_states(struct k_thread *thread,
					u32_t states)
{
	thread->base.thread_state &= ~states;
}

static inline int _is_thread_state_set(struct k_thread *thread, u32_t state)
{
	return !!(thread->base.thread_state & state);
}

/* mark a thread as being suspended */
static inline void _mark_thread_as_suspended(struct k_thread *thread)
{
	thread->base.thread_state |= _THREAD_SUSPENDED;
}

/* mark a thread as not being suspended */
static inline void _mark_thread_as_not_suspended(struct k_thread *thread)
{
	thread->base.thread_state &= ~_THREAD_SUSPENDED;
}

static ALWAYS_INLINE int _is_thread_timeout_expired(struct k_thread *thread)
{
#ifdef CONFIG_SYS_CLOCK_EXISTS
	return thread->base.timeout.delta_ticks_from_prev == _EXPIRED;
#else
	return 0;
#endif
}

/* check if a thread is on the timeout queue */
static inline int _is_thread_timeout_active(struct k_thread *thread)
{
#ifdef CONFIG_SYS_CLOCK_EXISTS
	return thread->base.timeout.delta_ticks_from_prev != _INACTIVE;
#else
	return 0;
#endif
}

static inline int _has_thread_started(struct k_thread *thread)
{
	return !(thread->base.thread_state & _THREAD_PRESTART);
}

static inline int _is_thread_prevented_from_running(struct k_thread *thread)
{
	u8_t state = thread->base.thread_state;

	return state & (_THREAD_PENDING | _THREAD_PRESTART | _THREAD_DEAD |
			_THREAD_DUMMY | _THREAD_SUSPENDED);

}

/* check if a thread is ready */
static inline int _is_thread_ready(struct k_thread *thread)
{
	return !(_is_thread_prevented_from_running(thread) ||
		 _is_thread_timeout_active(thread));
}

/* mark a thread as pending in its TCS */
static inline void _mark_thread_as_pending(struct k_thread *thread)
{
	thread->base.thread_state |= _THREAD_PENDING;

#ifdef CONFIG_KERNEL_EVENT_LOGGER_THREAD
	_sys_k_event_logger_thread_pend(thread);
#endif
}

/* mark a thread as not pending in its TCS */
static inline void _mark_thread_as_not_pending(struct k_thread *thread)
{
	thread->base.thread_state &= ~_THREAD_PENDING;
}

/* check if a thread is pending */
static inline int _is_thread_pending(struct k_thread *thread)
{
	return !!(thread->base.thread_state & _THREAD_PENDING);
}

static inline int _is_thread_dummy(struct k_thread *thread)
{
	return _is_thread_state_set(thread, _THREAD_DUMMY);
}

static inline void _mark_thread_as_polling(struct k_thread *thread)
{
	_set_thread_states(thread, _THREAD_POLLING);
}

static inline void _mark_thread_as_not_polling(struct k_thread *thread)
{
	_reset_thread_states(thread, _THREAD_POLLING);
}

static inline int _is_thread_polling(struct k_thread *thread)
{
	return _is_thread_state_set(thread, _THREAD_POLLING);
}

/**
 * @brief Mark a thread as started
 *
 * This routine must be called with interrupts locked.
 */
static inline void _mark_thread_as_started(struct k_thread *thread)
{
	thread->base.thread_state &= ~_THREAD_PRESTART;
}

/*
 * Put the thread in the ready queue according to its priority if it is not
 * blocked for another reason (eg. suspended).
 *
 * Must be called with interrupts locked.
 */
static inline void _ready_thread(struct k_thread *thread)
{
	__ASSERT(_is_prio_higher(thread->base.prio, K_LOWEST_THREAD_PRIO) ||
		 ((thread->base.prio == K_LOWEST_THREAD_PRIO) &&
		  (thread == _idle_thread)),
		 "thread %p prio too low (is %d, cannot be lower than %d)",
		 thread, thread->base.prio,
		 thread == _idle_thread ? K_LOWEST_THREAD_PRIO :
					  K_LOWEST_APPLICATION_THREAD_PRIO);

	__ASSERT(!_is_prio_higher(thread->base.prio, K_HIGHEST_THREAD_PRIO),
		 "thread %p prio too high (id %d, cannot be higher than %d)",
		 thread, thread->base.prio, K_HIGHEST_THREAD_PRIO);

	/* needed to handle the start-with-delay case */
	_mark_thread_as_started(thread);

	if (_is_thread_ready(thread)) {
		_add_thread_to_ready_q(thread);
	}

#ifdef CONFIG_KERNEL_EVENT_LOGGER_THREAD
	_sys_k_event_logger_thread_ready(thread);
#endif
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
		thread->base.prio = prio;
		_add_thread_to_ready_q(thread);
	} else {
		thread->base.prio = prio;
	}
}

/* check if thread is a thread pending on a particular wait queue */
static inline struct k_thread *_peek_first_pending_thread(_wait_q_t *wait_q)
{
	return (struct k_thread *)sys_dlist_peek_head(wait_q);
}

static inline struct k_thread *
_find_first_thread_to_unpend(_wait_q_t *wait_q, struct k_thread *from)
{
#ifdef CONFIG_SYS_CLOCK_EXISTS
	extern volatile int _handling_timeouts;

	if (_handling_timeouts) {
		sys_dlist_t *q = (sys_dlist_t *)wait_q;
		sys_dnode_t *cur = from ? &from->base.k_q_node : NULL;

		/* skip threads that have an expired timeout */
		SYS_DLIST_ITERATE_FROM_NODE(q, cur) {
			struct k_thread *thread = (struct k_thread *)cur;

			if (_is_thread_timeout_expired(thread)) {
				continue;
			}

			return thread;
		}
		return NULL;
	}
#else
	ARG_UNUSED(from);
#endif

	return (struct k_thread *)sys_dlist_peek_head(wait_q);

}

/* Unpend a thread from the wait queue it is on. Thread must be pending. */
/* must be called with interrupts locked */
static inline void _unpend_thread(struct k_thread *thread)
{
	__ASSERT(thread->base.thread_state & _THREAD_PENDING, "");

	sys_dlist_remove(&thread->base.k_q_node);
	_mark_thread_as_not_pending(thread);
}

/* unpend the first thread from a wait queue */
/* must be called with interrupts locked */
static inline struct k_thread *_unpend_first_thread(_wait_q_t *wait_q)
{
	struct k_thread *thread = _find_first_thread_to_unpend(wait_q, NULL);

	if (thread) {
		_unpend_thread(thread);
	}

	return thread;
}

#ifdef CONFIG_USERSPACE
/**
 * Indicate whether the currently running thread has been configured to be
 * a user thread.
 *
 * @return nonzero if the current thread is a user thread, regardless of what
 *         mode the CPU is currently in
 */
static inline int _is_thread_user(void)
{
#ifdef CONFIG_ARCH_HAS_CUSTOM_SWAP_TO_MAIN
	/* the _current might be NULL before the first thread is scheduled if
	 * CONFIG_ARCH_HAS_CUSTOM_SWAP_TO_MAIN is enabled.
	 */
	if (!_current) {
		return 0;
	}

	return _current->base.user_options & K_USER;
#else
	return _current->base.user_options & K_USER;
#endif
}
#endif /* CONFIG_USERSPACE */
#endif /* _ksched__h_ */
