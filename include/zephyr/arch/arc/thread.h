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

#ifndef ZEPHYR_INCLUDE_ARCH_ARC_THREAD_H_
#define ZEPHYR_INCLUDE_ARCH_ARC_THREAD_H_

/*
 * Reason a thread has relinquished control.
 */
#define _CAUSE_NONE 0
#define _CAUSE_COOP 1
#define _CAUSE_RIRQ 2
#define _CAUSE_FIRQ 3

#ifndef _ASMLANGUAGE
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _callee_saved {
	uintptr_t sp; /* r28 */
};
typedef struct _callee_saved _callee_saved_t;

struct _thread_arch {

	/* one of the _CAUSE_xxxx definitions above */
	int32_t relinquish_cause;

#ifdef CONFIG_ARC_STACK_CHECKING
	/* High address of stack region, stack grows downward from this
	 * location. Usesd for hardware stack checking
	 */
	uintptr_t k_stack_base;
	uintptr_t k_stack_top;
#ifdef CONFIG_USERSPACE
	uintptr_t u_stack_base;
	uintptr_t u_stack_top;
#endif
#endif

#ifdef CONFIG_USERSPACE
	uintptr_t priv_stack_start;
#endif
};

typedef struct _thread_arch _thread_arch_t;

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */


#endif /* ZEPHYR_INCLUDE_ARCH_ARC_THREAD_H_ */
