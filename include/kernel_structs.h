/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * The purpose of this file is to provide essential/minimal kernel structure
 * definitions, so that they can be used without including kernel.h.
 *
 * The following rules must be observed:
 *  1. kernel_structs.h shall not depend on kernel.h both directly and
 *    indirectly (i.e. it shall not include any header files that include
 *    kernel.h in their dependency chain).
 *  2. kernel.h shall imply kernel_structs.h, such that it shall not be
 *    necessary to include kernel_structs.h explicitly when kernel.h is
 *    included.
 */

#ifndef ZEPHYR_KERNEL_INCLUDE_KERNEL_STRUCTS_H_
#define ZEPHYR_KERNEL_INCLUDE_KERNEL_STRUCTS_H_

#if !defined(_ASMLANGUAGE)
#include <sys/atomic.h>
#include <zephyr/types.h>
#include <sched_priq.h>
#include <sys/dlist.h>
#include <sys/util.h>
#include <sys/sys_heap.h>
#endif

#define K_NUM_PRIORITIES \
	(CONFIG_NUM_COOP_PRIORITIES + CONFIG_NUM_PREEMPT_PRIORITIES + 1)

#define K_NUM_PRIO_BITMAPS ((K_NUM_PRIORITIES + 31) >> 5)

/*
 * Bitmask definitions for the struct k_thread.thread_state field.
 *
 * Must be before kerneL_arch_data.h because it might need them to be already
 * defined.
 */

/* states: common uses low bits, arch-specific use high bits */

/* Not a real thread */
#define _THREAD_DUMMY (BIT(0))

/* Thread is waiting on an object */
#define _THREAD_PENDING (BIT(1))

/* Thread has not yet started */
#define _THREAD_PRESTART (BIT(2))

/* Thread has terminated */
#define _THREAD_DEAD (BIT(3))

/* Thread is suspended */
#define _THREAD_SUSPENDED (BIT(4))

/* Thread is being aborted */
#define _THREAD_ABORTING (BIT(5))

/* Thread is present in the ready queue */
#define _THREAD_QUEUED (BIT(7))

/* end - states */

#ifdef CONFIG_STACK_SENTINEL
/* Magic value in lowest bytes of the stack */
#define STACK_SENTINEL 0xF0F0F0F0
#endif

/* lowest value of _thread_base.preempt at which a thread is non-preemptible */
#define _NON_PREEMPT_THRESHOLD 0x0080U

/* highest value of _thread_base.preempt at which a thread is preemptible */
#define _PREEMPT_THRESHOLD (_NON_PREEMPT_THRESHOLD - 1U)

#if !defined(_ASMLANGUAGE)

struct _ready_q {
#ifndef CONFIG_SMP
	/* always contains next thread to run: cannot be NULL */
	struct k_thread *cache;
#endif

#if defined(CONFIG_SCHED_DUMB)
	sys_dlist_t runq;
#elif defined(CONFIG_SCHED_SCALABLE)
	struct _priq_rb runq;
#elif defined(CONFIG_SCHED_MULTIQ)
	struct _priq_mq runq;
#endif
};

typedef struct _ready_q _ready_q_t;

struct _cpu {
	/* nested interrupt count */
	uint32_t nested;

	/* interrupt stack pointer base */
	char *irq_stack;

	/* currently scheduled thread */
	struct k_thread *current;

	/* one assigned idle thread per CPU */
	struct k_thread *idle_thread;

	/* If non-null, self-aborted thread that needs cleanup */
	struct k_thread *pending_abort;

#if (CONFIG_NUM_METAIRQ_PRIORITIES > 0) && (CONFIG_NUM_COOP_PRIORITIES > 0)
	/* Coop thread preempted by current metairq, or NULL */
	struct k_thread *metairq_preempted;
#endif

#ifdef CONFIG_TIMESLICING
	/* number of ticks remaining in current time slice */
	int slice_ticks;
#endif

	uint8_t id;

#ifdef CONFIG_SMP
	/* True when _current is allowed to context switch */
	uint8_t swap_ok;
#endif
};

typedef struct _cpu _cpu_t;

struct z_kernel {
	struct _cpu cpus[CONFIG_MP_NUM_CPUS];

#ifdef CONFIG_SYS_CLOCK_EXISTS
	/* queue of timeouts */
	sys_dlist_t timeout_q;
#endif

#ifdef CONFIG_PM
	int32_t idle; /* Number of ticks for kernel idling */
#endif

	/*
	 * ready queue: can be big, keep after small fields, since some
	 * assembly (e.g. ARC) are limited in the encoding of the offset
	 */
	struct _ready_q ready_q;

#ifdef CONFIG_FPU_SHARING
	/*
	 * A 'current_sse' field does not exist in addition to the 'current_fp'
	 * field since it's not possible to divide the IA-32 non-integer
	 * registers into 2 distinct blocks owned by differing threads.  In
	 * other words, given that the 'fxnsave/fxrstor' instructions
	 * save/restore both the X87 FPU and XMM registers, it's not possible
	 * for a thread to only "own" the XMM registers.
	 */

	/* thread that owns the FP regs */
	struct k_thread *current_fp;
#endif

#if defined(CONFIG_THREAD_MONITOR)
	struct k_thread *threads; /* singly linked list of ALL threads */
#endif
};

typedef struct z_kernel _kernel_t;

extern struct z_kernel _kernel;

#ifdef CONFIG_SMP

/* True if the current context can be preempted and migrated to
 * another SMP CPU.
 */
bool z_smp_cpu_mobile(void);

#define _current_cpu ({ __ASSERT_NO_MSG(!z_smp_cpu_mobile()); \
			arch_curr_cpu(); })
#define _current k_current_get()

#else
#define _current_cpu (&_kernel.cpus[0])
#define _current _kernel.cpus[0].current
#endif

#define _timeout_q _kernel.timeout_q

/* kernel wait queue record */

#ifdef CONFIG_WAITQ_SCALABLE

typedef struct {
	struct _priq_rb waitq;
} _wait_q_t;

extern bool z_priq_rb_lessthan(struct rbnode *a, struct rbnode *b);

#define Z_WAIT_Q_INIT(wait_q) { { { .lessthan_fn = z_priq_rb_lessthan } } }

#else

typedef struct {
	sys_dlist_t waitq;
} _wait_q_t;

#define Z_WAIT_Q_INIT(wait_q) { SYS_DLIST_STATIC_INIT(&(wait_q)->waitq) }

#endif

/* kernel timeout record */

struct _timeout;
typedef void (*_timeout_func_t)(struct _timeout *t);

struct _timeout {
	sys_dnode_t node;
	_timeout_func_t fn;
#ifdef CONFIG_TIMEOUT_64BIT
	/* Can't use k_ticks_t for header dependency reasons */
	int64_t dticks;
#else
	int32_t dticks;
#endif
};

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_KERNEL_INCLUDE_KERNEL_STRUCTS_H_ */
