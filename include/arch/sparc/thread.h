/*
 * Copyright (c) 2018 ispace, inc.
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

#ifndef ZEPHYR_INCLUDE_ARCH_SPARC_THREAD_H_
#define ZEPHYR_INCLUDE_ARCH_SPARC_THREAD_H_

#ifndef _ASMLANGUAGE
#include <zephyr/types.h>

struct _callee_saved {
	uint32_t l0;
	uint32_t l1;
	uint32_t l2;
	uint32_t l3;
	uint32_t l4;
	uint32_t l5;
	uint32_t l6;
	uint32_t l7;

	uint32_t i0;
	uint32_t i1;
	uint32_t i2;
	uint32_t i3;
	uint32_t i4;
	uint32_t i5;
	uint32_t i6;
	uint32_t i7;
};

typedef struct _callee_saved _callee_saved_t;

struct _thread_arch {
	/* FIXME */
};

typedef struct _thread_arch _thread_arch_t;

#endif /* _ASMLANGUAGE */

#endif /* EPHYR_ARCH_SPARC_INCLUDE_KERNEL_ARCH_THREAD_H_ */
