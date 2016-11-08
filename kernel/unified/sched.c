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

#include <kernel.h>
#include <kernel_structs.h>
#include <atomic.h>
#include <ksched.h>
#include <wait_q.h>

/* the only struct _kernel instance */
struct _kernel _kernel = {0};

/* set the bit corresponding to prio in ready q bitmap */
static void _set_ready_q_prio_bit(int prio)
{
	int bmap_index = _get_ready_q_prio_bmap_index(prio);
	uint32_t *bmap = &_ready_q.prio_bmap[bmap_index];

	*bmap |= _get_ready_q_prio_bit(prio);
}

/* clear the bit corresponding to prio in ready q bitmap */
static void _clear_ready_q_prio_bit(int prio)
{
	int bmap_index = _get_ready_q_prio_bmap_index(prio);
	uint32_t *bmap = &_ready_q.prio_bmap[bmap_index];

	*bmap &= ~_get_ready_q_prio_bit(prio);
}

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
	int q_index = _get_ready_q_q_index(thread->base.prio);
	sys_dlist_t *q = &_ready_q.q[q_index];

	_set_ready_q_prio_bit(thread->base.prio);
	sys_dlist_append(q, &thread->base.k_q_node);

	struct k_thread **cache = &_ready_q.cache;

	*cache = *cache && _is_prio_higher(thread->base.prio,
					   (*cache)->base.prio) ?
		 thread : *cache;
}

/*
 * This function, along with _move_thread_to_end_of_prio_q(), are the _only_
 * places where a thread is taken off the ready queue.
 *
 * Interrupts must be locked when calling this function.
 */

void _remove_thread_from_ready_q(struct k_thread *thread)
{
	int q_index = _get_ready_q_q_index(thread->base.prio);
	sys_dlist_t *q = &_ready_q.q[q_index];

	sys_dlist_remove(&thread->base.k_q_node);
	if (sys_dlist_is_empty(q)) {
		_clear_ready_q_prio_bit(thread->base.prio);
	}

	struct k_thread **cache = &_ready_q.cache;

	*cache = *cache == thread ? NULL : *cache;
}

/* reschedule threads if the scheduler is not locked */
/* not callable from ISR */
/* must be called with interrupts locked */
void _reschedule_threads(int key)
{
	K_DEBUG("rescheduling threads\n");

	if (_must_switch_threads()) {
		K_DEBUG("context-switching out %p\n", _current);
		_Swap(key);
	} else {
		irq_unlock(key);
	}
}

void k_sched_lock(void)
{
	__ASSERT(!_is_in_isr(), "");

	atomic_inc(&_current->base.sched_locked);

	K_DEBUG("scheduler locked (%p:%d)\n",
		_current, _current->base.sched_locked);
}

void k_sched_unlock(void)
{
	__ASSERT(_current->base.sched_locked > 0, "");
	__ASSERT(!_is_in_isr(), "");

	int key = irq_lock();

	atomic_dec(&_current->base.sched_locked);

	K_DEBUG("scheduler unlocked (%p:%d)\n",
		_current, _current->sched_locked);

	_reschedule_threads(key);
}

/*
 * Callback for sys_dlist_insert_at() to find the correct insert point in a
 * wait queue (priority-based).
 */
static int _is_wait_q_insert_point(sys_dnode_t *node, void *insert_prio)
{
	struct k_thread *waitq_node =
		CONTAINER_OF(
			CONTAINER_OF(node, struct _thread_base, k_q_node),
			struct k_thread,
			base);

	return _is_prio_higher((int)insert_prio, waitq_node->base.prio);
}

/* convert milliseconds to ticks */

#define ceiling(numerator, divider) \
	(((numerator) + ((divider) - 1)) / (divider))

int32_t _ms_to_ticks(int32_t ms)
{
	int64_t ms_ticks_per_sec = (int64_t)ms * sys_clock_ticks_per_sec;

	return (int32_t)ceiling(ms_ticks_per_sec, MSEC_PER_SEC);
}

/* pend the specified thread: it must *not* be in the ready queue */
/* must be called with interrupts locked */
void _pend_thread(struct k_thread *thread, _wait_q_t *wait_q, int32_t timeout)
{
	sys_dlist_t *dlist = (sys_dlist_t *)wait_q;

	sys_dlist_insert_at(dlist, &thread->base.k_q_node,
			    _is_wait_q_insert_point,
			    (void *)thread->base.prio);

	_mark_thread_as_pending(thread);

	if (timeout != K_FOREVER) {
		_mark_thread_as_timing(thread);
		_add_thread_timeout(thread, wait_q,
					_TICK_ALIGN + _ms_to_ticks(timeout));
	}
}

/* pend the current thread */
/* must be called with interrupts locked */
void _pend_current_thread(_wait_q_t *wait_q, int32_t timeout)
{
	_remove_thread_from_ready_q(_current);
	_pend_thread(_current, wait_q, timeout);
}

/*
 * Find the next thread to run when there is no thread in the cache and update
 * the cache.
 */
