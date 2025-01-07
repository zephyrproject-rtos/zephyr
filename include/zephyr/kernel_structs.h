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
#include <zephyr/sys/atomic.h>
#include <zephyr/types.h>
#include <zephyr/sys/dlist.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/sys_heap.h>
#include <zephyr/arch/structs.h>
#include <zephyr/kernel/stats.h>
#include <zephyr/kernel/obj_core.h>
#include <zephyr/sys/rb.h>
#endif

#define K_NUM_THREAD_PRIO (CONFIG_NUM_PREEMPT_PRIORITIES + CONFIG_NUM_COOP_PRIORITIES + 1)
#define PRIQ_BITMAP_SIZE  (DIV_ROUND_UP(K_NUM_THREAD_PRIO, BITS_PER_LONG))

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Bitmask definitions for the struct k_thread.thread_state field.
 *
 * Must be before kernel_arch_data.h because it might need them to be already
 * defined.
 */

/* states: common uses low bits, arch-specific use high bits */

/* Not a real thread */
#define _THREAD_DUMMY (BIT(0))

/* Thread is waiting on an object */
#define _THREAD_PENDING (BIT(1))

/* Thread is sleeping */
#define _THREAD_SLEEPING (BIT(2))

/* Thread has terminated */
#define _THREAD_DEAD (BIT(3))

/* Thread is suspended */
#define _THREAD_SUSPENDED (BIT(4))

/* Thread is in the process of aborting */
#define _THREAD_ABORTING (BIT(5))

/* Thread is in the process of suspending */
#define _THREAD_SUSPENDING (BIT(6))

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

/* Two abstractions are defined here for "thread priority queues".
 *
 * One is a "dumb" list implementation appropriate for systems with
 * small numbers of threads and sensitive to code size.  It is stored
 * in sorted order, taking an O(N) cost every time a thread is added
 * to the list.  This corresponds to the way the original _wait_q_t
 * abstraction worked and is very fast as long as the number of
 * threads is small.
 *
 * The other is a balanced tree "fast" implementation with rather
 * larger code size (due to the data structure itself, the code here
 * is just stubs) and higher constant-factor performance overhead, but
 * much better O(logN) scaling in the presence of large number of
 * threads.
 *
 * Each can be used for either the wait_q or system ready queue,
 * configurable at build time.
 */

struct _priq_rb {
	struct rbtree tree;
	int next_order_key;
};


/* Traditional/textbook "multi-queue" structure.  Separate lists for a
 * small number (max 32 here) of fixed priorities.  This corresponds
 * to the original Zephyr scheduler.  RAM requirements are
 * comparatively high, but performance is very fast.  Won't work with
 * features like deadline scheduling which need large priority spaces
 * to represent their requirements.
 */
struct _priq_mq {
	sys_dlist_t queues[K_NUM_THREAD_PRIO];
	unsigned long bitmask[PRIQ_BITMAP_SIZE];
#ifndef CONFIG_SMP
	unsigned int cached_queue_index;
#endif
};

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

#ifdef CONFIG_SCHED_CPU_MASK_PIN_ONLY
	struct _ready_q ready_q;
#endif

#if (CONFIG_NUM_METAIRQ_PRIORITIES > 0) &&                                                         \
	(CONFIG_NUM_COOP_PRIORITIES > CONFIG_NUM_METAIRQ_PRIORITIES)
	/* Coop thread preempted by current metairq, or NULL */
	struct k_thread *metairq_preempted;
#endif

	uint8_t id;

#if defined(CONFIG_FPU_SHARING)
	void *fp_ctx;
#endif

#ifdef CONFIG_SMP
	/* True when _current is allowed to context switch */
	uint8_t swap_ok;
#endif

#ifdef CONFIG_SCHED_THREAD_USAGE
	/*
	 * [usage0] is used as a timestamp to mark the beginning of an
	 * execution window. [0] is a special value indicating that it
	 * has been stopped (but not disabled).
	 */

	uint32_t usage0;

#ifdef CONFIG_SCHED_THREAD_USAGE_ALL
	struct k_cycle_stats *usage;
#endif
#endif

#ifdef CONFIG_OBJ_CORE_SYSTEM
	struct k_obj_core  obj_core;
#endif

	/* Per CPU architecture specifics */
	struct _cpu_arch arch;
};

typedef struct _cpu _cpu_t;

struct z_kernel {
	struct _cpu cpus[CONFIG_MP_MAX_NUM_CPUS];

#ifdef CONFIG_PM
	int32_t idle; /* Number of ticks for kernel idling */
#endif

	/*
	 * ready queue: can be big, keep after small fields, since some
	 * assembly (e.g. ARC) are limited in the encoding of the offset
	 */
#ifndef CONFIG_SCHED_CPU_MASK_PIN_ONLY
	struct _ready_q ready_q;
#endif

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
#ifdef CONFIG_SCHED_THREAD_USAGE_ALL
	struct k_cycle_stats usage[CONFIG_MP_MAX_NUM_CPUS];
#endif

#ifdef CONFIG_OBJ_CORE_SYSTEM
	struct k_obj_core  obj_core;
#endif

#if defined(CONFIG_SMP) && defined(CONFIG_SCHED_IPI_SUPPORTED)
	/* Identify CPUs to send IPIs to at the next scheduling point */
	atomic_t pending_ipi;
#endif
};

typedef struct z_kernel _kernel_t;

extern struct z_kernel _kernel;

extern atomic_t _cpus_active;

#ifdef CONFIG_SMP

/* True if the current context can be preempted and migrated to
 * another SMP CPU.
 */
bool z_smp_cpu_mobile(void);
#define _current_cpu ({ __ASSERT_NO_MSG(!z_smp_cpu_mobile()); \
			arch_curr_cpu(); })

struct k_thread *z_smp_current_get(void);
#define _current z_smp_current_get()

#else
#define _current_cpu (&_kernel.cpus[0])
#define _current _kernel.cpus[0].current
#endif

/* This is always invoked from a context where preemption is disabled */
#define z_current_thread_set(thread) ({ _current_cpu->current = (thread); })

#ifdef CONFIG_ARCH_HAS_CUSTOM_CURRENT_IMPL
#undef _current
#define _current arch_current_thread()
#undef z_current_thread_set
#define z_current_thread_set(thread) \
	arch_current_thread_set(({ _current_cpu->current = (thread); }))
#endif

/* kernel wait queue record */
#ifdef CONFIG_WAITQ_SCALABLE

typedef struct {
	struct _priq_rb waitq;
} _wait_q_t;

/* defined in kernel/priority_queues.c */
bool z_priq_rb_lessthan(struct rbnode *a, struct rbnode *b);

#define Z_WAIT_Q_INIT(wait_q) { { { .lessthan_fn = z_priq_rb_lessthan } } }

#else

typedef struct {
	sys_dlist_t waitq;
} _wait_q_t;

#define Z_WAIT_Q_INIT(wait_q) { SYS_DLIST_STATIC_INIT(&(wait_q)->waitq) }

#endif /* CONFIG_WAITQ_SCALABLE */

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

typedef void (*k_thread_timeslice_fn_t)(struct k_thread *thread, void *data);

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_KERNEL_INCLUDE_KERNEL_STRUCTS_H_ */
