/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 * Copyright (c) 2016 Cadence Design Systems, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Private kernel definitions (XTENSA)
 *
 * This file contains private kernel structures definitions and various
 * other definitions for the XTENSA processors family architecture.
 *
 * This file is also included by assembly language files which must #define
 * _ASMLANGUAGE before including this header file.  Note that kernel
 * assembly source files obtains structure offset values via "absolute symbols"
 * in the offsets.o module.
 */

#ifndef _kernel_arch_data__h_
#define _kernel_arch_data__h_

#ifdef __cplusplus
extern "C" {
#endif

#include <toolchain.h>
#include <sections.h>
#include <arch/cpu.h>
#include <xtensa_context.h>

#if !defined(_ASMLANGUAGE) && !defined(__ASSEMBLER__)
#include <kernel.h>            /* public kernel API */
#include <nano_internal.h>
#include <stdint.h>
#include <misc/dlist.h>
#include <misc/util.h>

/* Bitmask definitions for the struct k_thread->flags bit field */

/* executing context is interrupt handler */
#define INT_ACTIVE (1 << 1)
/* executing context is exception handler */
#define EXC_ACTIVE (1 << 2)
/* thread uses floating point unit */
#define USE_FP 0x010

/*
 * The following structure defines the set of 'volatile' integer registers.
 * These registers need not be preserved by a called C function.  Given that
 * they are not preserved across function calls, they must be save/restored
 * (along with the struct _caller_saved) when a preemptive context switch
 * occurs.
 */

struct _caller_saved {

	/*
	 * The volatile registers area not included in the definition of
	 * 'tPreempReg' since the interrupt stubs (_IntEnt/_IntExit)
	 * and exception stubs (_ExcEnt/_ExcEnter) use the stack to save and
	 * restore the values of these registers in order to support interrupt
	 * nesting.  The stubs do _not_ copy the saved values from the stack
	 * into the TCS.
	 */
};

typedef struct _caller_saved _caller_saved_t;

/*
 * The following structure defines the set of 'non-volatile' integer registers.
 * These registers must be preserved by a called C function.  These are the
 * only registers that need to be saved/restored when a cooperative context
 * switch occurs.
 */
struct _callee_saved {
	/*
	 * The following registers are considered non-volatile, i.e. callee-saved,
	 * but their values are pushed onto the stack rather than stored in the
	 * TCS structure:
	 */
	uint32_t retval; /* a2 */
	XtExcFrame *topOfStack; /* a1 = sp */
};

typedef struct _callee_saved _callee_saved_t;

typedef struct __esf __esf_t;

/*
 * The following structure defines the set of 'non-volatile' x87 FPU/MMX/SSE
 * registers. These registers must be preserved by a called C function.
 * These are the only registers that need to be saved/restored when a
 * cooperative context switch occurs.
 */
typedef struct s_coopCoprocReg {

	/*
	 * This structure intentionally left blank. Coprocessor's registers are all
	 * 'volatile' and saved using the lazy context switch mechanism.
	 */

} tCoopCoprocReg;

/*
 * The following structure defines the set of 'volatile' x87 FPU/MMX/SSE
 * registers.  These registers need not be preserved by a called C function.
 * Given that they are not preserved across function calls, they must be
 * save/restored (along with s_coopCoprocReg) when a preemptive context
 * switch occurs.
 */
typedef struct s_preempCoprocReg {
	/*
	 * This structure intentionally left blank, as for now coprocessor's stack
	 * is positioned at top of the stack.
	 */
#if XCHAL_CP_NUM > 0
	char *cpStack;
#endif
} tPreempCoprocReg;

/*
 * The thread control stucture definition.  It contains the
 * various fields to manage a _single_ thread. The TCS will be aligned
 * to the appropriate architecture specific boundary via the
 * _new_thread() call.
 */
struct _thread_arch {
	/*
	 * See the above flag definitions above for valid bit settings.  This
	 * field must remain near the start of struct tcs, specifically
	 * before any #ifdef'ed fields since the host tools currently use a
	 * fixed
	 * offset to read the 'flags' field.
	 */
	uint32_t flags;
	int prio;     /* thread priority used to sort linked list */
#ifdef CONFIG_THREAD_CUSTOM_DATA
	void *custom_data;     /* available for custom use */
#endif
#if defined(CONFIG_THREAD_MONITOR)
	struct __thread_entry *entry; /* thread entry and parameters description */
	struct tcs *next_thread; /* next item in list of ALL fiber+tasks */
#endif
#ifdef CONFIG_ERRNO
	int errno_var;
#endif
	/*
	 * The location of all floating point related structures/fields MUST be
	 * located at the end of struct tcs.  This way only the
	 * fibers/tasks that actually utilize non-integer capabilities need to
	 * account for the increased memory required for storing FP state when
	 * sizing stacks.
	 *
	 * Given that stacks "grow down" on Xtensa, and the TCS is located
	 * at the start of a thread's "workspace" memory, the stacks of
	 * fibers/tasks that do not utilize floating point instruction can
	 * effectively consume the memory occupied by the 'tCoopCoprocReg' and
	 * 'tPreempCoprocReg' structures without ill effect.
	 */
	 /* TODO: Move Xtensa coprocessor's stack here to get rid of extra indirection */
	tCoopCoprocReg coopCoprocReg; /* non-volatile coprocessor's register storage */
	tPreempCoprocReg preempCoprocReg; /* volatile coprocessor's register storage */
};

typedef struct _thread_arch _thread_arch_t;

struct _kernel_arch {
#if defined(CONFIG_DEBUG_INFO)
	NANO_ISF *isf;    /* ptr to interrupt stack frame */
#endif
};

typedef struct _kernel_arch _kernel_arch_t;

#endif /*! _ASMLANGUAGE && ! __ASSEMBLER__ */


/* stacks */
#define STACK_ROUND_UP(x) ROUND_UP(x, STACK_ALIGN_SIZE)
#define STACK_ROUND_DOWN(x) ROUND_DOWN(x, STACK_ALIGN_SIZE)

#ifdef __cplusplus
}
#endif

#endif /* _kernel_arch_data__h_ */

