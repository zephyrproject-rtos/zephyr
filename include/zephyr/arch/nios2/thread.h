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
 *
 * necessary to instantiate instances of struct k_thread.
 */

#ifndef ZEPHYR_INCLUDE_ARCH_NIOS2_THREAD_H_
#define ZEPHYR_INCLUDE_ARCH_NIOS2_THREAD_H_

#ifndef _ASMLANGUAGE
#include <zephyr/types.h>

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

	/* IRQ status before irq_lock() and call to z_swap() */
	uint32_t key;

	/* Return value of z_swap() */
	uint32_t retval;
};

typedef struct _callee_saved _callee_saved_t;

struct _thread_arch {
	/* nothing for now */
};

typedef struct _thread_arch _thread_arch_t;

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_NIOS2_THREAD_H_ */
