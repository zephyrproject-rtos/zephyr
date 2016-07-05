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

int _do_nano_timeout_abort(struct _nano_timeout *t);
void _do_nano_timeout_add(struct tcs *tcs, struct _nano_timeout *t,
				struct _nano_queue *wait_q, int32_t timeout);
static inline void _nano_timeout_init(struct _nano_timeout *t,
				      _nano_timeout_func_t func)
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
	 * Must be initialized here, so the _nano_timeout_handle_one_timeout()
	 * routine can check if there is a fiber waiting on this timeout
	 */
	t->tcs = NULL;

	/*
	 * Set callback function
	 */
	t->func = func;
}

#if defined(CONFIG_NANO_TIMEOUTS)

/* initialize the nano timeouts part of TCS when enabled in the kernel */

static inline void _nano_timeout_tcs_init(struct tcs *tcs)
{
	_nano_timeout_init(&tcs->nano_timeout, NULL);

	/*
	 * These are initialized when enqueing on the timeout queue:
	 *
	 *   tcs->nano_timeout.node.next
	 *   tcs->nano_timeout.node.prev
	 */
}

/* abort a timeout for a specified fiber */
static inline int _nano_timeout_abort(struct tcs *tcs)
{
	return _do_nano_timeout_abort(&tcs->nano_timeout);
}

/* put a fiber on the timeout queue and record its wait queue */
static inline void _nano_timeout_add(struct tcs *tcs,
				     struct _nano_queue *wait_q,
				     int32_t timeout)
{
	_do_nano_timeout_add(tcs,  &tcs->nano_timeout, wait_q, timeout);
}

#else
#define _nano_timeout_object_dequeue(tcs, t) do { } while (0)
#endif /* CONFIG_NANO_TIMEOUTS */


void _nano_timeout_handle_timeouts(void);

static inline int _nano_timer_timeout_abort(struct _nano_timeout *t)
{
	return _do_nano_timeout_abort(t);
}

static inline void _nano_timer_timeout_add(struct _nano_timeout *t,
				     struct _nano_queue *wait_q,
				     int32_t timeout)
{
	_do_nano_timeout_add(NULL, t, wait_q, timeout);
}

uint32_t _nano_get_earliest_timeouts_deadline(void);

#ifdef __cplusplus
}
#endif

#endif /* _kernel_nanokernel_include_timeout_q__h_ */
