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

/**
 * @brief Callee-saved register context for cooperative context switching.
 */
struct _callee_saved {
	/** General-purpose register R16 (callee-saved). */
	uint32_t r16;
	/** General-purpose register R17 (callee-saved). */
	uint32_t r17;
	/** General-purpose register R18 (callee-saved). */
	uint32_t r18;
	/** General-purpose register R19 (callee-saved). */
	uint32_t r19;
	/** General-purpose register R20 (callee-saved). */
	uint32_t r20;
	/** General-purpose register R21 (callee-saved). */
	uint32_t r21;
	/** General-purpose register R22 (callee-saved). */
	uint32_t r22;
	/** General-purpose register R23 (callee-saved). */
	uint32_t r23;
	/** General-purpose register R24 (callee-saved). */
	uint32_t r24;
	/** General-purpose register R25 (callee-saved). */
	uint32_t r25;
	/** General-purpose register R26 (callee-saved). */
	uint32_t r26;
	/** General-purpose register R27 (callee-saved). */
	uint32_t r27;

	/** Stack pointer (R29). */
	uint32_t r29_sp;
	/** Frame pointer (R30). */
	uint32_t r30_fp;

	/** Link register (R31). */
	uint32_t r31_lr;
};

typedef struct _callee_saved _callee_saved_t;

/**
 * @brief Architecture-specific thread data.
 */
struct _thread_arch {
	/** Return value from arch_switch. */
	uint32_t swap_return_value;
};

typedef struct _thread_arch _thread_arch_t;

#endif /* _ASMLANGUAGE */

/* Hexagon requires 8-byte stack alignment. */
#define ARCH_STACK_PTR_ALIGN 8

#endif /* ZEPHYR_INCLUDE_ARCH_HEXAGON_THREAD_H_ */
