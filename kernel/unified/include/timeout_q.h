/*
 * Copyright (c) 2015 Wind River Systems, Inc.
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

#ifndef _kernel_include_timeout_q__h_
#define _kernel_include_timeout_q__h_

/**
 * @file
 * @brief timeout queue for threads on kernel objects
 *
 * This file is meant to be included by kernel/include/wait_q.h only
 */

#include <misc/dlist.h>

#ifdef __cplusplus
extern "C" {
#endif

/* initialize the timeouts part of k_thread when enabled in the kernel */

static inline void _init_timeout(struct _timeout *t, _timeout_func_t func)
{
	/*
	 * Must be initialized here and when dequeueing a timeout so that code
	 * not dealing with timeouts does not have to handle this, such as when
	 * waiting forever on a semaphore.
	 */
	t->delta_ticks_from_prev = _INACTIVE;

	/*
	 * Must be initialized here so that the _fiber_wakeup family of APIs can
	 * verify the fiber is not on a wait queue before aborting a timeout.
	 */
	t->wait_q = NULL;

	/*
	 * Must be initialized here, so the _handle_one_timeout()
	 * routine can check if there is a fiber waiting on this timeout
	 */
	t->thread = NULL;

	/*
	 * Function must be initialized before being potentially called.
	 */
	t->func = func;

	/*
	 * These are initialized when enqueing on the timeout queue:
	 *
	 *   thread->timeout.node.next
	 *   thread->timeout.node.prev
	 */
}

static ALWAYS_INLINE void
_init_thread_timeout(struct _thread_base *thread_base)
{
	_init_timeout(&thread_base->timeout, NULL);
}

/* remove a thread timing out from kernel object's wait queue */

static inline void _unpend_thread_timing_out(struct k_thread *thread,
					     struct _timeout *timeout_obj)
{
	if (timeout_obj->wait_q) {
		_unpend_thread(thread);
		thread->base.timeout.wait_q = NULL;
	}
}

/*
 * Handle one timeout from the expired timeout queue. Removes it from the wait
 * queue it is on if waiting for an object; in this case, the return value is
 * kept as -EAGAIN, set previously in _Swap().
 */

static inline void _handle_one_expired_timeout(struct _timeout *timeout)
{
	struct k_thread *thread = timeout->thread;
	unsigned int key = irq_lock();

	timeout->delta_ticks_from_prev = _INACTIVE;

	K_DEBUG("timeout %p\n", timeout);
	if (thread) {
		_unpend_thread_timing_out(thread, timeout);
		_ready_thread(thread);
		irq_unlock(key);
	} else {
		irq_unlock(key);
		if (timeout->func) {
			timeout->func(timeout);
		}
	}
}

/*
 * Loop over all expired timeouts and handle them one by one. Should be called
 * with interrupts unlocked: interrupts will be locked on each interation only
 * for the amount of time necessary.
 */

static inline void _handle_expired_timeouts(sys_dlist_t *expired)
{
	sys_dnode_t *timeout, *next;

	SYS_DLIST_FOR_EACH_NODE_SAFE(expired, timeout, next) {
		sys_dlist_remove(timeout);
		_handle_one_expired_timeout((struct _timeout *)timeout);
	}
}

/* returns _INACTIVE if the timer is not active */
static inline int _abort_timeout(struct _timeout *timeout)
{
	if (timeout->delta_ticks_from_prev == _INACTIVE) {
		return _INACTIVE;
	}

	if (!sys_dlist_is_tail(&_timeout_q, &timeout->node)) {
		sys_dnode_t *next_node =
			sys_dlist_peek_next(&_timeout_q, &timeout->node);
		struct _timeout *next = (struct _timeout *)next_node;

		next->delta_ticks_from_prev += timeout->delta_ticks_from_prev;
	}
	sys_dlist_remove(&timeout->node);
	timeout->delta_ticks_from_prev = _INACTIVE;

	return 0;
}

/* returns _INACTIVE if the timer has already expired */
static inline int _abort_thread_timeout(struct k_thread *thread)
{
	return _abort_timeout(&thread->base.timeout);
}

/*
 * callback for sys_dlist_insert_at():
 *
 * Returns 1 if the timeout to insert is lower or equal than the next timeout
 * in the queue, signifying that it should be inserted before the next.
 * Returns 0 if it is greater.
 *
 * If it is greater, the timeout to insert is decremented by the next timeout,
 * since the timeout queue is a delta queue.  If it lower or equal, decrement
 * the timeout of the insert point to update its delta queue value, since the
 * current timeout will be inserted before it.
 */

static int _is_timeout_insert_point(sys_dnode_t *test, void *timeout)
{
	struct _timeout *t = (void *)test;
	int32_t *timeout_to_insert = timeout;

	if (*timeout_to_insert > t->delta_ticks_from_prev) {
		*timeout_to_insert -= t->delta_ticks_from_prev;
		return 0;
	}

	t->delta_ticks_from_prev -= *timeout_to_insert;
	return 1;
}

/*
 * Add timeout to timeout queue. Record waiting thread and wait queue if any.
 *
 * Cannot handle timeout == 0 and timeout == K_FOREVER.
 *
 * Must be called with interrupts locked.
 */

static inline void _add_timeout(struct k_thread *thread,
				struct _timeout *timeout_obj,
				_wait_q_t *wait_q, int32_t timeout)
{
	__ASSERT(timeout > 0, "");

	K_DEBUG("thread %p on wait_q %p, for timeout: %d\n",
		thread, wait_q, timeout);

	sys_dlist_t *timeout_q = &_timeout_q;

	K_DEBUG("timeout_q %p before: head: %p, tail: %p\n",
		&_timeout_q,
		sys_dlist_peek_head(&_timeout_q),
		_timeout_q.tail);

	K_DEBUG("timeout   %p before: next: %p, prev: %p\n",
		timeout_obj, timeout_obj->node.next, timeout_obj->node.prev);

	timeout_obj->thread = thread;
	timeout_obj->delta_ticks_from_prev = timeout;
	timeout_obj->wait_q = (sys_dlist_t *)wait_q;
	sys_dlist_insert_at(timeout_q, (void *)timeout_obj,
			    _is_timeout_insert_point,
			    &timeout_obj->delta_ticks_from_prev);

	K_DEBUG("timeout_q %p after:  head: %p, tail: %p\n",
		&_timeout_q,
		sys_dlist_peek_head(&_timeout_q),
		_timeout_q.tail);

	K_DEBUG("timeout   %p after:  next: %p, prev: %p\n",
		timeout_obj, timeout_obj->node.next, timeout_obj->node.prev);
}

/*
 * Put thread on timeout queue. Record wait queue if any.
 *
 * Cannot handle timeout == 0 and timeout == K_FOREVER.
 *
 * Must be called with interrupts locked.
 */

static inline void _add_thread_timeout(struct k_thread *thread,
				       _wait_q_t *wait_q, int32_t timeout)
{
	_add_timeout(thread, &thread->base.timeout, wait_q, timeout);
}

/* find the closest deadline in the timeout queue */

static inline int32_t _get_next_timeout_expiry(void)
{
	struct _timeout *t = (struct _timeout *)
			     sys_dlist_peek_head(&_timeout_q);

	return t ? t->delta_ticks_from_prev : K_FOREVER;
}

#ifdef __cplusplus
}
#endif

#endif /* _kernel_include_timeout_q__h_ */
