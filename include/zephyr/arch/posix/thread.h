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
 *
 * necessary to instantiate instances of struct k_thread.
 */

#ifndef ZEPHYR_INCLUDE_ARCH_POSIX_THREAD_H_
#define ZEPHYR_INCLUDE_ARCH_POSIX_THREAD_H_

#ifndef _ASMLANGUAGE
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _callee_saved {
	/* IRQ status before irq_lock() and call to z_swap() */
	uint32_t key;

	/* Return value of z_swap() */
	uint32_t retval;

	/* Thread status pointer */
	void *thread_status;
};


struct _thread_arch {
	/* nothing for now */
	int dummy;
};

typedef struct _thread_arch _thread_arch_t;

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_POSIX_THREAD_H_ */
