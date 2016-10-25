/*
 * Copyright (c) 2010-2012, 2014-2015 Wind River Systems, Inc.
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

/**
 * @file
 * @brief Architecture-independent private nanokernel APIs
 *
 * This file contains private nanokernel APIs that are not
 * architecture-specific.
 */

#ifndef _NANO_INTERNAL__H_
#define _NANO_INTERNAL__H_

#ifndef _ASMLANGUAGE

#ifdef __cplusplus
extern "C" {
#endif

#include <nanokernel.h>

/* Early boot functions */

void _bss_zero(void);
#ifdef CONFIG_XIP
void _data_copy(void);
#else
static inline void _data_copy(void)
{
	/* Do nothing */
}
#endif
FUNC_NORETURN void _Cstart(void);

/* helper type alias for thread control structure */

typedef struct tcs tTCS;

/* thread entry point declarations */

typedef void *_thread_arg_t;
typedef void (*_thread_entry_t)(_thread_arg_t arg1,
							  _thread_arg_t arg2,
							  _thread_arg_t arg3);

extern void _thread_entry(_thread_entry_t,
			     _thread_arg_t,
			     _thread_arg_t,
			     _thread_arg_t);

extern void _new_thread(char *pStack, unsigned stackSize,
						void *uk_task_ptr, _thread_entry_t pEntry,
						_thread_arg_t arg1, _thread_arg_t arg2,
						_thread_arg_t arg3,
						int prio, unsigned options);

/* context switching and scheduling-related routines */

extern void _nano_fiber_ready(struct tcs *tcs);
extern void _nano_fiber_swap(void);

extern unsigned int _Swap(unsigned int);

/* set and clear essential fiber/task flag */

extern void _thread_essential_set(void);
extern void _thread_essential_clear(void);

/* clean up when a thread is aborted */

#if defined(CONFIG_THREAD_MONITOR)
extern void _thread_monitor_exit(struct tcs *tcs);
#else
#define _thread_monitor_exit(tcs) \
	do {/* nothing */    \
	} while (0)
#endif /* CONFIG_THREAD_MONITOR */

/* special nanokernel object APIs */

struct nano_lifo;

extern void *_nano_fiber_lifo_get_panic(struct nano_lifo *lifo);

#ifdef CONFIG_MICROKERNEL
extern void _task_nop(void);
extern void _nano_nop(void);
extern int  _nano_unpend_tasks(struct _nano_queue *queue);
extern void _nano_task_ready(void *ptr);
extern void _nano_timer_task_ready(void *task);

#define _TASK_PENDQ_INIT(queue)		_nano_wait_q_init(queue)
#define _NANO_UNPEND_TASKS(queue)                        \
	do {                                             \
		if (_nano_unpend_tasks(queue) != 0) {    \
			_nano_nop();                     \
		}                                        \
	} while (0)


#define _TASK_NANO_UNPEND_TASKS(queue)	                 \
	do {                                             \
		if (_nano_unpend_tasks(queue) != 0) {    \
			_task_nop();                     \
		}                                        \
	} while (0)

#define _NANO_TASK_READY(tcs)		_nano_task_ready(tcs->uk_task_ptr)
#define _NANO_TIMER_TASK_READY(tcs)	_nano_timer_task_ready(tcs->uk_task_ptr)
#define _IS_MICROKERNEL_TASK(tcs)	((tcs)->uk_task_ptr != NULL)
#define _IS_IDLE_TASK() \
	(task_priority_get() == (CONFIG_NUM_TASK_PRIORITIES - 1))
#else
#define _TASK_PENDQ_INIT(queue)		do { } while (0)
#define _NANO_UNPEND_TASKS(queue)	do { } while (0)
#define _TASK_NANO_UNPEND_TASKS(queue)	do { } while (0)
#define _NANO_TASK_READY(tcs)		do { } while (0)
#define _NANO_TIMER_TASK_READY(tcs)	do { } while (0)
#define _IS_MICROKERNEL_TASK(tcs)	(0)
#endif

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* _NANO_INTERNAL__H_ */
