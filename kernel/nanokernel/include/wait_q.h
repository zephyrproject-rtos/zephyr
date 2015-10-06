/* wait queue for multiple fibers on nanokernel objects */

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

#ifndef _kernel_nanokernel_include_wait_q__h_
#define _kernel_nanokernel_include_wait_q__h_

#include <nano_private.h>

/* reset a wait queue, call during operation */
static inline void _nano_wait_q_reset(struct _nano_queue *wait_q)
{
	wait_q->head = (void *)0;
	wait_q->tail = (void *)&(wait_q->head);
}

/* initialize a wait queue: call only during object initialization */
static inline void _nano_wait_q_init(struct _nano_queue *wait_q)
{
	_nano_wait_q_reset(wait_q);
}

/*
 * Remove first fiber from a wait queue and put it on the ready queue, knowing
 * that the wait queue is not empty.
 */
static inline
struct tcs *_nano_wait_q_remove_no_check(struct _nano_queue *wait_q)
{
	struct tcs *tcs = wait_q->head;

	if (wait_q->tail == wait_q->head) {
		_nano_wait_q_reset(wait_q);
	} else {
		wait_q->head = tcs->link;
	}
	tcs->link = 0;

	_nano_fiber_schedule(tcs);
	return tcs;
}

/*
 * Remove first fiber from a wait queue and put it on the ready queue.
 * Abort and return NULL if the wait queue is empty.
 */
static inline struct tcs *_nano_wait_q_remove(struct _nano_queue *wait_q)
{
	return wait_q->head ? _nano_wait_q_remove_no_check(wait_q) : NULL;
}

/* put current fiber on specified wait queue */
static inline void _nano_wait_q_put(struct _nano_queue *wait_q)
{
	((struct tcs *)wait_q->tail)->link = _nanokernel.current;
	wait_q->tail = _nanokernel.current;
}

#ifdef CONFIG_NANO_TIMEOUTS
static inline void _nano_timeout_remove_tcs_from_wait_q(struct tcs *tcs)
{
	struct _nano_queue *wait_q = tcs->nano_timeout.wait_q;

	if (wait_q->head == tcs) {
		if (wait_q->tail == wait_q->head) {
			_nano_wait_q_reset(wait_q);
		} else {
			wait_q->head = tcs->link;
		}
	} else {
		struct tcs *prev = wait_q->head;

		while (prev->link != tcs) {
			prev = prev->link;
		}
		prev->link = tcs->link;
		if (wait_q->tail == tcs) {
			wait_q->tail = prev;
		}
	}
}
#include <timeout_q.h>
#else
	#define _nano_timeout_tcs_init(tcs) do { } while ((0))
	#define _nano_timeout_abort(tcs) do { } while ((0))
	#define _nano_get_earliest_timeouts_deadline() ((uint32_t)TICKS_UNLIMITED)
#endif

#endif /* _kernel_nanokernel_include_wait_q__h_ */
