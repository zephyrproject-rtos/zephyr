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
 * by the C event handler (irq_manage.c) and the GDB stub (gdbstub.c).
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
 *   0x70  scratch
 *   -- 0x78 (EVENT_CTX_SIZE without debug consumers) --
 *
 * With CONFIG_DEBUG_COREDUMP or CONFIG_GDBSTUB, EVENT_ENTRY also saves:
 *   0x78  r16_r17
 *   0x80  r18_r19
 *   0x88  r20_r21
 *   0x90  r22_r23
 *   0x98  r24_r25
 *   0xa0  r26_r27
 *   0xa8  gp
 *   0xac  ugp
 *   -- total size 0xb0 --
 */

#ifndef ZEPHYR_ARCH_HEXAGON_INCLUDE_EVENT_CONTEXT_H_
#define ZEPHYR_ARCH_HEXAGON_INCLUDE_EVENT_CONTEXT_H_

#ifndef _ASMLANGUAGE

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
	uint32_t scratch;          /**< temporary slot used during EVENT_EXIT */
	uint32_t padding;          /**< align extended fields to the assembly offset */
#if defined(CONFIG_GDBSTUB) || defined(CONFIG_DEBUG_COREDUMP)
	uint32_t r16_r17[2];       /**< r16, r17 */
	uint32_t r18_r19[2];       /**< r18, r19 */
	uint32_t r20_r21[2];       /**< r20, r21 */
	uint32_t r22_r23[2];       /**< r22, r23 */
	uint32_t r24_r25[2];       /**< r24, r25 */
	uint32_t r26_r27[2];       /**< r26, r27 */
	uint32_t gp;               /**< global pointer */
	uint32_t ugp;              /**< user global pointer */
#endif
};

/** @brief Size of struct event_context in bytes (must equal EVENT_CTX_SIZE). */
#define EVENT_CTX_SIZE_C sizeof(struct event_context)

/*
 * EVENT_CTX_SIZE as a numeric constant matching event_handlers.S, used by
 * gdbstub.c to locate the pre-exception SP/FP above the saved context
 * frame. If struct event_context grows, update both definitions.
 */
#if defined(CONFIG_GDBSTUB) || defined(CONFIG_DEBUG_COREDUMP)
#define EVENT_CTX_SIZE 0xb0
#else
#define EVENT_CTX_SIZE 0x78
#endif

/*
 * Overhead appended by allocframe above EVENT_CTX_SIZE: old FP/LR stored
 * as a pair (8 bytes) right above the event context body.
 * Pre-exception SP = (uint8_t *)ctx + EVENT_CTX_SIZE + EVENT_ENTRY_ALLOCFRAME_OVERHEAD.
 */
#define EVENT_ENTRY_ALLOCFRAME_OVERHEAD 8

BUILD_ASSERT(EVENT_CTX_SIZE_C == EVENT_CTX_SIZE,
	     "struct event_context size differs from EVENT_CTX_SIZE; "
	     "update both event_context.h and event_handlers.S");

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_ARCH_HEXAGON_INCLUDE_EVENT_CONTEXT_H_ */
