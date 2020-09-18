/*
 * Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MACRO_PRIV_INC_
#define _MACRO_PRIV_INC_

#ifdef _ASMLANGUAGE

/*
 * Save volatile registers, x30, SPSR_EL1 and ELR_EL1
 *
 * Save the volatile registers and x30 on the process stack. This is
 * needed if the thread is switched out because they can be clobbered by the
 * ISR and/or context switch.
 */

.macro z_arm64_enter_exc xreg0, xreg1
	/*
	 * Two things can happen to the remaining registers:
	 *
	 * - No context-switch: in this case x19-x28 are callee-saved register
	 *   so we can be sure they are not going to be clobbered by ISR.
	 * - Context-switch: the callee-saved registers are saved by
	 *   z_arm64_context_switch() in the kernel structure.
	 */
	stp	x0, x1, [sp, #-16]!
	stp	x2, x3, [sp, #-16]!
	stp	x4, x5, [sp, #-16]!
	stp	x6, x7, [sp, #-16]!
	stp	x8, x9, [sp, #-16]!
	stp	x10, x11, [sp, #-16]!
	stp	x12, x13, [sp, #-16]!
	stp	x14, x15, [sp, #-16]!
	stp     x16, x17, [sp, #-16]!
	stp     x18, x30, [sp, #-16]!

	mrs	\xreg0, spsr_el1
	mrs	\xreg1, elr_el1
	stp	\xreg0, \xreg1, [sp, #-16]!
.endm

/*
 * Restore volatile registers, x30, SPSR_EL1 and ELR_EL1
 *
 * This is the common exit point for z_arm64_sync_exc() and _isr_wrapper().
 */

.macro z_arm64_exit_exc xreg0, xreg1
	ldp	\xreg0, \xreg1, [sp], #16
	msr	spsr_el1, \xreg0
	msr	elr_el1, \xreg1

	ldp	x18, x30, [sp], #16
	ldp	x16, x17, [sp], #16
	ldp	x14, x15, [sp], #16
	ldp	x12, x13, [sp], #16
	ldp	x10, x11, [sp], #16
	ldp	x8, x9, [sp], #16
	ldp	x6, x7, [sp], #16
	ldp	x4, x5, [sp], #16
	ldp	x2, x3, [sp], #16
	ldp	x0, x1, [sp], #16

	/*
	 * In general in the ELR_EL1 register we can find:
	 *
	 * - The address of ret in z_arm64_call_svc()
	 * - The address of the next instruction at the time of the IRQ when the
	 *   thread was switched out.
	 * - The address of z_thread_entry() for new threads (see thread.c).
	 */
	eret
.endm

/*
 * Increment nested counter
 */

.macro inc_nest_counter xreg0, xreg1
	ldr	\xreg0, =_kernel
	ldr	\xreg1, [\xreg0, #_kernel_offset_to_nested]
	add	\xreg1, \xreg1, #1
	str	\xreg1, [\xreg0, #_kernel_offset_to_nested]
.endm

/*
 * Decrement nested counter
 */

.macro dec_nest_counter xreg0, xreg1
	ldr	\xreg0, =_kernel
	ldr	\xreg1, [\xreg0, #_kernel_offset_to_nested]
	sub	\xreg1, \xreg1, #1
	str	\xreg1, [\xreg0, #_kernel_offset_to_nested]
.endm

#endif /* _ASMLANGUAGE */

#endif /* _MACRO_PRIV_INC_ */
