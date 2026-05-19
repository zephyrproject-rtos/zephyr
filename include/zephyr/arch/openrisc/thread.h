/*
 * Copyright (c) 2025 NVIDIA Corporation
 *
 * based on include/arch/mips/thread.h
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

#ifndef ZEPHYR_INCLUDE_ARCH_OPENRISC_THREAD_H_
#define ZEPHYR_INCLUDE_ARCH_OPENRISC_THREAD_H_

#ifndef _ASMLANGUAGE
#include <zephyr/types.h>

/*
 * The following structure defines the list of registers that need to be
 * saved/restored when a cooperative context switch occurs.
 */
struct _callee_saved {
	uint32_t r1;	/* Stack pointer */
	uint32_t r2;	/* Frame Pointer */
	uint32_t r9;	/* Return Address */

	uint32_t r10;
	uint32_t r14;
	uint32_t r16;
	uint32_t r18;
	uint32_t r20;
	uint32_t r22;
	uint32_t r24;
	uint32_t r26;
	uint32_t r28;
	uint32_t r30;
};
typedef struct _callee_saved _callee_saved_t;

struct _thread_arch {
};

typedef struct _thread_arch _thread_arch_t;

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_OPENRISC_THREAD_H_ */
