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

/* initialize the nano timeouts part of TCS when enabled in the kernel */

static inline void _nano_timeout_tcs_init(struct tcs *tcs)
{
	/*
	 * Must be initialized here and when dequeueing a timeout so that code
	 * not dealing with timeouts does not have to handle this, such as when
	 * waiting forever on a semaphore.
	 */
	tcs->nano_timeout.delta_ticks_from_prev = -1;

	/*
	 * These are initialized when enqueing on the timeout queue:
	 *
	 *   tcs->nano_timeout.node.next
	 *   tcs->nano_timeout.node.prev
	 *   tcs->nano_timeout.wait_q
	 */
}

/*
 * Handle one expired timeout.
 * This removes the fiber from the timeout queue head, and also removes it
 * from the wait queue it is on if waiting for an object. In that case, it
 * also sets the return value to 0/NULL.
 */

static inline struct _nano_timeout *_nano_timeout_handle_one_timeout(
	sys_dlist_t *timeout_q)
{
	struct _nano_timeout *t = (void *)sys_dlist_get(timeout_q);
	struct tcs *tcs = CONTAINER_OF(t, struct tcs, nano_timeout);

	if (tcs->nano_timeout.wait_q) {
		_nano_timeout_remove_tcs_from_wait_q(tcs);
		fiberRtnValueSet(tcs, (unsigned int)0);
	}
	_nano_fiber_schedule(tcs);
	t->delta_ticks_from_prev = -1;

	return (struct _nano_timeout *)sys_dlist_peek_head(timeout_q);
}

/* loop over all expired timeouts and handle them one by one */
static inline void _nano_timeout_handle_timeouts(void)
{
	sys_dlist_t *timeout_q = &_nanokernel.timeout_q;
	struct _nano_timeout *next;

	next = (struct _nano_timeout *)sys_dlist_peek_head(timeout_q);
	while (next && next->delta_ticks_from_prev == 0) {
		next = _nano_timeout_handle_one_timeout(timeout_q);
	}
}

/* abort a timeout for a specific fiber */
static inline void _nano_timeout_abort(struct tcs *tcs)
{
	sys_dlist_t *timeout_q = &_nanokernel.timeout_q;
	struct _nano_timeout *t = &tcs->nano_timeout;

	if (-1 == t->delta_ticks_from_prev) {
		return;
	}

	if (!sys_dlist_is_tail(timeout_q, &t->node)) {
		struct _nano_timeout *next =
			(struct _nano_timeout *)sys_dlist_peek_next(timeout_q,
								    &t->node);
		next->delta_ticks_from_prev += t->delta_ticks_from_prev;
	}
	sys_dlist_remove(&t->node);
	t->delta_ticks_from_prev = -1;
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
static int _nano_timeout_insert_point_test(sys_dnode_t *test, void *timeout)
{
	struct _nano_timeout *t = (void *)test;
	int32_t *timeout_to_insert = timeout;

	if (*timeout_to_insert > t->delta_ticks_from_prev) {
		*timeout_to_insert -= t->delta_ticks_from_prev;
		return 0;
	}

	t->delta_ticks_from_prev -= *timeout_to_insert;
	return 1;
}

/* put a fiber on the timeout queue and record its wait queue */
static inline void _nano_timeout_add(struct tcs *tcs,
				     struct _nano_queue *wait_q,
				     int32_t timeout)
{
	sys_dlist_t *timeout_q = &_nanokernel.timeout_q;
	struct _nano_timeout *t = &tcs->nano_timeout;

	t->delta_ticks_from_prev = timeout;
	t->wait_q = wait_q;
	sys_dlist_insert_at(timeout_q, (void *)t,
						_nano_timeout_insert_point_test,
						&t->delta_ticks_from_prev);
}

/* find the closest deadline in the timeout queue */
static inline uint32_t _nano_get_earliest_timeouts_deadline(void)
{
	sys_dlist_t *q = &_nanokernel.timeout_q;
	struct _nano_timeout *t = (struct _nano_timeout *)sys_dlist_peek_head(q);

	return t ? t->delta_ticks_from_prev : TICKS_UNLIMITED;
}

#endif /* _kernel_nanokernel_include_timeout_q__h_ */
