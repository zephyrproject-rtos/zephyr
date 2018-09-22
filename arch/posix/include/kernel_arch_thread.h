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

#ifndef ZEPHYR_ARCH_POSIX_INCLUDE_KERNEL_ARCH_THREAD_H_
#define ZEPHYR_ARCH_POSIX_INCLUDE_KERNEL_ARCH_THREAD_H_

#ifndef _ASMLANGUAGE
#include <zephyr/types.h>

struct _caller_saved {
	/*
	 * Nothing here
	 */
};


struct _callee_saved {
	/* IRQ status before irq_lock() and call to _Swap() */
	u32_t key;

	/* Return value of _Swap() */
	u32_t retval;

	/*
	 * Thread status pointer
	 * (We need to compile as 32bit binaries in POSIX)
	 */
	u32_t thread_status;
};


struct _thread_arch {
	/* nothing for now */
};

typedef struct _thread_arch _thread_arch_t;

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_ARCH_POSIX_INCLUDE_KERNEL_ARCH_THREAD_H_ */
