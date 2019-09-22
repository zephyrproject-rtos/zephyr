/*
 * Copyright (c) 2016-2017 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_KERNEL_INCLUDE_KSCHED_H_
#define ZEPHYR_KERNEL_INCLUDE_KSCHED_H_

#include <kernel_structs.h>
#include <kernel_internal.h>
#include <timeout_q.h>
#include <debug/tracing.h>
#include <stdbool.h>

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
#endif

void z_sched_init(void);
void z_add_thread_to_ready_q(struct k_thread *thread);
void z_move_thread_to_end_of_prio_q(struct k_thread *thread);
void z_remove_thread_from_ready_q(struct k_thread *thread);
int z_is_thread_time_slicing(struct k_thread *thread);
void z_unpend_thread_no_timeout(struct k_thread *thread);
int z_pend_curr(struct k_spinlock *lock, k_spinlock_key_t key,
	       _wait_q_t *wait_q, s32_t timeout);
int z_pend_curr_irqlock(u32_t key, _wait_q_t *wait_q, s32_t timeout);
void z_pend_thread(struct k_thread *thread, _wait_q_t *wait_q, s32_t timeout);
void z_reschedule(struct k_spinlock *lock, k_spinlock_key_t key);
void z_reschedule_irqlock(u32_t key);
struct k_thread *z_unpend_first_thread(_wait_q_t *wait_q);
void z_unpend_thread(struct k_thread *thread);
int z_unpend_all(_wait_q_t *wait_q);
void z_thread_priority_set(struct k_thread *thread, int prio);
bool z_set_prio(struct k_thread *thread, int prio);
void *z_get_next_switch_handle(void *interrupted);
struct k_thread *z_find_first_thread_to_unpend(_wait_q_t *wait_q,
					      struct k_thread *from);
void idle(void *a, void *b, void *c);
void z_time_slice(int ticks);
void z_reset_time_slice(void);
void z_sched_abort(struct k_thread *thread);
void z_sched_ipi(void);

static inline void z_pend_curr_unlocked(_wait_q_t *wait_q, s32_t timeout)
{
	(void) z_pend_curr_irqlock(z_arch_irq_lock(), wait_q, timeout);
}

static inline void z_reschedule_unlocked(void)
{
	(void) z_reschedule_irqlock(z_arch_irq_lock());
}

/* find which one is the next thread to run */
/* must be called with interrupts locked */
#ifdef CONFIG_SMP
extern struct k_thread *z_get_next_ready_thread(void);
#else
static ALWAYS_INLINE struct k_thread *z_get_next_ready_thread(void)
{
	return _kernel.ready_q.cache;
}
#endif

static inline bool z_is_idle_thread_entry(void *entry_point)
{
	return entry_point == idle;
}

static inline bool z_is_idle_thread_object(struct k_thread *thread)
{
#ifdef CONFIG_SMP
	return thread->base.is_idle;
#else
	return thread == &z_idle_thread;
#endif
}

static inline bool z_is_thread_pending(struct k_thread *thread)
{
	return (thread->base.thread_state & _THREAD_PENDING) != 0U;
}

static inline bool z_is_thread_prevented_from_running(struct k_thread *thread)
{
	u8_t state = thread->base.thread_state;

	return (state & (_THREAD_PENDING | _THREAD_PRESTART | _THREAD_DEAD |
			 _THREAD_DUMMY | _THREAD_SUSPENDED)) != 0U;

}

static inline bool z_is_thread_timeout_active(struct k_thread *thread)
{
	return !z_is_inactive_timeout(&thread->base.timeout);
}

static inline bool z_is_thread_ready(struct k_thread *thread)
{
	return !((z_is_thread_prevented_from_running(thread)) != 0 ||
		 z_is_thread_timeout_active(thread));
}

static inline bool z_has_thread_started(struct k_thread *thread)
{
	return (thread->base.thread_state & _THREAD_PRESTART) == 0U;
}

static inline bool z_is_thread_state_set(struct k_thread *thread, u32_t state)
{
	return (thread->base.thread_state & state) != 0U;
}

static inline bool z_is_thread_queued(struct k_thread *thread)
{
	return z_is_thread_state_set(thread, _THREAD_QUEUED);
}

static inline void z_mark_thread_as_suspended(struct k_thread *thread)
{
	thread->base.thread_state |= _THREAD_SUSPENDED;
}

static inline void z_mark_thread_as_not_suspended(struct k_thread *thread)
{
	thread->base.thread_state &= ~_THREAD_SUSPENDED;
}

static inline void z_mark_thread_as_started(struct k_thread *thread)
{
	thread->base.thread_state &= ~_THREAD_PRESTART;
}

static inline void z_mark_thread_as_pending(struct k_thread *thread)
{
	thread->base.thread_state |= _THREAD_PENDING;
}

static inline void z_mark_thread_as_not_pending(struct k_thread *thread)
{
	thread->base.thread_state &= ~_THREAD_PENDING;
}

static inline void z_set_thread_states(struct k_thread *thread, u32_t states)
{
	thread->base.thread_state |= states;
}

static inline void z_reset_thread_states(struct k_thread *thread,
					u32_t states)
{
	thread->base.thread_state &= ~states;
}

static inline void z_mark_thread_as_queued(struct k_thread *thread)
{
	z_set_thread_states(thread, _THREAD_QUEUED);
}

static inline void z_mark_thread_as_not_queued(struct k_thread *thread)
{
	z_reset_thread_states(thread, _THREAD_QUEUED);
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

bool z_is_t1_higher_prio_than_t2(struct k_thread *t1, struct k_thread *t2);

static inline bool _is_valid_prio(int prio, void *entry_point)
{
	if (prio == K_IDLE_PRIO && z_is_idle_thread_entry(entry_point)) {
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

static ALWAYS_INLINE void z_ready_thread(struct k_thread *thread)
{
	if (z_is_thread_ready(thread)) {
		z_add_thread_to_ready_q(thread);
	}

	sys_trace_thread_ready(thread);
}

static inline void _ready_one_thread(_wait_q_t *wq)
{
	struct k_thread *th = z_unpend_first_thread(wq);

	if (th != NULL) {
		z_ready_thread(th);
	}
}

static inline void z_sched_lock(void)
{
#ifdef CONFIG_PREEMPT_ENABLED
	__ASSERT(!z_arch_is_in_isr(), "");
	__ASSERT(_current->base.sched_locked != 1, "");

	--_current->base.sched_locked;

	compiler_barrier();

	K_DEBUG("scheduler locked (%p:%d)\n",
		_current, _current->base.sched_locked);
#endif
}

static ALWAYS_INLINE void z_sched_unlock_no_reschedule(void)
{
#ifdef CONFIG_PREEMPT_ENABLED
	__ASSERT(!z_arch_is_in_isr(), "");
	__ASSERT(_current->base.sched_locked != 0, "");

	compiler_barrier();

	++_current->base.sched_locked;
#endif
}

static ALWAYS_INLINE bool z_is_thread_timeout_expired(struct k_thread *thread)
{
#ifdef CONFIG_SYS_CLOCK_EXISTS
	return thread->base.timeout.dticks == _EXPIRED;
#else
	return 0;
#endif
}

static inline struct k_thread *z_unpend1_no_timeout(_wait_q_t *wait_q)
{
	struct k_thread *thread = z_find_first_thread_to_unpend(wait_q, NULL);

	if (thread != NULL) {
		z_unpend_thread_no_timeout(thread);
	}

	return thread;
}

#endif /* ZEPHYR_KERNEL_INCLUDE_KSCHED_H_ */
