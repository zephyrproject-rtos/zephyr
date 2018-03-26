/*
 * Copyright (c) 2016-2017 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <kernel_structs.h>
#include <atomic.h>
#include <ksched.h>
#include <wait_q.h>
#include <misc/util.h>
#include <syscall_handler.h>
#include <kswap.h>

/* the only struct _kernel instance */
struct _kernel _kernel = {0};

/* set the bit corresponding to prio in ready q bitmap */
#if defined(CONFIG_MULTITHREADING) && !defined(CONFIG_SMP)
static void set_ready_q_prio_bit(int prio)
{
	int bmap_index = _get_ready_q_prio_bmap_index(prio);
	u32_t *bmap = &_ready_q.prio_bmap[bmap_index];

	*bmap |= _get_ready_q_prio_bit(prio);
}

/* clear the bit corresponding to prio in ready q bitmap */
static void clear_ready_q_prio_bit(int prio)
{
	int bmap_index = _get_ready_q_prio_bmap_index(prio);
	u32_t *bmap = &_ready_q.prio_bmap[bmap_index];

	*bmap &= ~_get_ready_q_prio_bit(prio);
}
#endif

#if !defined(CONFIG_SMP) && defined(CONFIG_MULTITHREADING)
/*
 * Find the next thread to run when there is no thread in the cache and update
 * the cache.
 */
static struct k_thread *get_ready_q_head(void)
{
	int prio = _get_highest_ready_prio();
	int q_index = _get_ready_q_q_index(prio);
	sys_dlist_t *list = &_ready_q.q[q_index];

	__ASSERT(!sys_dlist_is_empty(list),
		 "no thread to run (prio: %d, queue index: %u)!\n",
		 prio, q_index);

	struct k_thread *thread =
		(struct k_thread *)sys_dlist_peek_head_not_empty(list);

	return thread;
}
#endif

/*
 * Add thread to the ready queue, in the slot for its priority; the thread
 * must not be on a wait queue.
 *
 * This function, along with _move_thread_to_end_of_prio_q(), are the _only_
 * places where a thread is put on the ready queue.
 *
 * Interrupts must be locked when calling this function.
 */

void _add_thread_to_ready_q(struct k_thread *thread)
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

#ifdef CONFIG_MULTITHREADING
	int q_index = _get_ready_q_q_index(thread->base.prio);
	sys_dlist_t *q = &_ready_q.q[q_index];

# ifndef CONFIG_SMP
	set_ready_q_prio_bit(thread->base.prio);
# endif
	sys_dlist_append(q, &thread->base.k_q_node);

# ifndef CONFIG_SMP
	struct k_thread **cache = &_ready_q.cache;

	*cache = _is_t1_higher_prio_than_t2(thread, *cache) ? thread : *cache;
# endif
#else
	sys_dlist_append(&_ready_q.q[0], &thread->base.k_q_node);
	_ready_q.prio_bmap[0] = 1;
# ifndef CONFIG_SMP
	_ready_q.cache = thread;
# endif
#endif
}

/*
 * This function, along with _move_thread_to_end_of_prio_q(), are the _only_
 * places where a thread is taken off the ready queue.
 *
 * Interrupts must be locked when calling this function.
 */

void _remove_thread_from_ready_q(struct k_thread *thread)
{
#if defined(CONFIG_MULTITHREADING) && !defined(CONFIG_SMP)
	int q_index = _get_ready_q_q_index(thread->base.prio);
	sys_dlist_t *q = &_ready_q.q[q_index];

	sys_dlist_remove(&thread->base.k_q_node);
	if (sys_dlist_is_empty(q)) {
		clear_ready_q_prio_bit(thread->base.prio);
	}

	struct k_thread **cache = &_ready_q.cache;

	*cache = *cache == thread ? get_ready_q_head() : *cache;
#else
# if !defined(CONFIG_SMP)
	_ready_q.prio_bmap[0] = 0;
	_ready_q.cache = NULL;
# endif
	sys_dlist_remove(&thread->base.k_q_node);
#endif
}

