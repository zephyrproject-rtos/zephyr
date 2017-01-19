/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
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
