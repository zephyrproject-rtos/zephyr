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
 * Restore volatile registers, x30, SPSR_EL1 and ELR_EL1
 *
 * This is the common exit point for z_arm64_sync_exc() and _isr_wrapper().
 */

.macro z_arm64_exit_exc xreg0, xreg1
	ldp	\xreg0, \xreg1, [sp, ___esf_t_spsr_elr_OFFSET]
	msr	spsr_el1, \xreg0
	msr	elr_el1, \xreg1

	ldp	x18, x30, [sp, ___esf_t_x18_x30_OFFSET]
	ldp	x16, x17, [sp, ___esf_t_x16_x17_OFFSET]
	ldp	x14, x15, [sp, ___esf_t_x14_x15_OFFSET]
	ldp	x12, x13, [sp, ___esf_t_x12_x13_OFFSET]
	ldp	x10, x11, [sp, ___esf_t_x10_x11_OFFSET]
	ldp	x8, x9, [sp, ___esf_t_x8_x9_OFFSET]
	ldp	x6, x7, [sp, ___esf_t_x6_x7_OFFSET]
	ldp	x4, x5, [sp, ___esf_t_x4_x5_OFFSET]
	ldp	x2, x3, [sp, ___esf_t_x2_x3_OFFSET]
	ldp	x0, x1, [sp, ___esf_t_x0_x1_OFFSET]

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
