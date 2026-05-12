/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Hexagon exception stack frame definition
 */

#ifndef ZEPHYR_INCLUDE_ARCH_HEXAGON_EXCEPTION_H_
#define ZEPHYR_INCLUDE_ARCH_HEXAGON_EXCEPTION_H_

#ifndef _ASMLANGUAGE
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Exception stack frame.
 *
 * Saved register state at the time of an exception or interrupt.
 */
struct arch_esf {
	/** General-purpose register R0. */
	uint32_t r0;
	/** General-purpose register R1. */
	uint32_t r1;
	/** General-purpose register R2. */
	uint32_t r2;
	/** General-purpose register R3. */
	uint32_t r3;
	/** General-purpose register R4. */
	uint32_t r4;
	/** General-purpose register R5. */
	uint32_t r5;
	/** General-purpose register R6. */
	uint32_t r6;
	/** General-purpose register R7. */
	uint32_t r7;
	/** General-purpose register R8. */
	uint32_t r8;
	/** General-purpose register R9. */
	uint32_t r9;
	/** General-purpose register R10. */
	uint32_t r10;
	/** General-purpose register R11. */
	uint32_t r11;
	/** General-purpose register R12. */
	uint32_t r12;
	/** General-purpose register R13. */
	uint32_t r13;
	/** General-purpose register R14. */
	uint32_t r14;
	/** General-purpose register R15. */
	uint32_t r15;
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
	/** General-purpose register R28. */
	uint32_t r28;
	/** Stack pointer (R29). */
	uint32_t r29_sp;
	/** Frame pointer (R30). */
	uint32_t r30_fp;
	/** Link register (R31). */
	uint32_t r31_lr;

	/** Program counter. */
	uint32_t pc;

	/** Event type that triggered this exception. */
	uint32_t event_type;
	/** Event information registers (g0-g3). */
	uint32_t event_info[4];
};

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_HEXAGON_EXCEPTION_H_ */
