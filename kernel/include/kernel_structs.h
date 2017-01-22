/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _kernel_structs__h_
#define _kernel_structs__h_

#include <kernel.h>

#if !defined(_ASMLANGUAGE)
#include <atomic.h>
#include <misc/dlist.h>
#endif

/*
 * Bitmask definitions for the struct k_thread.thread_state field.
 *
 * Must be before kerneL_arch_data.h because it might need them to be already
 * defined.
 */


/* states: common uses low bits, arch-specific use high bits */

/* Not a real thread */
#define _THREAD_DUMMY (1 << 0)

/* Thread is waiting on an object */
#define _THREAD_PENDING (1 << 1)

/* Thread has not yet started */
#define _THREAD_PRESTART (1 << 2)

/* Thread has terminated */
#define _THREAD_DEAD (1 << 3)

/* Thread is suspended */
#define _THREAD_SUSPENDED (1 << 4)

/* end - states */


/* lowest value of _thread_base.preempt at which a thread is non-preemptible */
#define _NON_PREEMPT_THRESHOLD 0x0080

/* highest value of _thread_base.preempt at which a thread is preemptible */
#define _PREEMPT_THRESHOLD (_NON_PREEMPT_THRESHOLD - 1)

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

	/* user facing 'thread options'; values defined in include/kernel.h */
	uint8_t user_options;

	/* thread state */
	uint8_t thread_state;

	/*
	 * scheduler lock count and thread priority
	 *
	 * These two fields control the preemptibility of a thread.
	 *
	 * When the scheduler is locked, sched_locked is decremented, which
	 * means that the scheduler is locked for values from 0xff to 0x01. A
	 * thread is coop if its prio is negative, thus 0x80 to 0xff when
	 * looked at the value as unsigned.
	 *
	 * By putting them end-to-end, this means that a thread is
	 * non-preemptible if the bundled value is greater than or equal to
	 * 0x0080.
	 */
	union {
		struct {
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
			volatile uint8_t sched_locked;
			int8_t prio;
#else /* LITTLE and PDP */
			int8_t prio;
			volatile uint8_t sched_locked;
#endif
		};
		uint16_t preempt;
	};

	/* data returned by APIs */
	void *swap_data;

#ifdef CONFIG_SYS_CLOCK_EXISTS
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

	/* always contains next thread to run: cannot be NULL */
	struct k_thread *cache;

	/* bitmap of priorities that contain at least one ready thread */
	uint32_t prio_bmap[K_NUM_PRIO_BITMAPS];

	/* ready queues, one per priority */
	sys_dlist_t q[K_NUM_PRIORITIES];
};

typedef struct _ready_q _ready_q_t;

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

extern void _init_thread_base(struct _thread_base *thread_base,
			      int priority, uint32_t initial_state,
			      unsigned int options);

#endif /* _ASMLANGUAGE */

#endif /* _kernel_structs__h_ */
