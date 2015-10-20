/* nano_private.h - private nanokernel definitions (ARM) */

/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
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

/*
 * DESCRIPTION
 * This file contains private nanokernel structures definitions and various
 * other definitions for the ARM Cortex-M3 processor architecture.
 *
 * This file is also included by assembly language files which must #define
 * _ASMLANGUAGE before including this header file.  Note that nanokernel
 * assembly source files obtains structure offset values via "absolute symbols"
 * in the offsets.o module.
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
#include <../../../kernel/nanokernel/include/nano_internal.h>
#include <stdint.h>
#include <misc/dlist.h>
#endif

#ifndef _ASMLANGUAGE
struct coop {
	/*
	 * Unused for Cortex-M, which automatically saves the neccesary
	 *registers
	 * in its exception stack frame.
	 *
	 * For Cortex-A, this may be:
	 *
	 * uint32_t a1;    // r0
	 * uint32_t a2;    // r1
	 * uint32_t a3;    // r2
	 * uint32_t a4;    // r3
	 * uint32_t ip;    // r12
	 * uint32_t lr;    // r14
	 * uint32_t pc;    // r15
	 * uint32_t xpsr;
	 */
};

typedef struct __esf tESF;

struct preempt {
	uint32_t v1;  /* r4 */
	uint32_t v2;  /* r5 */
	uint32_t v3;  /* r6 */
	uint32_t v4;  /* r7 */
	uint32_t v5;  /* r8 */
	uint32_t v6;  /* r9 */
	uint32_t v7;  /* r10 */
	uint32_t v8;  /* r11 */
	uint32_t psp; /* r13 */
};

typedef struct preempt tPreempt;

#endif /* _ASMLANGUAGE */

/* Bitmask definitions for the struct tcs.flags bit field */

#define FIBER 0x000
#define TASK 0x001	   /* 1 = task, 0 = fiber   */
#define INT_ACTIVE 0x002     /* 1 = executino context is interrupt handler */
#define EXC_ACTIVE 0x004     /* 1 = executino context is exception handler */
#define USE_FP 0x010	 /* 1 = thread uses floating point unit */
#define PREEMPTIBLE                                            \
	0x020 /* 1 = preemptible thread                       \
	       * NOTE: the value must be < 0x100 to be able to \
	       *       use a small thumb instr with immediate  \
	       *       when loading PREEMPTIBLE in a GPR       \
	       */
#define ESSENTIAL 0x200  /* 1 = system thread that must not abort */
#define NO_METRICS 0x400 /* 1 = _Swap() not to update task metrics */

/* stacks */

#define STACK_ROUND_UP(x) ROUND_UP(x, STACK_ALIGN_SIZE)
#define STACK_ROUND_DOWN(x) ROUND_DOWN(x, STACK_ALIGN_SIZE)

#ifdef CONFIG_CPU_CORTEX_M
#include <cortex_m/stack.h>
#include <cortex_m/exc.h>
#endif

#ifndef _ASMLANGUAGE
struct tcs {
	struct tcs *link; /* singly-linked list in _nanokernel.fibers */
	uint32_t flags;
	uint32_t basepri;
	int prio;
#ifdef CONFIG_THREAD_CUSTOM_DATA
	void *custom_data;     /* available for custom use */
#endif
	struct coop coopReg;
	struct preempt preempReg;
#if defined(CONFIG_THREAD_MONITOR)
	struct tcs *next_thread; /* next item in list of ALL fiber+tasks */
#endif
#ifdef CONFIG_NANO_TIMEOUTS
	struct _nano_timeout nano_timeout;
#endif
};

struct s_NANO {
	struct tcs *fiber;   /* singly linked list of runnable fiber */
	struct tcs *task;    /* pointer to runnable task */
	struct tcs *current; /* currently scheduled thread (fiber or task) */
	int flags;     /* struct tcs->flags of 'current' thread */

#if defined(CONFIG_THREAD_MONITOR)
	struct tcs *threads; /* singly linked list of ALL fiber+tasks */
#endif

#ifdef CONFIG_FP_SHARING
	struct tcs *current_fp; /* thread (fiber or task) that owns the FP regs */
#endif			  /* CONFIG_FP_SHARING */

#ifdef CONFIG_ADVANCED_POWER_MANAGEMENT
	int32_t idle; /* Number of ticks for kernel idling */
#endif		      /* CONFIG_ADVANCED_POWER_MANAGEMENT */

#ifdef CONFIG_NANO_TIMEOUTS
	sys_dlist_t timeout_q;
#endif
};

typedef struct s_NANO tNANO;
extern tNANO _nanokernel;

#endif /* _ASMLANGUAGE */

#ifndef _ASMLANGUAGE
extern void _FaultInit(void);
extern void _CpuIdleInit(void);
static ALWAYS_INLINE void nanoArchInit(void)
{
	_nanokernel.flags = FIBER;
	_InterruptStackSetup();
	_ExcSetup();
	_FaultInit();
	_CpuIdleInit();
}

/**
 *
 * @brief Set the return value for the specified fiber (inline)
 *
 * The register used to store the return value from a function call invocation
 * to <value>.  It is assumed that the specified <fiber> is pending, and thus
 * the fiber's thread is stored in its struct tcs structure.
 *
 * @param fiber pointer to the fiber
 * @param value is the value to set as a return value
 *
 * @return N/A
 *
 * \NOMANUAL
 */

static ALWAYS_INLINE void fiberRtnValueSet(struct tcs *fiber,
					   unsigned int value)
{
	tESF *pEsf = (void *)fiber->preempReg.psp;

	pEsf->a1 = value;
}

extern void nano_cpu_atomic_idle(unsigned int);

#define _IS_IN_ISR() _IsInIsr()

extern void _IntLibInit(void);

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* _NANO_PRIVATE_H */
