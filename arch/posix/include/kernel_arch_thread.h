/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2017 Oticon A/S
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

#ifndef _ASMLANGUAGE
#include <zephyr/types.h>

struct _caller_saved {
	/*
	 * Nothing here
	 */
};

typedef struct _caller_saved _caller_saved_t;

struct _callee_saved {
	/* IRQ status before irq_lock() and call to _Swap() */
	u32_t key;

	/* Return value of _Swap() */
	u32_t retval;

	/*
	 * Thread status pointer
	 * (Portability: you need to compile as a 32bit binary..)
	 */
	u32_t thread_status;
};

typedef struct _callee_saved _callee_saved_t;

struct _thread_arch {
	/* nothing for now */
};

typedef struct _thread_arch _thread_arch_t;


#endif /* _ASMLANGUAGE */

#endif /* _kernel_arch_thread__h_ */
