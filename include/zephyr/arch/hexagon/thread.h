/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Per-arch thread definition
 */

#ifndef ZEPHYR_INCLUDE_ARCH_HEXAGON_THREAD_H_
#define ZEPHYR_INCLUDE_ARCH_HEXAGON_THREAD_H_

#ifndef _ASMLANGUAGE
#include <zephyr/types.h>

/* Callee-saved register context for cooperative context switching */
struct _callee_saved {
	/* Callee-saved registers (r16-r27) */
	uint32_t r16;
	uint32_t r17;
	uint32_t r18;
	uint32_t r19;
	uint32_t r20;
	uint32_t r21;
	uint32_t r22;
	uint32_t r23;
	uint32_t r24;
	uint32_t r25;
	uint32_t r26;
	uint32_t r27;

	/* Stack pointer and frame pointer */
	uint32_t r29_sp;
	uint32_t r30_fp;

	/* Link register */
	uint32_t r31_lr;
};

typedef struct _callee_saved _callee_saved_t;

/* Extended thread structure */
struct _thread_arch {
	/* Return value from arch_switch */
	uint32_t swap_return_value;
};

typedef struct _thread_arch _thread_arch_t;

#endif /* _ASMLANGUAGE */

/**
 * @brief Required alignment of the stack pointer.
 *
 * Hexagon requires 8-byte stack alignment.
 */
#define ARCH_STACK_PTR_ALIGN 8

#endif /* ZEPHYR_INCLUDE_ARCH_HEXAGON_THREAD_H_ */
