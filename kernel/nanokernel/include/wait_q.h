/* wait queue for multiple fibers on nanokernel objects */

/*
 * Copyright (c) 2015 Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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
