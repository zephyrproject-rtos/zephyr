/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_KERNEL_INCLUDE_KERNEL_STRUCTS_H_
#define ZEPHYR_KERNEL_INCLUDE_KERNEL_STRUCTS_H_

#include <kernel.h>

#if !defined(_ASMLANGUAGE)
#include <sys/atomic.h>
#include <sys/dlist.h>
#include <sys/rb.h>
#include <sys/util.h>
#include <string.h>
#include <sys/arch_interface.h>
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

/* Thread is being aborted (SMP only) */
#define _THREAD_ABORTING (BIT(5))

/* Thread is present in the ready queue */
#define _THREAD_QUEUED (BIT(6))

/* end - states */

#ifdef CONFIG_STACK_SENTINEL
/* Magic value in lowest bytes of the stack */
#define STACK_SENTINEL 0xF0F0F0F0
#endif

/* lowest value of _thread_base.preempt at which a thread is non-preemptible */
#define _NON_PREEMPT_THRESHOLD 0x0080

/* highest value of _thread_base.preempt at which a thread is preemptible */
#define _PREEMPT_THRESHOLD (_NON_PREEMPT_THRESHOLD - 1)

#include <kernel_arch_data.h>

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
	u32_t nested;

	/* interrupt stack pointer base */
	char *irq_stack;

	/* currently scheduled thread */
	struct k_thread *current;

	/* one assigned idle thread per CPU */
	struct k_thread *idle_thread;

#ifdef CONFIG_USERSPACE
	/* current syscall frame pointer */
	void *syscall_frame;
#endif

#ifdef CONFIG_TIMESLICING
	/* number of ticks remaining in current time slice */
	int slice_ticks;
#endif

	u8_t id;

#ifdef CONFIG_SMP
	/* True when _current is allowed to context switch */
	u8_t swap_ok;
#endif
};

typedef struct _cpu _cpu_t;

struct z_kernel {
	/* For compatibility with pre-SMP code, union the first CPU
	 * record with the legacy fields so code can continue to use
	 * the "_kernel.XXX" expressions and assembly offsets.
	 */
	union {
		struct _cpu cpus[CONFIG_MP_NUM_CPUS];
#ifndef CONFIG_SMP
		struct {
			/* nested interrupt count */
			u32_t nested;

			/* interrupt stack pointer base */
			char *irq_stack;

			/* currently scheduled thread */
			struct k_thread *current;
		};
#endif
	};

#ifdef CONFIG_SYS_CLOCK_EXISTS
	/* queue of timeouts */
	sys_dlist_t timeout_q;
#endif

#ifdef CONFIG_SYS_POWER_MANAGEMENT
	s32_t idle; /* Number of ticks for kernel idling */
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
#define _current_cpu (z_arch_curr_cpu())
#define _current (z_arch_curr_cpu()->current)
#else
#define _current_cpu (&_kernel.cpus[0])
#define _current _kernel.current
#endif

#define _timeout_q _kernel.timeout_q

#include <kernel_arch_func.h>

#ifdef CONFIG_USE_SWITCH
/* This is a arch function traditionally, but when the switch-based
 * z_swap() is in use it's a simple inline provided by the kernel.
 */
static ALWAYS_INLINE void
z_arch_thread_return_value_set(struct k_thread *thread, unsigned int value)
{
	thread->swap_retval = value;
}
#endif

static ALWAYS_INLINE void
z_thread_return_value_set_with_data(struct k_thread *thread,
				   unsigned int value,
				   void *data)
{
	z_arch_thread_return_value_set(thread, value);
	thread->base.swap_data = data;
}

extern void z_init_thread_base(struct _thread_base *thread_base,
			      int priority, u32_t initial_state,
			      unsigned int options);

static ALWAYS_INLINE void z_new_thread_init(struct k_thread *thread,
					    char *pStack, size_t stackSize,
					    int prio, unsigned int options)
{
#if !defined(CONFIG_INIT_STACKS) && !defined(CONFIG_THREAD_STACK_INFO)
	ARG_UNUSED(pStack);
	ARG_UNUSED(stackSize);
#endif

#ifdef CONFIG_INIT_STACKS
	memset(pStack, 0xaa, stackSize);
#endif
#ifdef CONFIG_STACK_SENTINEL
	/* Put the stack sentinel at the lowest 4 bytes of the stack area.
	 * We periodically check that it's still present and kill the thread
	 * if it isn't.
	 */
	*((u32_t *)pStack) = STACK_SENTINEL;
#endif /* CONFIG_STACK_SENTINEL */
	/* Initialize various struct k_thread members */
	z_init_thread_base(&thread->base, prio, _THREAD_PRESTART, options);

	/* static threads overwrite it afterwards with real value */
	thread->init_data = NULL;
	thread->fn_abort = NULL;

#ifdef CONFIG_THREAD_CUSTOM_DATA
	/* Initialize custom data field (value is opaque to kernel) */
	thread->custom_data = NULL;
#endif

#ifdef CONFIG_THREAD_NAME
	thread->name[0] = '\0';
#endif

#if defined(CONFIG_USERSPACE)
	thread->mem_domain_info.mem_domain = NULL;
#endif /* CONFIG_USERSPACE */

#if defined(CONFIG_THREAD_STACK_INFO)
	thread->stack_info.start = (uintptr_t)pStack;
	thread->stack_info.size = (u32_t)stackSize;
#endif /* CONFIG_THREAD_STACK_INFO */
}

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_KERNEL_INCLUDE_KERNEL_STRUCTS_H_ */
