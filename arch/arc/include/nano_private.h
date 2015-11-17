/* nano_private.h - private nanokernel definitions */

/*
 * Copyright (c) 2014 Wind River Systems, Inc.
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
 * other definitions for the ARCv2 processor architecture.
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
#include <vector_table.h>

#ifndef _ASMLANGUAGE
#include <nanokernel.h>            /* public nanokernel API */
#include <../../../kernel/nanokernel/include/nano_internal.h>
#include <stdint.h>
#include <misc/util.h>
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

/*
 * Reason a thread has relinquished control: fibers can only be in the NONE
 * or COOP state, tasks can be one in the four.
 */
#define _CAUSE_NONE 0
#define _CAUSE_COOP 1
#define _CAUSE_RIRQ 2
#define _CAUSE_FIRQ 3

#ifndef _ASMLANGUAGE

struct tcs {
	struct tcs *link;         /* node in singly-linked list
				   * _nanokernel.fibers
				   */
	uint32_t flags;           /* bitmask of flags above */
	uint32_t intlock_key;     /* interrupt key when relinquishing control */
	int relinquish_cause;     /* one of the _CAUSE_xxxx definitions above */
	unsigned int return_value;/* return value from _Swap */
	int prio;                 /* fiber priority, -1 for a task */
#ifdef CONFIG_THREAD_CUSTOM_DATA
	void *custom_data;        /* available for custom use */
#endif
	struct coop coopReg;
	struct preempt preempReg;
#ifdef CONFIG_THREAD_MONITOR
	struct tcs *next_thread;  /* next item in list of ALL fiber+tasks */
#endif
#ifdef CONFIG_NANO_TIMEOUTS
	struct _nano_timeout nano_timeout;
#endif
};

struct s_NANO {
	struct tcs *fiber;    /* singly linked list of runnable fibers */
	struct tcs *task;     /* current task the nanokernel knows about */
	struct tcs *current;  /* currently scheduled thread (fiber or task) */

#ifdef CONFIG_THREAD_MONITOR
	struct tcs *threads; /* singly linked list of ALL fiber+tasks */
#endif

#ifdef CONFIG_FP_SHARING
	struct tcs *current_fp; /* thread (fiber or task) that owns the FP regs */
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

/**
 *
 * @brief Set the return value for the specified fiber (inline)
 *
 * The register used to store the return value from a function call invocation
 * to <value>.  It is assumed that the specified <fiber> is pending, and thus
 * the fiber's thread is stored in its struct tcs structure.
 *
 * @return N/A
 */
static ALWAYS_INLINE void fiberRtnValueSet(struct tcs *fiber, unsigned int value)
{
	fiber->return_value = value;
}

/**
 *
 * @brief Indicates if kernel is handling interrupt
 *
 * @return 1 if interrupt handler is executed, 0 otherwise
 */
static ALWAYS_INLINE int _IS_IN_ISR(void)
{
	uint32_t act = _arc_v2_aux_reg_read(_ARC_V2_AUX_IRQ_ACT);
#if CONFIG_IRQ_OFFLOAD
	/* Check if we're in a TRAP_S exception as well */
	if (_arc_v2_aux_reg_read(_ARC_V2_STATUS32) & _ARC_V2_STATUS32_AE &&
	    _ARC_V2_ECR_VECTOR(_arc_v2_aux_reg_read(_ARC_V2_ECR)) == EXC_EV_TRAP) {
		return 1;
	}
#endif
	return ((act & 0xffff) != 0);
}

extern void nanoCpuAtomicIdle(unsigned int);
extern void _thread_entry_wrapper(void);

static inline void _IntLibInit(void)
{
	/* nothing needed, here because the kernel requires it */
}

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* _NANO_PRIVATE_H */
