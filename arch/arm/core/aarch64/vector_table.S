/*
 * Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Populated vector table
 */

#include <toolchain.h>
#include <linker/sections.h>
#include <arch/cpu.h>
#include <offsets_short.h>
#include "vector_table.h"

_ASM_FILE_PROLOGUE

/*
 * Save volatile registers, x30, SPSR_EL1 and ELR_EL1
 *
 * Save the volatile registers and x30 on the process stack. This is
 * needed if the thread is switched out because they can be clobbered by the
 * ISR and/or context switch.
 */

.macro z_arm64_enter_exc xreg0, xreg1

	/* Switch to SP_EL0 */
	msr	spsel, #0

	/*
	 * Two things can happen to the remaining registers:
	 *
	 * - No context-switch: in this case x19-x28 are callee-saved register
	 *   so we can be sure they are not going to be clobbered by ISR.
	 * - Context-switch: the callee-saved registers are saved by
	 *   z_arm64_context_switch() in the kernel structure.
	 */

	sub	sp, sp, ___esf_t_SIZEOF

	stp	x0, x1, [sp, ___esf_t_x0_x1_OFFSET]
	stp	x2, x3, [sp, ___esf_t_x2_x3_OFFSET]
	stp	x4, x5, [sp, ___esf_t_x4_x5_OFFSET]
	stp	x6, x7, [sp, ___esf_t_x6_x7_OFFSET]
	stp	x8, x9, [sp, ___esf_t_x8_x9_OFFSET]
	stp	x10, x11, [sp, ___esf_t_x10_x11_OFFSET]
	stp	x12, x13, [sp, ___esf_t_x12_x13_OFFSET]
	stp	x14, x15, [sp, ___esf_t_x14_x15_OFFSET]
	stp	x16, x17, [sp, ___esf_t_x16_x17_OFFSET]
	stp	x18, x30, [sp, ___esf_t_x18_x30_OFFSET]

	mrs	\xreg0, spsr_el1
	mrs	\xreg1, elr_el1
	stp	\xreg0, \xreg1, [sp, ___esf_t_spsr_elr_OFFSET]

.endm

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
 * |          + 0x400 | Synchronous      | Lower EL using AArch64  |
 * |          + 0x480 | IRQ / vIRQ       |                         |
 * |          + 0x500 | FIQ / vFIQ       |                         |
 * |          + 0x580 | SError / vSError |                         |
 * +------------------+------------------+-------------------------+
 * |          + 0x600 | Synchronous      | Lower EL using AArch32  |
 * |          + 0x680 | IRQ / vIRQ       |                         |
 * |          + 0x700 | FIQ / vFIQ       |                         |
 * |          + 0x780 | SError / vSError |                         |
 * +------------------+------------------+-------------------------+
 */

	/* The whole table must be 2K aligned */
SECTION_SUBSEC_FUNC(exc_vector_table,_vector_table_section,_vector_table)
	.align 11

	/* Current EL with SP0 / Synchronous */
	.align 7
	z_arm64_enter_exc x0, x1
	b	z_arm64_sync_exc

	/* Current EL with SP0 / IRQ */
	.align 7
	z_arm64_enter_exc x0, x1
#ifdef CONFIG_GEN_SW_ISR_TABLE
	b 	_isr_wrapper
#else
	b	z_irq_spurious
#endif

	/* Current EL with SP0 / FIQ */
	.align 7
	b 	.

	/* Current EL with SP0 / SError */
	.align 7
	z_arm64_enter_exc x0, x1
	b	z_arm64_serror

	/* Current EL with SPx / Synchronous */
	.align 7
	z_arm64_enter_exc x0, x1
	b	z_arm64_sync_exc

	/* Current EL with SPx / IRQ */
	.align 7
	z_arm64_enter_exc x0, x1
#ifdef CONFIG_GEN_SW_ISR_TABLE
	b 	_isr_wrapper
#else
	b	z_irq_spurious
#endif

	/* Current EL with SPx / FIQ */
	.align 7
	b	.

	/* Current EL with SPx / SError */
	.align 7
	z_arm64_enter_exc x0, x1
	b	z_arm64_serror

	/* Lower EL using AArch64 / Synchronous */
	.align 7
	b	.

	/* Lower EL using AArch64 / IRQ */
	.align 7
	b	.

	/* Lower EL using AArch64 / FIQ */
	.align 7
	b	.

	/* Lower EL using AArch64 / SError */
	.align 7
	b	.

	/* Lower EL using AArch32 / Synchronous */
	.align 7
	b	.

	/* Lower EL using AArch32 / IRQ */
	.align 7
	b	.

	/* Lower EL using AArch32 / FIQ */
	.align 7
	b	.

	/* Lower EL using AArch32 / SError */
	.align 7
	b	.

GTEXT(z_arm64_serror)
SECTION_FUNC(TEXT, z_arm64_serror)

	mov	x1, sp
	mov	x0, #0 /* K_ERR_CPU_EXCEPTION */

	bl	z_arm64_fatal_error
	/* Return here only in case of recoverable error */
	b	z_arm64_exit_exc

/*
 * Restore volatile registers, x30, SPSR_EL1 and ELR_EL1
 *
 * This is the common exit point for z_arm64_sync_exc() and _isr_wrapper().
 */

GTEXT(z_arm64_exit_exc)
SECTION_FUNC(TEXT, z_arm64_exit_exc)

	ldp	x0, x1, [sp, ___esf_t_spsr_elr_OFFSET]
	msr	spsr_el1, x0
	msr	elr_el1, x1

	ldp	x0, x1, [sp, ___esf_t_x0_x1_OFFSET]
	ldp	x2, x3, [sp, ___esf_t_x2_x3_OFFSET]
	ldp	x4, x5, [sp, ___esf_t_x4_x5_OFFSET]
	ldp	x6, x7, [sp, ___esf_t_x6_x7_OFFSET]
	ldp	x8, x9, [sp, ___esf_t_x8_x9_OFFSET]
	ldp	x10, x11, [sp, ___esf_t_x10_x11_OFFSET]
	ldp	x12, x13, [sp, ___esf_t_x12_x13_OFFSET]
	ldp	x14, x15, [sp, ___esf_t_x14_x15_OFFSET]
	ldp	x16, x17, [sp, ___esf_t_x16_x17_OFFSET]
	ldp	x18, x30, [sp, ___esf_t_x18_x30_OFFSET]

	add	sp, sp, ___esf_t_SIZEOF

	/*
	 * In general in the ELR_EL1 register we can find:
	 *
	 * - The address of ret in z_arm64_call_svc()
	 * - The address of the next instruction at the time of the IRQ when the
	 *   thread was switched out.
	 * - The address of z_thread_entry() for new threads (see thread.c).
	 */
	eret

