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

#ifdef __cplusplus
extern "C" {
#endif

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

struct tcs *_nano_wait_q_remove(struct _nano_queue *wait_q);

/* put current fiber on specified wait queue */
static inline void _nano_wait_q_put(struct _nano_queue *wait_q)
{
	((struct tcs *)wait_q->tail)->link = _nanokernel.current;
	wait_q->tail = _nanokernel.current;
}

#if defined(CONFIG_NANO_TIMEOUTS)

#include <timeout_q.h>

	#define _NANO_TIMEOUT_TICK_GET()  sys_tick_get()

	#define _NANO_TIMEOUT_ADD(pq, ticks)                                 \
		do {                                                             \
			if ((ticks) != TICKS_UNLIMITED) {                            \
				_nano_timeout_add(_nanokernel.current, (pq), (ticks));   \
			}                                                            \
		} while (0)
	#define _NANO_TIMEOUT_SET_TASK_TIMEOUT(ticks) \
		_nanokernel.task_timeout = (ticks)

	#define _NANO_TIMEOUT_UPDATE(timeout, limit, cur_ticks)               \
		do {                                                          \
			if ((timeout) != TICKS_UNLIMITED) {                   \
				(timeout) = (int32_t)((limit) - (cur_ticks)); \
			}                                                     \
		} while (0)

#elif defined(CONFIG_NANO_TIMERS)
#include <timeout_q.h>
	#define _nano_timeout_tcs_init(tcs) do { } while ((0))
	#define _nano_timeout_abort(tcs) do { } while ((0))

	#define _NANO_TIMEOUT_TICK_GET()  0
	#define _NANO_TIMEOUT_ADD(pq, ticks) do { } while (0)
	#define _NANO_TIMEOUT_SET_TASK_TIMEOUT(ticks) do { } while ((0))
	#define _NANO_TIMEOUT_UPDATE(timeout, limit, cur_ticks) do { } while (0)
#else
	#define _nano_timeout_tcs_init(tcs) do { } while ((0))
	#define _nano_timeout_abort(tcs) do { } while ((0))
	#define _nano_get_earliest_timeouts_deadline() ((uint32_t)TICKS_UNLIMITED)
	#define _NANO_TIMEOUT_TICK_GET()  0
	#define _NANO_TIMEOUT_ADD(pq, ticks) do { } while (0)
	#define _NANO_TIMEOUT_SET_TASK_TIMEOUT(ticks) do { } while ((0))
	#define _NANO_TIMEOUT_UPDATE(timeout, limit, cur_ticks) do { } while (0)
#endif

#ifdef CONFIG_MICROKERNEL
	extern void _task_nano_pend_task(struct _nano_queue *, int32_t);
	extern uint32_t task_priority_get(void);

	#define _NANO_OBJECT_WAIT(queue, data, timeout, key)                  \
		do {                                                          \
			if (_IS_IDLE_TASK()) {                                \
				_NANO_TIMEOUT_SET_TASK_TIMEOUT(timeout);      \
				nano_cpu_atomic_idle(key);                    \
				key = irq_lock();                             \
			} else {                                              \
				_task_nano_pend_task(queue, timeout);         \
			}                                                     \
		} while (0)

#else
	#define _NANO_OBJECT_WAIT(queue, data, timeout, key)     \
		do {                                             \
			_NANO_TIMEOUT_SET_TASK_TIMEOUT(timeout); \
			nano_cpu_atomic_idle(key);               \
			key = irq_lock();                        \
		} while (0)

#endif

#ifdef __cplusplus
}
#endif

#endif /* _kernel_nanokernel_include_wait_q__h_ */