static struct k_thread *__get_next_ready_thread(void)
{
	int prio = _get_highest_ready_prio();
	int q_index = _get_ready_q_q_index(prio);
	sys_dlist_t *list = &_ready_q.q[q_index];

	__ASSERT(!sys_dlist_is_empty(list),
		 "no thread to run (prio: %d, queue index: %u)!\n",
		 prio, q_index);

	struct k_thread *thread =
		(struct k_thread *)sys_dlist_peek_head_not_empty(list);

	_ready_q.cache = thread;

	return thread;
}

/* find which one is the next thread to run */
/* must be called with interrupts locked */
struct k_thread *_get_next_ready_thread(void)
{
	struct k_thread *cache = _ready_q.cache;

	return cache ? cache : __get_next_ready_thread();
}

/*
 * Check if there is a thread of higher prio than the current one. Should only
 * be called if we already know that the current thread is preemptible.
 */
int __must_switch_threads(void)
{
	K_DEBUG("current prio: %d, highest prio: %d\n",
		_current->prio, _get_highest_ready_prio());

	extern void _dump_ready_q(void);
	_dump_ready_q();

	return _is_prio_higher(_get_highest_ready_prio(), _current->base.prio);
}

int _is_next_thread_current(void)
{
	return _get_next_ready_thread() == _current;
}

int  k_thread_priority_get(k_tid_t thread)
{
	return thread->base.prio;
}

void k_thread_priority_set(k_tid_t tid, int prio)
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
	_reschedule_threads(key);
}

/*
 * Interrupts must be locked when calling this function.
 *
 * This function, along with _add_thread_to_ready_q() and
 * _remove_thread_from_ready_q(), are the _only_ places where a thread is
 * taken off or put on the ready queue.
 */
void _move_thread_to_end_of_prio_q(struct k_thread *thread)
{
	int q_index = _get_ready_q_q_index(thread->base.prio);
	sys_dlist_t *q = &_ready_q.q[q_index];

	if (sys_dlist_is_tail(q, &thread->base.k_q_node)) {
		return;
	}

	sys_dlist_remove(&thread->base.k_q_node);
	sys_dlist_append(q, &thread->base.k_q_node);

	struct k_thread **cache = &_ready_q.cache;

	*cache = *cache == thread ? NULL : *cache;
}

void k_yield(void)
{
	__ASSERT(!_is_in_isr(), "");

	int key = irq_lock();

	_move_thread_to_end_of_prio_q(_current);

	if (_current == _get_next_ready_thread()) {
		irq_unlock(key);
	} else {
		_Swap(key);
	}
}

void k_sleep(int32_t duration)
{
	__ASSERT(!_is_in_isr(), "");
	__ASSERT(duration != K_FOREVER, "");

	K_DEBUG("thread %p for %d ns\n", _current, duration);

	/* wait of 0 ns is treated as a 'yield' */
	if (duration == 0) {
		k_yield();
		return;
	}

	int key = irq_lock();

	_mark_thread_as_timing(_current);
	_remove_thread_from_ready_q(_current);
	_add_thread_timeout(_current, NULL,
				_TICK_ALIGN + _ms_to_ticks(duration));

	_Swap(key);
}

void k_wakeup(k_tid_t thread)
{
	int key = irq_lock();

	/* verify first if thread is not waiting on an object */
	if (_is_thread_pending(thread)) {
		irq_unlock(key);
		return;
	}

	if (_abort_thread_timeout(thread) < 0) {
		irq_unlock(key);
		return;
	}

	_ready_thread(thread);

	if (_is_in_isr()) {
		irq_unlock(key);
	} else {
		_reschedule_threads(key);
	}
}

k_tid_t k_current_get(void)
{
	return _current;
}

/* debug aid */
void _dump_ready_q(void)
{
	K_DEBUG("bitmap: %x\n", _ready_q.prio_bmap[0]);
	for (int prio = 0; prio < K_NUM_PRIORITIES; prio++) {
		K_DEBUG("prio: %d, head: %p\n",
			prio - CONFIG_NUM_COOP_PRIORITIES,
			sys_dlist_peek_head(&_ready_q.q[prio]));
	}
}

#ifdef CONFIG_TIMESLICING
extern int32_t _time_slice_duration;    /* Measured in ms */
extern int32_t _time_slice_elapsed;     /* Measured in ms */
extern int _time_slice_prio_ceiling;

void k_sched_time_slice_set(int32_t duration_in_ms, int prio)
{
	__ASSERT(duration_in_ms >= 0, "");
	__ASSERT((prio >= 0) && (prio < CONFIG_NUM_PREEMPT_PRIORITIES), "");

	_time_slice_duration = duration_in_ms;
	_time_slice_elapsed = 0;
	_time_slice_prio_ceiling = prio;
}
#endif /* CONFIG_TIMESLICING */

int k_is_preempt_thread(void)
{
	return !_is_in_isr() && _is_preempt(_current);
}
