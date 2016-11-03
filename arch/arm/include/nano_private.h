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

/**
 * @file
 * @brief Private nanokernel definitions (ARM)
 *
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
#include <kernel.h>
#include <../../../kernel/unified/include/nano_internal.h>
#include <stdint.h>
#include <misc/dlist.h>
#include <atomic.h>
#endif

#ifndef _ASMLANGUAGE

#ifdef CONFIG_THREAD_MONITOR
struct __thread_entry {
	_thread_entry_t pEntry;
	void *parameter1;
	void *parameter2;
	void *parameter3;
};
#endif /*CONFIG_THREAD_MONITOR*/

struct coop {
	/*
	 * Unused for Cortex-M, which automatically saves the necessary
	 * registers in its exception stack frame.
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

#define K_STATIC  0x00000800

#define K_READY              0x00000000    /* Thread is ready to run */
#define K_TIMING             0x00001000    /* Thread is waiting on a timeout */
#define K_PENDING            0x00002000    /* Thread is waiting on an object */
#define K_PRESTART           0x00004000    /* Thread has not yet started */
#define K_DEAD               0x00008000    /* Thread has terminated */
#define K_SUSPENDED          0x00010000    /* Thread is suspended */
#define K_DUMMY              0x00020000    /* Not a real thread */
#define K_EXECUTION_MASK    (K_TIMING | K_PENDING | K_PRESTART | \
			     K_DEAD | K_SUSPENDED | K_DUMMY)

#define USE_FP 0x010	 /* 1 = thread uses floating point unit */
#define K_ESSENTIAL 0x200  /* 1 = system thread that must not abort */
#define NO_METRICS 0x400 /* 1 = _Swap() not to update task metrics */

/* stacks */

#define STACK_ROUND_UP(x) ROUND_UP(x, STACK_ALIGN_SIZE)
#define STACK_ROUND_DOWN(x) ROUND_DOWN(x, STACK_ALIGN_SIZE)

#ifdef CONFIG_CPU_CORTEX_M
#include <cortex_m/stack.h>
#include <cortex_m/exc.h>
#endif

#ifndef _ASMLANGUAGE

#ifdef CONFIG_FLOAT
struct preemp_float {
	float  s16;
	float  s17;
	float  s18;
	float  s19;
	float  s20;
	float  s21;
	float  s22;
	float  s23;
	float  s24;
	float  s25;
	float  s26;
	float  s27;
	float  s28;
	float  s29;
	float  s30;
	float  s31;
};
#endif

/* 'struct tcs_base' must match the beginning of 'struct tcs' */
struct tcs_base {
	sys_dnode_t  k_q_node;
	uint32_t     flags;
	int          prio;
	void        *swap_data;
#ifdef CONFIG_NANO_TIMEOUTS
	struct _timeout timeout;
#endif
};

struct tcs {
	sys_dnode_t k_q_node;	/* node object in any kernel queue */
	uint32_t    flags;
	int         prio;
	void       *swap_data;
#ifdef CONFIG_NANO_TIMEOUTS
	struct _timeout timeout;
#endif
	uint32_t basepri;
#ifdef CONFIG_THREAD_CUSTOM_DATA
	void *custom_data;     /* available for custom use */
#endif
	struct coop coopReg;
	struct preempt preempReg;
#if defined(CONFIG_THREAD_MONITOR)
	struct __thread_entry *entry; /* thread entry and parameters description */
	struct tcs *next_thread; /* next item in list of ALL fiber+tasks */
#endif
#ifdef CONFIG_ERRNO
	int errno_var;
#endif
	atomic_t sched_locked;
	void *init_data;
	void (*fn_abort)(void);
#ifdef CONFIG_FLOAT
	/*
	 * No cooperative floating point register set structure exists for
	 * the Cortex-M as it automatically saves the necessary registers
	 * in its exception stack frame.
	 */
	struct preemp_float  preemp_float_regs;
#endif
};

struct ready_q {
	struct k_thread *cache;
	uint32_t prio_bmap[1];
	sys_dlist_t q[K_NUM_PRIORITIES];
};

struct s_NANO {
	struct tcs *current; /* currently scheduled thread (fiber or task) */

#if defined(CONFIG_THREAD_MONITOR)
	struct tcs *threads; /* singly linked list of ALL fiber+tasks */
#endif

#ifdef CONFIG_FP_SHARING
	struct tcs *current_fp; /* thread (fiber or task) that owns the FP regs */
#endif			  /* CONFIG_FP_SHARING */

#ifdef CONFIG_SYS_POWER_MANAGEMENT
	int32_t idle; /* Number of ticks for kernel idling */
#endif

#if defined(CONFIG_NANO_TIMEOUTS) || defined(CONFIG_NANO_TIMERS)
	sys_dlist_t timeout_q;
#endif
	struct ready_q ready_q;
};

typedef struct s_NANO tNANO;
extern tNANO _nanokernel;

#endif /* _ASMLANGUAGE */

#ifndef _ASMLANGUAGE
extern void _FaultInit(void);
extern void _CpuIdleInit(void);
static ALWAYS_INLINE void nanoArchInit(void)
{
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
 */
static ALWAYS_INLINE void fiberRtnValueSet(struct tcs *fiber,
					   unsigned int value)
{
	tESF *pEsf = (void *)fiber->preempReg.psp;

	pEsf->a1 = value;
}

#define _current _nanokernel.current
#define _ready_q _nanokernel.ready_q
#define _timeout_q _nanokernel.timeout_q
#define _set_thread_return_value fiberRtnValueSet

static ALWAYS_INLINE void
_set_thread_return_value_with_data(struct k_thread *thread, unsigned int value,
				   void *data)
{
	_set_thread_return_value(thread, value);
	thread->swap_data = data;
}

#define _IDLE_THREAD_PRIO (CONFIG_NUM_PREEMPT_PRIORITIES)

extern void nano_cpu_atomic_idle(unsigned int);

#define _is_in_isr() _IsInIsr()

extern void _IntLibInit(void);

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* _NANO_PRIVATE_H */
