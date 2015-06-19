/* nano_private.h - private nanokernel definitions */

/*
 * Copyright (c) 2014 Wind River Systems, Inc.
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
definitions for the ARCv2 processor architecture.

This file is also included by assembly language files which must #define
_ASMLANGUAGE before including this header file.  Note that nanokernel assembly
source files obtains structure offset values via "absolute symbols" in the
offsets.o module.
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
	 * Saved on the stack as part of handling a regular IRQ or by the kernel
	 * when calling the FIRQ return code.
	 */
};

struct irq_stack_frame {
	uint32_t r0;
	uint32_t r1;
	uint32_t r2;
	uint32_t r3;
	uint32_t r4;
	uint32_t r5;
	uint32_t r6;
	uint32_t r7;
	uint32_t r8;
	uint32_t r9;
	uint32_t r10;
	uint32_t r11;
	uint32_t r12;
	uint32_t r13;
	uint32_t blink;
	uint32_t lp_end;
	uint32_t lp_start;
	uint32_t lp_count;
#ifdef CONFIG_CODE_DENSITY
	/*
	 * Currently unsupported. This is where those registers are automatically
	 * pushed on the stack by the CPU when taking a regular IRQ.
	 */
	uint32_t ei_base;
	uint32_t ldi_base;
	uint32_t jli_base;
#endif
	uint32_t pc;
	uint32_t status32;
};

typedef struct irq_stack_frame tISF;

struct preempt {
	uint32_t sp; /* r28 */
};
typedef struct preempt tPreempt;

struct callee_saved {
	uint32_t r13;
	uint32_t r14;
	uint32_t r15;
	uint32_t r16;
	uint32_t r17;
	uint32_t r18;
	uint32_t r19;
	uint32_t r20;
	uint32_t r21;
	uint32_t r22;
	uint32_t r23;
	uint32_t r24;
	uint32_t r25;
	uint32_t r26;
	uint32_t fp; /* r27 */
	/* r28 is the stack pointer and saved separately */
	/* r29 is ILINK and does not need to be saved */
	uint32_t r30;
	/*
	 * No need to save r31 (blink), it's either alread pushed as the pc or
	 * blink on an irq stack frame.
	 */
};
typedef struct callee_saved tCalleeSaved;

/* registers saved by software when taking a FIRQ */
struct firq_regs {
	uint32_t lp_count;
	uint32_t lp_start;
	uint32_t lp_end;
};
typedef struct firq_regs tFirqRegs;

#endif /* _ASMLANGUAGE */

/* Bitmask definitions for the tCCS->flags bit field */

#define FIBER          0x000
#define TASK           0x001 /* 1 = task context, 0 = fiber context   */

#define INT_ACTIVE     0x002 /* 1 = context is executing interrupt handler */
#define EXC_ACTIVE     0x004 /* 1 = context is executing exception handler */
#define USE_FP         0x010 /* 1 = context uses floating point unit */
#define PREEMPTIBLE    0x020 /* 1 = preemptible context */
#define ESSENTIAL      0x200 /* 1 = system context that must not abort */
#define NO_METRICS     0x400 /* 1 = _Swap() not to update task metrics */

/* stacks */

#define STACK_GROWS_DOWN 0
#define STACK_GROWS_UP   1
#define STACK_ALIGN_SIZE 4

#define STACK_ROUND_UP(x) ROUND_UP(x, STACK_ALIGN_SIZE)
#define STACK_ROUND_DOWN(x) ROUND_DOWN(x, STACK_ALIGN_SIZE)

/*
 * Reason a context has relinquished control: fibers can only be in the NONE
 * or COOP state, tasks can be one in the four.
 */
#define _CAUSE_NONE 0
#define _CAUSE_COOP 1
#define _CAUSE_RIRQ 2
#define _CAUSE_FIRQ 3

#ifndef _ASMLANGUAGE

struct ccs {
	struct ccs *link;        /* node in singly-linked list
								* _nanokernel.fibers */
	uint32_t flags;            /* bitmask of flags above */
	uint32_t intlock_key;      /* interrupt key when relinquishing control */
	int relinquish_cause;      /* one of the _CAUSE_xxxx definitions above */
	unsigned int return_value; /* return value from _Swap */
	int prio;                  /* fiber priority, -1 for a task */
#ifdef CONFIG_CONTEXT_CUSTOM_DATA
	void *custom_data;         /* available for custom use */
#endif
	struct coop coopReg;
	struct preempt preempReg;
#ifdef CONFIG_CONTEXT_MONITOR
	struct ccs *next_context;  /* next item in list of ALL fiber+tasks */
#endif
#ifdef CONFIG_NANO_TIMEOUTS
	struct _nano_timeout nano_timeout;
#endif
};

struct s_NANO {
	tCCS *fiber;    /* singly linked list of runnable fiber contexts */
	tCCS *task;     /* current task the nanokernel knows about */
	tCCS *current;  /* currently scheduled context (fiber or task) */

#ifdef CONFIG_CONTEXT_MONITOR
	tCCS *contexts; /* singly linked list of ALL fiber+tasks */
#endif

#ifdef CONFIG_FP_SHARING
	tCCS *current_fp; /* context (fiber or task) that owns the FP regs */
#endif

#ifdef CONFIG_ADVANCED_POWER_MANAGEMENT
	int32_t idle; /* Number of ticks for kernel idling */
#endif

	char *rirq_sp; /* regular IRQ stack pointer base */

	/*
	 * FIRQ stack pointer is installed once in the second bank's SP, so
	 * there is no need to track it in _nanokernel.
	 */

	struct firq_regs firq_regs;
#ifdef CONFIG_NANO_TIMEOUTS
	sys_dlist_t timeout_q;
#endif
};

typedef struct s_NANO tNANO;
extern tNANO _nanokernel;

#ifdef CONFIG_CPU_ARCV2
#include <v2/cache.h>
#include <v2/irq.h>
#endif

static ALWAYS_INLINE void nanoArchInit(void)
{
	_icache_setup();
	_irq_setup();
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

static ALWAYS_INLINE void fiberRtnValueSet(tCCS *fiber, unsigned int value)
{
	fiber->return_value = value;
}

/*******************************************************************************
*
* _IS_IN_ISR - indicates if kernel is handling interrupt
*
* RETURNS: 1 if interrupt handler is executed, 0 otherwise
*
* \NOMANUAL
*/

static ALWAYS_INLINE int _IS_IN_ISR(void)
{
	uint32_t act = _arc_v2_aux_reg_read(_ARC_V2_AUX_IRQ_ACT);

	return ((act & 0xffff) != 0);
}

extern void nanoCpuAtomicIdle(unsigned int);
extern void _ContextEntryWrapper(void);

static inline void _IntLibInit(void)
{
	/* nothing needed, here because the kernel requires it */
}

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* _NANO_PRIVATE_H */