/* Releases the irq_lock and swaps to a higher priority thread if one
 * is available, returning the _Swap() return value, otherwise zero.
 * Does not swap away from a thread at a cooperative (unpreemptible)
 * priority unless "yield" is true.
 */
static int resched(int key, int yield)
{
	K_DEBUG("rescheduling threads\n");

	if (!_is_in_isr() &&
	    (yield || _is_preempt(_current)) &&
	    _is_prio_higher(_get_highest_ready_prio(), _current->base.prio)) {
		K_DEBUG("context-switching out %p\n", _current);
		return _Swap(key);
	} else {
		irq_unlock(key);
		return 0;
	}
}

int _reschedule_noyield(int key)
{
	return resched(key, 0);
}

int _reschedule_yield(int key)
{
	return resched(key, 1);
}

void k_sched_lock(void)
{
	_sched_lock();
}

void k_sched_unlock(void)
{
#ifdef CONFIG_PREEMPT_ENABLED
	__ASSERT(_current->base.sched_locked != 0, "");
	__ASSERT(!_is_in_isr(), "");

	int key = irq_lock();

	/* compiler_barrier() not needed, comes from irq_lock() */

	++_current->base.sched_locked;

	K_DEBUG("scheduler unlocked (%p:%d)\n",
		_current, _current->base.sched_locked);

	_reschedule_noyield(key);
#endif
}

/* convert milliseconds to ticks */

#ifdef _NON_OPTIMIZED_TICKS_PER_SEC
s32_t _ms_to_ticks(s32_t ms)
{
	s64_t ms_ticks_per_sec = (s64_t)ms * sys_clock_ticks_per_sec;

	return (s32_t)ceiling_fraction(ms_ticks_per_sec, MSEC_PER_SEC);
}
#endif

/* Pend the specified thread: it must *not* be in the ready queue.  It
 * must be either _current or a DUMMY thread (i.e. this is NOT an API
 * for pending another thread that might be running!).  It must be
 * called with interrupts locked
 */
void _pend_thread(struct k_thread *thread, _wait_q_t *wait_q, s32_t timeout)
{
	__ASSERT(thread == _current || _is_thread_dummy(thread),
		 "Can only pend _current or DUMMY");

#ifdef CONFIG_MULTITHREADING
	sys_dlist_t *wait_q_list = (sys_dlist_t *)wait_q;
	struct k_thread *pending;

	if (!wait_q_list) {
		goto inserted;
	}

	SYS_DLIST_FOR_EACH_CONTAINER(wait_q_list, pending, base.k_q_node) {
		if (_is_t1_higher_prio_than_t2(thread, pending)) {
			sys_dlist_insert_before(wait_q_list,
						&pending->base.k_q_node,
						&thread->base.k_q_node);
			goto inserted;
		}
	}

	sys_dlist_append(wait_q_list, &thread->base.k_q_node);

inserted:
	_mark_thread_as_pending(thread);

	if (timeout != K_FOREVER) {
		s32_t ticks = _TICK_ALIGN + _ms_to_ticks(timeout);

		_add_thread_timeout(thread, wait_q, ticks);
	}
#endif
}

/* Block the current thread and swap to the next.  Releases the
 * irq_lock, does a _Swap and returns the return value set at wakeup
 * time
 */
int _pend_current_thread(int key, _wait_q_t *wait_q, s32_t timeout)
{
	_remove_thread_from_ready_q(_current);
	_pend_thread(_current, wait_q, timeout);
	return _Swap(key);
}

int _impl_k_thread_priority_get(k_tid_t thread)
{
	return thread->base.prio;
}

#ifdef CONFIG_USERSPACE
_SYSCALL_HANDLER1_SIMPLE(k_thread_priority_get, K_OBJ_THREAD,
			 struct k_thread *);
#endif

void _impl_k_thread_priority_set(k_tid_t tid, int prio)
{
	/*
	 * Use NULL, since we cannot know what the entry point is (we do not
	 * keep track of it) and idle cannot change its priority.
	 */
	_ASSERT_VALID_PRIO(prio, NULL);
	__ASSERT(!_is_in_isr(), "");

	struct k_thread *thread = (struct k_thread *)tid;
	int key = irq_lock();

	_thread_priority_set(thread, prio);
	_reschedule_noyield(key);
}

