/*
 * Copyright (c) 2023 Advanced Micro Devices, Inc. (AMD)
 * Copyright (c) 2023 Alp Sayin <alpsayin@gmail.com>
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

#ifndef ZEPHYR_INCLUDE_ARCH_MICROBLAZE_THREAD_H_
#define ZEPHYR_INCLUDE_ARCH_MICROBLAZE_THREAD_H_

#ifndef _ASMLANGUAGE
#include <zephyr/types.h>

/*
 * The following structure defines the list of registers that need to be
 * saved/restored when a cooperative context switch occurs.
 */
struct _callee_saved {
	/* r1 is thread's stack pointer */
	uint32_t r1;
	/* IRQ status before irq_lock() and call to z_swap() */
	uint32_t key;
	/* Return value of z_swap() */
	uint32_t retval;
	/* 1 if the thread cooperatively yielded */
	uint32_t preempted;
};
typedef struct _callee_saved _callee_saved_t;

struct _thread_arch {
};

typedef struct _thread_arch _thread_arch_t;

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_MICROBLAZE_THREAD_H_ */
