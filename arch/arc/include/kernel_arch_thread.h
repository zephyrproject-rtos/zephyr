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

#ifndef _kernel_arch_thread__h_
#define _kernel_arch_thread__h_

/*
 * Reason a thread has relinquished control: threads can only be in the NONE
 * or COOP state, threads can be one in the four.
 */
#define _CAUSE_NONE 0
#define _CAUSE_COOP 1
#define _CAUSE_RIRQ 2
#define _CAUSE_FIRQ 3

#ifndef _ASMLANGUAGE
#include <zephyr/types.h>

struct _caller_saved {
	/*
	 * Saved on the stack as part of handling a regular IRQ or by the
	 * kernel when calling the FIRQ return code.
	 */
};

typedef struct _caller_saved _caller_saved_t;

struct _callee_saved {
	u32_t sp; /* r28 */
};
typedef struct _callee_saved _callee_saved_t;

struct _thread_arch {

	/* interrupt key when relinquishing control */
	u32_t intlock_key;

	/* one of the _CAUSE_xxxx definitions above */
	int relinquish_cause;

	/* return value from _Swap */
	unsigned int return_value;

#ifdef CONFIG_ARC_STACK_CHECKING
	/* High address of stack region, stack grows downward from this
	 * location. Usesd for hardware stack checking
	 */
	u32_t stack_base;
#endif
};

typedef struct _thread_arch _thread_arch_t;

#endif /* _ASMLANGUAGE */

#endif /* _kernel_arch_thread__h_ */
