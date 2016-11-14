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

#ifndef _kernel_structs__h_
#define _kernel_structs__h_

#if !defined(_ASMLANGUAGE)
#include <kernel.h>
#include <atomic.h>
#include <misc/dlist.h>
#endif

#include <kernel_arch_data.h>

#if !defined(_ASMLANGUAGE)

#ifdef CONFIG_THREAD_MONITOR
struct __thread_entry {
	_thread_entry_t pEntry;
	void *parameter1;
	void *parameter2;
	void *parameter3;
};
#endif

/* can be used for creating 'dummy' threads, e.g. for pending on objects */
struct _thread_base {

	/* this thread's entry in a ready/wait queue */
	sys_dnode_t k_q_node;

	/* execution flags */
	int flags;

	/* thread priority used to sort linked list */
	int prio;

	/* scheduler lock count */
	atomic_t sched_locked;

	/* data returned by APIs */
	void *swap_data;

#ifdef CONFIG_NANO_TIMEOUTS
	/* this thread's entry in a timeout queue */
	struct _timeout timeout;
#endif

};

typedef struct _thread_base _thread_base_t;

struct k_thread {

	struct _thread_base base;

	/* defined by the architecture, but all archs need these */
	struct _caller_saved caller_saved;
	struct _callee_saved callee_saved;

	/* static thread init data */
	void *init_data;

	/* abort function */
	void (*fn_abort)(void);

#if defined(CONFIG_THREAD_MONITOR)
	/* thread entry and parameters description */
	struct __thread_entry *entry;

	/* next item in list of all threads */
	struct k_thread *next_thread;
#endif

#ifdef CONFIG_THREAD_CUSTOM_DATA
	/* crude thread-local storage */
	void *custom_data;
#endif

#ifdef CONFIG_ERRNO
	/* per-thread errno variable */
	int errno_var;
#endif

	/* arch-specifics: must always be at the end */
	struct _thread_arch arch;
};

typedef struct k_thread _thread_t;

struct _ready_q {

	/* next thread to run if known, NULL otherwise */
	struct k_thread *cache;

	/* bitmap of priorities that contain at least one ready thread */
	uint32_t prio_bmap[1];

	/* ready queues, one per priority */
	sys_dlist_t q[K_NUM_PRIORITIES];
};

struct _kernel {

	/* nested interrupt count */
	uint32_t nested;

	/* interrupt stack pointer base */
	char *irq_stack;

	/* currently scheduled thread */
	struct k_thread *current;

#ifdef CONFIG_SYS_CLOCK_EXISTS
	/* queue of timeouts */
	sys_dlist_t timeout_q;
#endif

#ifdef CONFIG_SYS_POWER_MANAGEMENT
	int32_t idle; /* Number of ticks for kernel idling */
#endif

	/*
	 * ready queue: can be big, keep after small fields, since some
	 * assembly (e.g. ARC) are limited in the encoding of the offset
	 */
	struct _ready_q ready_q;

#ifdef CONFIG_FP_SHARING
	/*
	 * A 'current_sse' field does not exist in addition to the 'current_fp'
	 * field since it's not possible to divide the IA-32 non-integer
	 * registers into 2 distinct blocks owned by differing threads.  In
	 * other words, given that the 'fxnsave/fxrstor' instructions
	 * save/restore both the X87 FPU and XMM registers, it's not possible
	 * for a thread to only "own" the XMM registers.
	 */

	/* thread (fiber or task) that owns the FP regs */
	struct k_thread *current_fp;
#endif

#if defined(CONFIG_THREAD_MONITOR)
	struct k_thread *threads; /* singly linked list of ALL fiber+tasks */
#endif

	/* arch-specific part of _kernel */
	struct _kernel_arch arch;
};

typedef struct _kernel _kernel_t;

extern struct _kernel _kernel;

#define _current _kernel.current
#define _ready_q _kernel.ready_q
#define _timeout_q _kernel.timeout_q
#define _threads _kernel.threads

#include <kernel_arch_func.h>

static ALWAYS_INLINE void
_set_thread_return_value_with_data(struct k_thread *thread,
				   unsigned int value,
				   void *data)
{
	_set_thread_return_value(thread, value);
	thread->base.swap_data = data;
}

#endif /* _ASMLANGUAGE */

#endif /* _kernel_structs__h_ */
