/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Per-arch thread definition
 *
 * This file contains definitions for
 *
 *  struct _thread_arch
 *  struct _callee_saved
 *  struct _caller_saved
 *
 * necessary to instantiate instances of struct k_thread.
 */

#ifndef ZEPHYR_ARCH_NIOS2_INCLUDE_KERNEL_ARCH_THREAD_H_
#define ZEPHYR_ARCH_NIOS2_INCLUDE_KERNEL_ARCH_THREAD_H_

#ifndef _ASMLANGUAGE
#include <zephyr/types.h>

struct _caller_saved {
	/*
	 * Nothing here, the exception code puts all the caller-saved
	 * registers onto the stack.
	 */
};

typedef struct _caller_saved _caller_saved_t;

struct _callee_saved {
	/* General purpose callee-saved registers */
	u32_t r16;
	u32_t r17;
	u32_t r18;
	u32_t r19;
	u32_t r20;
	u32_t r21;
	u32_t r22;
	u32_t r23;

	 /* Normally used for the frame pointer but also a general purpose
	  * register if frame pointers omitted
	  */
	u32_t r28;

	/* Return address */
	u32_t ra;

	/* Stack pointer */
	u32_t sp;

	/* IRQ status before irq_lock() and call to _Swap() */
	u32_t key;

	/* Return value of _Swap() */
	u32_t retval;
};

typedef struct _callee_saved _callee_saved_t;

struct _thread_arch {
	/* nothing for now */
};

typedef struct _thread_arch _thread_arch_t;

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_ARCH_NIOS2_INCLUDE_KERNEL_ARCH_THREAD_H_ */