#ifdef CONFIG_USERSPACE
_SYSCALL_HANDLER(k_thread_priority_set, thread_p, prio)
{
	struct k_thread *thread = (struct k_thread *)thread_p;

	_SYSCALL_OBJ(thread, K_OBJ_THREAD);
	_SYSCALL_VERIFY_MSG(_is_valid_prio(prio, NULL),
			    "invalid thread priority %d", (int)prio);
	_SYSCALL_VERIFY_MSG((s8_t)prio >= thread->base.prio,
			    "thread priority may only be downgraded (%d < %d)",
			    prio, thread->base.prio);

	_impl_k_thread_priority_set((k_tid_t)thread, prio);
	return 0;
}
#endif

/*
 * Interrupts must be locked when calling this function.
 *
 * This function, along with _add_thread_to_ready_q() and
 * _remove_thread_from_ready_q(), are the _only_ places where a thread is
 * taken off or put on the ready queue.
 */
void _move_thread_to_end_of_prio_q(struct k_thread *thread)
{
#ifdef CONFIG_MULTITHREADING
	int q_index = _get_ready_q_q_index(thread->base.prio);
	sys_dlist_t *q = &_ready_q.q[q_index];

	if (sys_dlist_is_tail(q, &thread->base.k_q_node)) {
		return;
	}

	sys_dlist_remove(&thread->base.k_q_node);
	sys_dlist_append(q, &thread->base.k_q_node);

# ifndef CONFIG_SMP
	struct k_thread **cache = &_ready_q.cache;

	*cache = *cache == thread ? get_ready_q_head() : *cache;
# endif
#endif
}

void _impl_k_yield(void)
{
	__ASSERT(!_is_in_isr(), "");

	int key = irq_lock();

	_move_thread_to_end_of_prio_q(_current);

	if (_current == _get_next_ready_thread()) {
		irq_unlock(key);
#ifdef CONFIG_STACK_SENTINEL
		_check_stack_sentinel();
#endif
	} else {
		_Swap(key);
	}
}

#ifdef CONFIG_USERSPACE
_SYSCALL_HANDLER0_SIMPLE_VOID(k_yield);
#endif

void _impl_k_sleep(s32_t duration)
{
#ifdef CONFIG_MULTITHREADING
	/* volatile to guarantee that irq_lock() is executed after ticks is
	 * populated
	 */
	volatile s32_t ticks;
	unsigned int key;

	__ASSERT(!_is_in_isr(), "");
	__ASSERT(duration != K_FOREVER, "");

	K_DEBUG("thread %p for %d ns\n", _current, duration);

	/* wait of 0 ms is treated as a 'yield' */
	if (duration == 0) {
		k_yield();
		return;
	}

	ticks = _TICK_ALIGN + _ms_to_ticks(duration);
	key = irq_lock();

	_remove_thread_from_ready_q(_current);
	_add_thread_timeout(_current, NULL, ticks);

	_Swap(key);
#endif
}

#ifdef CONFIG_USERSPACE
_SYSCALL_HANDLER(k_sleep, duration)
{
	/* FIXME there were some discussions recently on whether we should
	 * relax this, thread would be unscheduled until k_wakeup issued
	 */
	_SYSCALL_VERIFY_MSG(duration != K_FOREVER,
			    "sleeping forever not allowed");
	_impl_k_sleep(duration);

	return 0;
}
#endif

void _impl_k_wakeup(k_tid_t thread)
{
	int key = irq_lock();

	/* verify first if thread is not waiting on an object */
	if (_is_thread_pending(thread)) {
		irq_unlock(key);
		return;
	}

	if (_abort_thread_timeout(thread) == _INACTIVE) {
		irq_unlock(key);
		return;
	}

	_ready_thread(thread);

	if (_is_in_isr()) {
		irq_unlock(key);
	} else {
		_reschedule_noyield(key);
	}
}

#ifdef CONFIG_USERSPACE
_SYSCALL_HANDLER1_SIMPLE_VOID(k_wakeup, K_OBJ_THREAD, k_tid_t);
#endif

