/*
 * Copyright (c) 2016 Intel Corporation
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
 * @brief Private nanokernel definitions
 *
 * This file contains private nanokernel structures definitions and various
 * other definitions for the Nios II processor architecture.
 *
 * This file is also included by assembly language files which must #define
 * _ASMLANGUAGE before including this header file.  Note that nanokernel
 * assembly source files obtains structure offset values via "absolute
 * symbols" in the offsets.o module.
 */

#ifndef _NANO_PRIVATE_H
#define _NANO_PRIVATE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <toolchain.h>
#include <sections.h>
#include <arch/cpu.h>

#ifndef _ASMLANGUAGE
#include <nanokernel.h>            /* public nanokernel API */
#include <../../../kernel/nanokernel/include/nano_internal.h>
#include <stdint.h>
#include <misc/util.h>
#include <misc/dlist.h>
#endif

/* Bitmask definitions for the struct tcs->flags bit field */
#define FIBER          0x000
#define TASK           0x001 /* 1 = task, 0 = fiber   */

#define INT_ACTIVE     0x002 /* 1 = execution context is interrupt handler */
#define EXC_ACTIVE     0x004 /* 1 = executino context is exception handler */
#define USE_FP         0x010 /* 1 = thread uses floating point unit */
#define PREEMPTIBLE    0x020 /* 1 = preemptible thread */
#define ESSENTIAL      0x200 /* 1 = system thread that must not abort */
#define NO_METRICS     0x400 /* 1 = _Swap() not to update task metrics */

/* stacks */

#define STACK_ALIGN_SIZE 4

#define STACK_ROUND_UP(x) ROUND_UP(x, STACK_ALIGN_SIZE)
#define STACK_ROUND_DOWN(x) ROUND_DOWN(x, STACK_ALIGN_SIZE)

#ifndef _ASMLANGUAGE

/*
 * The following structure defines the set of 'non-volatile' or 'callee saved'
 * integer registers.  These registers must be preserved by a called C
 * function.  These are the only registers that need to be saved/restored when
 * a cooperative context switch occurs.
 */
struct s_coop {
	/* General purpose callee-saved registers */
	uint32_t r16;
	uint32_t r17;
	uint32_t r18;
	uint32_t r19;
	uint32_t r20;
	uint32_t r21;
	uint32_t r22;
	uint32_t r23;

	 /* Normally used for the frame pointer but also a general purpose
	  * register if frame pointers omitted
	  */
	uint32_t r28;

	uint32_t ra; /* Return address */
	uint32_t sp; /* Stack pointer */
	uint32_t key; /* IRQ status before irq_lock() and call to _Swap() */
	uint32_t retval; /* Return value of _Swap() */
};
typedef struct s_coop t_coop;

/*
 * The following structure defines the set of caller-saved integer registers.
 * These registers need not be preserved by a called C function.  Given that
 * they are not preserved across function calls, they must be save/restored
 * (along with the struct coop regs) when a preemptive context switch occurs.
 */
struct preempt {
	/* Nothing here, the exception code puts all the caller-saved registers
	 * onto the stack
	 */
};

struct tcs {
	struct tcs *link;         /* node in singly-linked list
				   * _nanokernel.fibers
				   */
	uint32_t flags;           /* bitmask of flags above */
	int prio;                 /* fiber priority, -1 for a task */
	struct preempt preempReg;
	t_coop coopReg;

#ifdef CONFIG_ERRNO
	int errno_var;
#endif
#ifdef CONFIG_NANO_TIMEOUTS
	struct _nano_timeout nano_timeout;
#endif
#if defined(CONFIG_THREAD_MONITOR)
	struct __thread_entry *entry; /* thread entry and parameters description */
	struct tcs *next_thread; /* next item in list of ALL fiber+tasks */
#endif
#ifdef CONFIG_MICROKERNEL
	void *uk_task_ptr;
#endif
#ifdef CONFIG_THREAD_CUSTOM_DATA
	void *custom_data;        /* available for custom use */
#endif
};

struct s_NANO {
	struct tcs *fiber;    /* singly linked list of runnable fibers */
	struct tcs *task;     /* current task the nanokernel knows about */
	struct tcs *current;  /* currently scheduled thread (fiber or task) */

#if defined(CONFIG_NANO_TIMEOUTS) || defined(CONFIG_NANO_TIMERS)
	sys_dlist_t timeout_q;
	int32_t task_timeout;
#endif
#if defined(CONFIG_THREAD_MONITOR)
	struct tcs *threads; /* singly linked list of ALL fiber+tasks */
#endif

	/* Nios II-specific members */

	char *irq_sp;		/* Interrupt stack pointer */
	uint32_t nested;	/* IRQ/exception nest level */
};

typedef struct s_NANO tNANO;
extern tNANO _nanokernel;
extern char _interrupt_stack[CONFIG_ISR_STACK_SIZE];


/* Arch-specific nanokernel APIs */
void nano_cpu_idle(void);
void nano_cpu_atomic_idle(unsigned int key);

static ALWAYS_INLINE void nanoArchInit(void)
{
	_nanokernel.irq_sp = (char *)STACK_ROUND_DOWN(_interrupt_stack +
						      CONFIG_ISR_STACK_SIZE);
}

static ALWAYS_INLINE void fiberRtnValueSet(struct tcs *fiber,
					   unsigned int value)
{
	fiber->coopReg.retval = value;
}

static inline void _IntLibInit(void)
{
	/* No special initialization of the interrupt subsystem required */
}

FUNC_NORETURN void _NanoFatalErrorHandler(unsigned int reason,
					  const NANO_ESF *esf);


static ALWAYS_INLINE int _IS_IN_ISR(void)
{
	char *sp = (char *)_nios2_read_sp();

	/* Make sure we're on the interrupt stack somewhere */
	if (sp < _interrupt_stack ||
	    sp >= (char *)(STACK_ROUND_DOWN(_interrupt_stack +
					    CONFIG_ISR_STACK_SIZE))) {
		return 0;
	}
	return 1;
}

#ifdef CONFIG_IRQ_OFFLOAD
void _irq_do_offload(void);
#endif

#if NIOS2_ICACHE_SIZE > 0
void _nios2_icache_flush_all(void);
#else
#define _nios2_icache_flush_all() do { } while (0)
#endif

#if NIOS2_DCACHE_SIZE > 0
void _nios2_dcache_flush_all(void);
#else
#define _nios2_dcache_flush_all() do { } while (0)
#endif

#endif /* _ASMLANGUAGE */

#endif /* _NANO_PRIVATE_H */

