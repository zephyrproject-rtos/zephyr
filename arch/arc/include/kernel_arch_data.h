/*
 * Copyright (c) 2014-2016 Wind River Systems, Inc.
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
 * other definitions for the ARCv2 processor architecture.
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
#include <vector_table.h>

#ifndef _ASMLANGUAGE
#include <kernel.h>
#include <nano_internal.h>
#include <stdint.h>
#include <misc/util.h>
#include <misc/dlist.h>
#endif

#ifndef _ASMLANGUAGE

struct _caller_saved {
	/*
	 * Saved on the stack as part of handling a regular IRQ or by the
	 * kernel when calling the FIRQ return code.
	 */
};

typedef struct _caller_saved _caller_saved_t;

struct _irq_stack_frame {
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
	 * Currently unsupported. This is where those registers are
	 * automatically pushed on the stack by the CPU when taking a regular
	 * IRQ.
	 */
	uint32_t ei_base;
	uint32_t ldi_base;
	uint32_t jli_base;
#endif
	uint32_t pc;
	uint32_t status32;
};

typedef struct _irq_stack_frame _isf_t;

struct _callee_saved {
	uint32_t sp; /* r28 */
};
typedef struct _callee_saved _callee_saved_t;

/* callee-saved registers pushed on the stack, not in k_thread */
struct _callee_saved_stack {
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

typedef struct _callee_saved_stack _callee_saved_stack_t;

#endif /* _ASMLANGUAGE */

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

#define K_FP_REGS      0x010 /* 1 = thread uses floating point registers */
#define K_ESSENTIAL    0x200 /* 1 = system thread that must not abort */
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

struct _thread_arch {

	/* interrupt key when relinquishing control */
	uint32_t intlock_key;

	/* one of the _CAUSE_xxxx definitions above */
	int relinquish_cause;

	/* return value from _Swap */
	unsigned int return_value;

#ifdef CONFIG_ARC_STACK_CHECKING
	/* top of stack for hardware stack checking */
	uint32_t stack_top;
#endif
};

typedef struct _thread_arch _thread_arch_t;

struct _kernel_arch {

	char *rirq_sp; /* regular IRQ stack pointer base */

	/*
	 * FIRQ stack pointer is installed once in the second bank's SP, so
	 * there is no need to track it in _kernel.
	 */

};

typedef struct _kernel_arch _kernel_arch_t;

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* _kernel_arch_data__h_ */