k_tid_t _impl_k_current_get(void)
{
	return _current;
}

#ifdef CONFIG_USERSPACE
_SYSCALL_HANDLER0_SIMPLE(k_current_get);
#endif

#ifdef CONFIG_TIMESLICING
extern s32_t _time_slice_duration;    /* Measured in ms */
extern s32_t _time_slice_elapsed;     /* Measured in ms */
extern int _time_slice_prio_ceiling;

void k_sched_time_slice_set(s32_t duration_in_ms, int prio)
{
	__ASSERT(duration_in_ms >= 0, "");
	__ASSERT((prio >= 0) && (prio < CONFIG_NUM_PREEMPT_PRIORITIES), "");

	_time_slice_duration = duration_in_ms;
	_time_slice_elapsed = 0;
	_time_slice_prio_ceiling = prio;
}

int _is_thread_time_slicing(struct k_thread *thread)
{
	/*
	 * Time slicing is done on the thread if following conditions are met
	 *
	 * Time slice duration should be set > 0
	 * Should not be the idle thread
	 * Priority should be higher than time slice priority ceiling
	 * There should be multiple threads active with same priority
	 */

	if (!(_time_slice_duration > 0) || (_is_idle_thread_ptr(thread))
	    || _is_prio_higher(thread->base.prio, _time_slice_prio_ceiling)) {
		return 0;
	}

	int q_index = _get_ready_q_q_index(thread->base.prio);
	sys_dlist_t *q = &_ready_q.q[q_index];

	return sys_dlist_has_multiple_nodes(q);
}

/* Must be called with interrupts locked */
/* Should be called only immediately before a thread switch */
void _update_time_slice_before_swap(void)
{
#ifdef CONFIG_TICKLESS_KERNEL
	if (!_is_thread_time_slicing(_get_next_ready_thread())) {
		return;
	}

	u32_t remaining = _get_remaining_program_time();

	if (!remaining || (_time_slice_duration < remaining)) {
		_set_time(_time_slice_duration);
	} else {
		/* Account previous elapsed time and reprogram
		 * timer with remaining time
		 */
		_set_time(remaining);
	}

#endif
	/* Restart time slice count at new thread switch */
	_time_slice_elapsed = 0;
}
#endif /* CONFIG_TIMESLICING */

int _impl_k_is_preempt_thread(void)
{
	return !_is_in_isr() && _is_preempt(_current);
}

#ifdef CONFIG_USERSPACE
_SYSCALL_HANDLER0_SIMPLE(k_is_preempt_thread);
#endif

#ifdef CONFIG_SMP
int _get_highest_ready_prio(void)
{
	int p;

	for (p = 0; p < ARRAY_SIZE(_kernel.ready_q.q); p++) {
		if (!sys_dlist_is_empty(&_kernel.ready_q.q[p])) {
			break;
		}
	}

	__ASSERT(p < K_NUM_PRIORITIES, "No ready prio");

	return p - _NUM_COOP_PRIO;
}

struct k_thread *_get_next_ready_thread(void)
{
	int p, mycpu = _arch_curr_cpu()->id;

	for (p = 0; p < ARRAY_SIZE(_ready_q.q); p++) {
		sys_dlist_t *list = &_ready_q.q[p];
		sys_dnode_t *node;

		for (node = list->tail; node != list; node = node->prev) {
			struct k_thread *th = (struct k_thread *)node;

			/* Skip threads that are already running elsewhere! */
			if (th->base.active && th->base.cpu != mycpu) {
				continue;
			}

			return th;
		}
	}

	__ASSERT(0, "No ready thread found for cpu %d\n", mycpu);
	return NULL;
}
#endif

#ifdef CONFIG_USE_SWITCH
void *_get_next_switch_handle(void *interrupted)
{
	if (!_is_preempt(_current) &&
	    !(_current->base.thread_state & _THREAD_DEAD)) {
		return interrupted;
	}

	int key = irq_lock();

	_current->switch_handle = interrupted;
	_current = _get_next_ready_thread();

	void *ret = _current->switch_handle;

	irq_unlock(key);

	_check_stack_sentinel();

	return ret;
}
#endif
