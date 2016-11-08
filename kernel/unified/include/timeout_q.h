/** @file
 * @brief timeout queue for fibers on nanokernel objects
 *
 * This file is meant to be included by nanokernel/include/wait_q.h only
 */

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

#ifndef _kernel_nanokernel_include_timeout_q__h_
#define _kernel_nanokernel_include_timeout_q__h_

#include <misc/dlist.h>

#ifdef __cplusplus
extern "C" {
#endif

/* initialize the nano timeouts part of k_thread when enabled in the kernel */

static inline void _init_timeout(struct _timeout *t, _timeout_func_t func)
{
	/*
	 * Must be initialized here and when dequeueing a timeout so that code
	 * not dealing with timeouts does not have to handle this, such as when
	 * waiting forever on a semaphore.
	 */
	t->delta_ticks_from_prev = -1;

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

static inline void _init_thread_timeout(struct k_thread *thread)
{
	_init_timeout(&thread->base.timeout, NULL);
}

/*
 * XXX - backwards compatibility until the arch part is updated to call
 * _init_thread_timeout()
 */
static inline void _nano_timeout_thread_init(struct k_thread *thread)
{
	_init_thread_timeout(thread);
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
 * Handle one expired timeout.
 *
 * This removes the thread from the timeout queue head, and also removes it
 * from the wait queue it is on if waiting for an object. In that case,
 * the return value is kept as -EAGAIN, set previously in _Swap().
 *
 * Must be called with interrupts locked.
 */

static inline struct _timeout *_handle_one_timeout(
	sys_dlist_t *timeout_q)
{
	struct _timeout *t = (void *)sys_dlist_get(timeout_q);
	struct k_thread *thread = t->thread;

	K_DEBUG("timeout %p\n", t);
	if (thread != NULL) {
		_unpend_thread_timing_out(thread, t);
		_ready_thread(thread);
	} else if (t->func) {
		t->func(t);
	}
	/*
	 * Note: t->func() may add timeout again. Make sure that
	 * delta_ticks_from_prev is set to -1 only if timeout is
	 * still expired (delta_ticks_from_prev == 0)
	 */
	if (t->delta_ticks_from_prev == 0) {
		t->delta_ticks_from_prev = -1;
	}

	return (struct _timeout *)sys_dlist_peek_head(timeout_q);
}

/*
 * Loop over all expired timeouts and handle them one by one.
 * Must be called with interrupts locked.
 */

static inline void _handle_timeouts(void)
{
	sys_dlist_t *timeout_q = &_timeout_q;
	struct _timeout *next;

	next = (struct _timeout *)sys_dlist_peek_head(timeout_q);
	while (next && next->delta_ticks_from_prev == 0) {
		next = _handle_one_timeout(timeout_q);
	}
}

/* returns 0 in success and -1 if the timer has expired */

static inline int _abort_timeout(struct _timeout *t)
{
	sys_dlist_t *timeout_q = &_timeout_q;

	if (-1 == t->delta_ticks_from_prev) {
		return -1;
	}

	if (!sys_dlist_is_tail(timeout_q, &t->node)) {
		struct _timeout *next =
			(struct _timeout *)sys_dlist_peek_next(timeout_q,
								    &t->node);
		next->delta_ticks_from_prev += t->delta_ticks_from_prev;
	}
	sys_dlist_remove(&t->node);
	t->delta_ticks_from_prev = -1;

	return 0;
}

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

#endif /* _kernel_nanokernel_include_timeout_q__h_ */
