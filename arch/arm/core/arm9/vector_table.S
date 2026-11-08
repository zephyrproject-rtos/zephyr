/*
 * Copyright (c) 2026 Takayuki Goto
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARMv5 exception vector table
 *
 * ARMv5 cores fetch their exception vectors either from 0x00000000 or, when
 * SCTLR.V is set, from 0xFFFF0000. Neither address is necessarily where the
 * image happens to be linked, so the table is written in a fully position
 * independent form: eight `ldr pc, [pc, #24]` instructions followed by the
 * eight handler addresses. The resulting 64-byte block can be memcpy()'d to
 * the vector base by z_armv5_vector_table_init().
 *
 * `ldr pc, [pc, #24]` at offset N reads the word at N + 8 + 24 == N + 32,
 * i.e. entry N of the address table that starts right after the branches.
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include "vector_table.h"

_ASM_FILE_PROLOGUE

SECTION_SUBSEC_FUNC(exc_vector_table,_vector_table_section,_vector_table)
	ldr pc, [pc, #24]	/* reset             offset 0x00 */
	ldr pc, [pc, #24]	/* undefined instr   offset 0x04 */
	ldr pc, [pc, #24]	/* supervisor call   offset 0x08 */
	ldr pc, [pc, #24]	/* prefetch abort    offset 0x0c */
	ldr pc, [pc, #24]	/* data abort        offset 0x10 */
	ldr pc, [pc, #24]	/* reserved          offset 0x14 */
	ldr pc, [pc, #24]	/* IRQ               offset 0x18 */
	ldr pc, [pc, #24]	/* FIQ               offset 0x1c */

GDATA(_vector_table_addrs)
_vector_table_addrs:
	.word z_arm_reset
	.word z_arm_undef_instruction
	.word z_arm_svc
	.word z_arm_prefetch_abort
	.word z_arm_data_abort
	.word z_arm_reserved
#ifdef CONFIG_GEN_SW_ISR_TABLE
	.word _isr_wrapper
#else
	.word z_irq_spurious
#endif
	.word z_arm_fiq
