/*
 * Copyright (c) 2013-2016 Wind River Systems, Inc.
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
 * @brief Private kernel definitions (ARM)
 *
 * This file contains private kernel structures definitions and various
 * other definitions for the ARM Cortex-M3 processor architecture.
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

#ifndef _ASMLANGUAGE
#include <kernel.h>
#include <nano_internal.h>
#include <stdint.h>
#include <misc/dlist.h>
#include <atomic.h>
#endif

#ifndef _ASMLANGUAGE

struct _caller_saved {
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

typedef struct _caller_saved _caller_saved_t;

struct _callee_saved {
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

typedef struct _callee_saved _callee_saved_t;

typedef struct __esf _esf_t;

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
#define K_EXECUTION_MASK \
	(K_TIMING | K_PENDING | K_PRESTART | K_DEAD | K_SUSPENDED | K_DUMMY)

#define K_FP_REGS 0x010	   /* 1 = thread uses floating point registers */
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
struct _preempt_float {
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

struct _thread_arch {

	/* interrupt locking key */
	uint32_t basepri;

#ifdef CONFIG_FLOAT
	/*
	 * No cooperative floating point register set structure exists for
	 * the Cortex-M as it automatically saves the necessary registers
	 * in its exception stack frame.
	 */
	struct _preempt_float  preempt_float;
#endif
};

typedef struct _thread_arch _thread_arch_t;

struct _kernel_arch {
	/* empty */
};

typedef struct _kernel_arch _kernel_arch_t;

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* _kernel_arch_data__h_ */
