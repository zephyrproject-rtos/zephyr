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

#ifdef CONFIG_KERNEL_V2
#include <misc/dlist.h>
#include <sched.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if 0
/* reset a wait queue, call during operation */
static inline void _nano_wait_q_reset(struct _nano_queue *wait_q)
{
	sys_dlist_init((sys_dlist_t *)wait_q);
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
	struct tcs *tcs = (struct tcs *)sys_dlist_get((sys_dlist_t *)wait_q);

	_ready_thread(tcs);

	return tcs;
}

/*
 * Remove first fiber from a wait queue and put it on the ready queue.
 * Abort and return NULL if the wait queue is empty.
 */
static inline struct tcs *_nano_wait_q_remove(struct _nano_queue *wait_q)
{
	return _nano_wait_q_remove_no_check(wait_q);
}

/* put current fiber on specified wait queue */
static inline void _nano_wait_q_put(struct _nano_queue *wait_q)
{
	/* unused */
}
#endif

#if defined(CONFIG_NANO_TIMEOUTS)
static inline void _timeout_remove_tcs_from_wait_q(struct tcs *tcs)
{
	_unpend_thread(tcs);
	tcs->timeout.wait_q = NULL;
}
#include <timeout_q.h>

	#define _TIMEOUT_ADD(thread, pq, ticks) \
		do { \
			if ((ticks) != TICKS_UNLIMITED) { \
				_timeout_add(thread, pq, ticks); \
			} \
		} while (0)

#elif defined(CONFIG_NANO_TIMERS)
#include <timeout_q.h>
	#define _timeout_tcs_init(tcs) do { } while ((0))
	#define _timeout_abort(tcs) do { } while ((0))

	#define _TIMEOUT_ADD(thread, pq, ticks) do { } while (0)
#else
	#define _timeout_tcs_init(tcs) do { } while ((0))
	#define _timeout_abort(tcs) do { } while ((0))
	#define _timeout_get_next_expiry() (K_FOREVER)

	#define _TIMEOUT_ADD(thread, pq, ticks) do { } while (0)
#endif

#ifdef __cplusplus
}
#endif

#endif /* _kernel_nanokernel_include_wait_q__h_ */
