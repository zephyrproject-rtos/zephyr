/*
 * Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Populated vector table
 */

#include <toolchain.h>
#include <linker/sections.h>
#include <arch/cpu.h>
#include "vector_table.h"
#include "macro_priv.inc"

_ASM_FILE_PROLOGUE

/*
 * Four types of exceptions:
 * - synchronous: aborts from MMU, SP/CP alignment checking, unallocated
 *   instructions, SVCs/SMCs/HVCs, ...)
 * - IRQ: group 1 (normal) interrupts
 * - FIQ: group 0 or secure interrupts
 * - SError: fatal system errors
 *
 * Four different contexts:
 * - from same exception level, when using the SP_EL0 stack pointer
 * - from same exception level, when using the SP_ELx stack pointer
 * - from lower exception level, when this is AArch64
 * - from lower exception level, when this is AArch32
 *
 * +------------------+------------------+-------------------------+
 * |     Address      |  Exception type  |       Description       |
 * +------------------+------------------+-------------------------+
 * | VBAR_ELn + 0x000 | Synchronous      | Current EL with SP0     |
 * |          + 0x080 | IRQ / vIRQ       |                         |
 * |          + 0x100 | FIQ / vFIQ       |                         |
 * |          + 0x180 | SError / vSError |                         |
 * +------------------+------------------+-------------------------+
 * |          + 0x200 | Synchronous      | Current EL with SPx     |
 * |          + 0x280 | IRQ / vIRQ       |                         |
 * |          + 0x300 | FIQ / vFIQ       |                         |
 * |          + 0x380 | SError / vSError |                         |
 * +------------------+------------------+-------------------------+
 * |          + 0x400 | Synchronous      | Lower EL using  AArch64 |
 * |          + 0x480 | IRQ / vIRQ       |                         |
 * |          + 0x500 | FIQ / vFIQ       |                         |
 * |          + 0x580 | SError / vSError |                         |
 * +------------------+------------------+-------------------------+
 * |          + 0x600 | Synchronous      | Lower EL using AArch64  |
 * |          + 0x680 | IRQ / vIRQ       |                         |
 * |          + 0x700 | FIQ / vFIQ       |                         |
 * |          + 0x780 | SError / vSError |                         |
 * +------------------+------------------+-------------------------+
 */

	/* The whole table must be 2K aligned */
	.align 11
SECTION_SUBSEC_FUNC(exc_vector_table,_vector_table_section,_vector_table)

	/* Current EL with SP0 / Synchronous */
	.align 7
	b	.

	/* Current EL with SP0 / IRQ */
	.align 7
	b	.

	/* Current EL with SP0 / FIQ */
	.align 7
	b 	.

	/* Current EL with SP0 / SError */
	.align 7
	b	.

	/* Current EL with SPx / Synchronous */
	.align 7
	b	z_arm64_svc

	/* Current EL with SPx / IRQ */
	.align 7
	b	_isr_wrapper

	/* Current EL with SPx / FIQ */
	.align 7
	b	.

	/* Current EL with SPx / SError */
	.align 7
	z_arm64_enter_exc x0, x1, x2

	mov	x1, sp
	mov	x0, #0 /* K_ERR_CPU_EXCEPTION */

	b	z_arm64_fatal_error

