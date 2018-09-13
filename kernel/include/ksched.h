/*
 * Copyright (c) 2016-2017 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_KERNEL_INCLUDE_KSCHED_H_
#define ZEPHYR_KERNEL_INCLUDE_KSCHED_H_

#include <kernel_structs.h>
#include <tracing.h>

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

void _sched_init(void);
void _add_thread_to_ready_q(struct k_thread *thread);
void _move_thread_to_end_of_prio_q(struct k_thread *thread);
void _remove_thread_from_ready_q(struct k_thread *thread);
int _is_thread_time_slicing(struct k_thread *thread);
void _unpend_thread_no_timeout(struct k_thread *thread);
int _pend_current_thread(int key, _wait_q_t *wait_q, s32_t timeout);
void _pend_thread(struct k_thread *thread, _wait_q_t *wait_q, s32_t timeout);
void _reschedule(int key);
struct k_thread *_unpend_first_thread(_wait_q_t *wait_q);
void _unpend_thread(struct k_thread *thread);
int _unpend_all(_wait_q_t *wait_q);
void _thread_priority_set(struct k_thread *thread, int prio);
void *_get_next_switch_handle(void *interrupted);
struct k_thread *_find_first_thread_to_unpend(_wait_q_t *wait_q,
					      struct k_thread *from);
void idle(void *a, void *b, void *c);
void z_reset_timeslice(void);

/* find which one is the next thread to run */
/* must be called with interrupts locked */
#ifdef CONFIG_SMP
extern struct k_thread *_get_next_ready_thread(void);
#else
static ALWAYS_INLINE struct k_thread *_get_next_ready_thread(void)
{
	return _ready_q.cache;
}
#endif


static inline int _is_idle_thread(void *entry_point)
{
	return entry_point == idle;
}

static inline int _is_thread_pending(struct k_thread *thread)
{
	return !!(thread->base.thread_state & _THREAD_PENDING);
}

static inline int _is_thread_prevented_from_running(struct k_thread *thread)
{
	u8_t state = thread->base.thread_state;

	return state & (_THREAD_PENDING | _THREAD_PRESTART | _THREAD_DEAD |
			_THREAD_DUMMY | _THREAD_SUSPENDED);

}

static inline int _is_thread_timeout_active(struct k_thread *thread)
{
#ifdef CONFIG_SYS_CLOCK_EXISTS
	return thread->base.timeout.delta_ticks_from_prev != _INACTIVE;
#else
	return 0;
#endif
}

static inline int _is_thread_ready(struct k_thread *thread)
{
	return !(_is_thread_prevented_from_running(thread) ||
		 _is_thread_timeout_active(thread));
}

static inline int _has_thread_started(struct k_thread *thread)
{
	return !(thread->base.thread_state & _THREAD_PRESTART);
}

static inline int _is_thread_state_set(struct k_thread *thread, u32_t state)
{
	return !!(thread->base.thread_state & state);
}

static inline int _is_thread_queued(struct k_thread *thread)
{
	return _is_thread_state_set(thread, _THREAD_QUEUED);
}

static inline void _mark_thread_as_suspended(struct k_thread *thread)
{
	thread->base.thread_state |= _THREAD_SUSPENDED;
}

static inline void _mark_thread_as_not_suspended(struct k_thread *thread)
{
	thread->base.thread_state &= ~_THREAD_SUSPENDED;
}

static inline void _mark_thread_as_started(struct k_thread *thread)
{
	thread->base.thread_state &= ~_THREAD_PRESTART;
}

static inline void _mark_thread_as_pending(struct k_thread *thread)
{
	thread->base.thread_state |= _THREAD_PENDING;
}

static inline void _mark_thread_as_not_pending(struct k_thread *thread)
{
	thread->base.thread_state &= ~_THREAD_PENDING;
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

static inline void _mark_thread_as_queued(struct k_thread *thread)
{
	_set_thread_states(thread, _THREAD_QUEUED);
}

static inline void _mark_thread_as_not_queued(struct k_thread *thread)
{
	_reset_thread_states(thread, _THREAD_QUEUED);
}

static inline int _is_under_prio_ceiling(int prio)
{
	return prio >= CONFIG_PRIORITY_CEILING;
}

static inline int _get_new_prio_with_ceiling(int prio)
{
	return _is_under_prio_ceiling(prio) ? prio : CONFIG_PRIORITY_CEILING;
}

static inline int _is_prio1_higher_than_or_equal_to_prio2(int prio1, int prio2)
{
	return prio1 <= prio2;
}

static inline int _is_prio_higher_or_equal(int prio1, int prio2)
{
	return _is_prio1_higher_than_or_equal_to_prio2(prio1, prio2);
}

static inline int _is_prio1_lower_than_or_equal_to_prio2(int prio1, int prio2)
{
	return prio1 >= prio2;
}

static inline int _is_prio1_higher_than_prio2(int prio1, int prio2)
{
	return prio1 < prio2;
}

static inline int _is_prio_higher(int prio, int test_prio)
{
	return _is_prio1_higher_than_prio2(prio, test_prio);
}

static inline int _is_prio_lower_or_equal(int prio1, int prio2)
{
	return _is_prio1_lower_than_or_equal_to_prio2(prio1, prio2);
}

int _is_t1_higher_prio_than_t2(struct k_thread *t1, struct k_thread *t2);

static inline int _is_valid_prio(int prio, void *entry_point)
{
	if (prio == K_IDLE_PRIO && _is_idle_thread(entry_point)) {
		return 1;
	}

	if (!_is_prio_higher_or_equal(prio,
				      K_LOWEST_APPLICATION_THREAD_PRIO)) {
		return 0;
	}

	if (!_is_prio_lower_or_equal(prio,
				     K_HIGHEST_APPLICATION_THREAD_PRIO)) {
		return 0;
	}

	return 1;
}

static inline void _ready_thread(struct k_thread *thread)
{
	if (_is_thread_ready(thread)) {
		_add_thread_to_ready_q(thread);
	}

#if defined(CONFIG_TICKLESS_KERNEL) && !defined(CONFIG_SMP)
	z_reset_timeslice();
#endif

	sys_trace_thread_ready(thread);

}

static inline void _ready_one_thread(_wait_q_t *wq)
{
	struct k_thread *th = _unpend_first_thread(wq);

	if (th) {
		_ready_thread(th);
	}
}

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

static ALWAYS_INLINE void _sched_unlock_no_reschedule(void)
{
#ifdef CONFIG_PREEMPT_ENABLED
	__ASSERT(!_is_in_isr(), "");
	__ASSERT(_current->base.sched_locked != 0, "");

	compiler_barrier();

	++_current->base.sched_locked;
#endif
}

static ALWAYS_INLINE int _is_thread_timeout_expired(struct k_thread *thread)
{
#ifdef CONFIG_SYS_CLOCK_EXISTS
	return thread->base.timeout.delta_ticks_from_prev == _EXPIRED;
#else
	return 0;
#endif
}

static inline struct k_thread *_unpend1_no_timeout(_wait_q_t *wait_q)
{
	struct k_thread *thread = _find_first_thread_to_unpend(wait_q, NULL);

	if (thread) {
		_unpend_thread_no_timeout(thread);
	}

	return thread;
}

#endif /* ZEPHYR_KERNEL_INCLUDE_KSCHED_H_ */
