/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2016 Wind River Systems, Inc.
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
 * @brief Private kernel definitions
 *
 * This file contains private kernel structures definitions and various
 * other definitions for the Nios II processor architecture.
 *
 * This file is also included by assembly language files which must #define
 * _ASMLANGUAGE before including this header file.  Note that kernel
 * assembly source files obtains structure offset values via "absolute
 * symbols" in the offsets.o module.
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
#include <misc/util.h>
#include <misc/dlist.h>
#endif

/* Bitmask definitions for the struct tcs->flags bit field */
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

#define INT_ACTIVE     0x002 /* 1 = executing context is interrupt handler */
#define EXC_ACTIVE     0x004 /* 1 = executing context is exception handler */
#define K_FP_REGS      0x010 /* 1 = thread uses floating point registers */
#define K_ESSENTIAL    0x200 /* 1 = system thread that must not abort */
#define NO_METRICS     0x400 /* 1 = _Swap() not to update task metrics */

/* stacks */

#define STACK_ALIGN_SIZE 4

#define STACK_ROUND_UP(x) ROUND_UP(x, STACK_ALIGN_SIZE)
#define STACK_ROUND_DOWN(x) ROUND_DOWN(x, STACK_ALIGN_SIZE)

#ifndef _ASMLANGUAGE

struct _caller_saved {
	/*
	 * Nothing here, the exception code puts all the caller-saved
	 * registers onto the stack.
	 */
};

typedef struct _caller_saved _caller_saved_t;

struct _callee_saved {
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

	/* Return address */
	uint32_t ra;

	/* Stack pointer */
	uint32_t sp;

	/* IRQ status before irq_lock() and call to _Swap() */
	uint32_t key;

	/* Return value of _Swap() */
	uint32_t retval;
};

typedef struct _callee_saved _callee_saved_t;

struct _thread_arch {
	/* nothing for now */
};

typedef struct _thread_arch _thread_arch_t;

struct _kernel_arch {
	/* nothing for now */
};

typedef struct _kernel_arch _kernel_arch_t;

extern char _interrupt_stack[CONFIG_ISR_STACK_SIZE];

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* _kernel_arch_data__h_ */
