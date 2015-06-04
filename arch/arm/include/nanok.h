/* nanok.h - private nanokernel definitions (ARM) */

/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
DESCRIPTION
This file contains private nanokernel structures definitions and various other
definitions for the ARM Cortex-M3 processor architecture.

This file is also included by assembly language files which must #define
_ASMLANGUAGE before including this header file.  Note that nanokernel assembly
source files obtains structure offset values via "absolute symbols" in the
offsets.o module.
*/

#ifndef _NANOK__H_
#define _NANOK__H_

#ifdef __cplusplus
extern "C" {
#endif

#include <toolchain.h>
#include <sections.h>
#include <arch/cpu.h>

#ifndef _ASMLANGUAGE
#include <../../../kernel/nanokernel/include/nano_internal.h>
#include <stdint.h>
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

/* Bitmask definitions for the tCCS->flags bit field */

#define FIBER 0x000
#define TASK 0x001	   /* 1 = task context, 0 = fiber context   */
#define INT_ACTIVE 0x002     /* 1 = context is executing interrupt handler */
#define EXC_ACTIVE 0x004     /* 1 = context is executing exception handler */
#define USE_FP 0x010	 /* 1 = context uses floating point unit */
#define PREEMPTIBLE                                            \
	0x020 /* 1 = preemptible context                       \
	       * NOTE: the value must be < 0x100 to be able to \
	       *       use a small thumb instr with immediate  \
	       *       when loading PREEMPTIBLE in a GPR       \
	       */
#define ESSENTIAL 0x200  /* 1 = system context that must not abort */
#define NO_METRICS 0x400 /* 1 = _Swap() not to update task metrics */

/* stacks */

#define STACK_GROWS_DOWN 0
#define STACK_GROWS_UP 1

#define STACK_ROUND_UP(x) ROUND_UP(x, STACK_ALIGN_SIZE)
#define STACK_ROUND_DOWN(x) ROUND_DOWN(x, STACK_ALIGN_SIZE)

#ifdef CONFIG_CPU_CORTEXM
#include <CortexM/stack.h>
#include <CortexM/exc.h>
#endif

#ifndef _ASMLANGUAGE
struct s_CCS {
	struct s_CCS *link; /* singly-linked list in _nanokernel.fibers */
	uint32_t flags;
	uint32_t basepri;
	int prio;
#ifdef CONFIG_CONTEXT_CUSTOM_DATA
	void *custom_data;     /* available for custom use */
#endif
	struct coop coopReg;
	struct preempt preempReg;
#if defined(CONFIG_CONTEXT_MONITOR)
	struct s_CCS *next_context; /* next item in list of ALL fiber+tasks */
#endif
};

struct s_NANO {
	tCCS *fiber;   /* singly linked list of runnable fiber contexts */
	tCCS *task;    /* pointer to runnable task context */
	tCCS *current; /* currently scheduled context (fiber or task) */
	int flags;     /* tCCS->flags of 'current' context */

#if defined(CONFIG_CONTEXT_MONITOR)
	tCCS *contexts; /* singly linked list of ALL fiber+tasks */
#endif

#ifdef CONFIG_FP_SHARING
	tCCS *current_fp; /* context (fiber or task) that owns the FP regs */
#endif			  /* CONFIG_FP_SHARING */

#ifdef CONFIG_ADVANCED_POWER_MANAGEMENT
	int32_t idle; /* Number of ticks for kernel idling */
#endif		      /* CONFIG_ADVANCED_POWER_MANAGEMENT */
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

/*******************************************************************************
*
* fiberRtnValueSet - set the return value for the specified fiber (inline)
*
* The register used to store the return value from a function call invocation
* to <value>.  It is assumed that the specified <fiber> is pending, and thus
* the fiber's context is stored in its tCCS structure.
*
* RETURNS: N/A
*
* \NOMANUAL
*/

static ALWAYS_INLINE void fiberRtnValueSet(
	tCCS *fiber,       /* pointer to fiber */
	unsigned int value /* value to set as return value */
	)
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

#endif /* _NANOK__H_ */
