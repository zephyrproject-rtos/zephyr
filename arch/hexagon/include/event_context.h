/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Hexagon event context saved by the assembly EVENT_ENTRY macro.
 *
 * Must match the register save layout in event_handlers.S exactly; shared
 * by the C event handler (irq_manage.c).
 *
 * Offsets below correspond to the EVENT_CTX_* constants in
 * event_handlers.S:
 *
 *   0x00  r0_r1
 *   0x08  r2_r3
 *   0x10  r4_r5
 *   0x18  r6_r7
 *   0x20  r8_r9
 *   0x28  r10_r11
 *   0x30  r12_r13
 *   0x38  r14_r15
 *   0x40  pred_regs
 *   0x44  link_reg
 *   0x48  gelr
 *   0x4c  gsr
 *   0x50  sa0
 *   0x54  lc0
 *   0x58  sa1
 *   0x5c  lc1
 *   0x60  m0
 *   0x64  m1
 *   0x68  usr
 *   0x6c  r28
 *   0x70  gosp (G2; reused as EVENT_EXIT scratch)
 *   0x74  badva (G3)
 *   -- total size 0x78 --
 *
 * With CONFIG_DEBUG_COREDUMP, fault events (machine check, exception,
 * debug) also save r16-r27 at 0x78-0xa7, for a total size of 0xa8.
 */

#ifndef ZEPHYR_ARCH_HEXAGON_INCLUDE_EVENT_CONTEXT_H_
#define ZEPHYR_ARCH_HEXAGON_INCLUDE_EVENT_CONTEXT_H_

#ifndef _ASMLANGUAGE

#include <stddef.h>
#include <zephyr/types.h>

/** @brief Volatile register context saved on the stack by EVENT_ENTRY. */
struct event_context {
	uint32_t r0_r1[2];         /**< r0, r1 */
	uint32_t r2_r3[2];         /**< r2, r3 */
	uint32_t r4_r5[2];         /**< r4, r5 */
	uint32_t r6_r7[2];         /**< r6 (syscall num), r7 */
	uint32_t r8_r9[2];         /**< r8, r9 */
	uint32_t r10_r11[2];       /**< r10, r11 */
	uint32_t r12_r13[2];       /**< r12, r13 */
	uint32_t r14_r15[2];       /**< r14, r15 */
	uint32_t pred_regs;        /**< P3:0 packed as one 32-bit word */
	uint32_t link_reg;         /**< R31 (LR) */
	uint32_t gelr;             /**< GELR: return PC (G0 at event entry) */
	uint32_t gsr;              /**< GSR: guest status (G1 at event entry) */
	uint32_t sa0;              /**< hardware loop SA0 */
	uint32_t lc0;              /**< hardware loop LC0 */
	uint32_t sa1;              /**< hardware loop SA1 */
	uint32_t lc1;              /**< hardware loop LC1 */
	uint32_t m0;               /**< modifier register M0 */
	uint32_t m1;               /**< modifier register M1 */
	uint32_t usr;              /**< user status register USR */
	uint32_t r28;              /**< R28 (caller-saved, not in r0-r15 pairs) */
	uint32_t gosp;             /**< GOSP (G2), reused as EVENT_EXIT scratch */
	uint32_t badva;            /**< GBADVA (G3), raw event-time value */
#ifdef CONFIG_DEBUG_COREDUMP
	uint32_t r16_r27[12];      /**< r16-r27, fault events only */
#endif
};

/** @brief Size of struct event_context in bytes (must equal EVENT_CTX_SIZE). */
#define EVENT_CTX_SIZE_C sizeof(struct event_context)

/* EVENT_CTX_SIZE must match the save layout in event_handlers.S. */
#ifdef CONFIG_DEBUG_COREDUMP
#define EVENT_CTX_SIZE 0xa8
#else
#define EVENT_CTX_SIZE 0x78
#endif

/*
 * Overhead appended by allocframe above EVENT_CTX_SIZE: old FP/LR stored
 * as a pair (8 bytes) right above the event context body.
 * Pre-exception SP = (uint8_t *)ctx + EVENT_CTX_SIZE + EVENT_ENTRY_ALLOCFRAME_OVERHEAD.
 */
#define EVENT_ENTRY_ALLOCFRAME_OVERHEAD 8

/* Room below FRAMELIMIT for the unchecked EVENT_ENTRY frame. */
#define HEXAGON_STACK_LIMIT_HEADROOM (EVENT_CTX_SIZE + EVENT_ENTRY_ALLOCFRAME_OVERHEAD)

BUILD_ASSERT(EVENT_CTX_SIZE_C == EVENT_CTX_SIZE,
	     "struct event_context size differs from EVENT_CTX_SIZE; "
	     "update both event_context.h and event_handlers.S");
#ifdef CONFIG_DEBUG_COREDUMP
BUILD_ASSERT(offsetof(struct event_context, r16_r27) == 0x78,
	     "r16_r27 must match EVENT_CTX_R16_R17 in event_handlers.S");
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_ARCH_HEXAGON_INCLUDE_EVENT_CONTEXT_H_ */
