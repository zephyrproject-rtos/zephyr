/*
 * Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MACRO_PRIV_INC_
#define _MACRO_PRIV_INC_

#ifdef _ASMLANGUAGE

/**
 * @brief Save volatile registers
 *
 * Save the volatile registers and x30 on the process stack. This is
 * needed if the thread is switched out because they can be clobbered by the
 * ISR and/or context switch.
 *
 * @return N/A
 */

.macro z_arm64_enter_exc xreg0, xreg1, xreg2
	/*
	 * Two things can happen:
	 *
	 * - No context-switch: in this case x19-x28 are callee-saved register
	 *   so we can be sure they are not going to be clobbered by ISR.
	 * - Context-switch: the callee-saved registers are saved by
	 *   z_arm64_pendsv() in the kernel structure.
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

	/*
	 * Store SPSR_ELn and ELR_ELn. This is needed to support nested
	 * exception handlers
	 */
	mrs	\xreg1, spsr_el1
	mrs	\xreg2, elr_el1
	stp	\xreg1, \xreg2, [sp, #-16]!
.endm

/**
 * @brief Restore volatile registers and x30
 *
 * This is the common exit point for z_arm64_pendsv() and _isr_wrapper(). We
 * restore the registers saved on the process stack including X30. The return
 * address used by eret (in ELR_ELn) is either restored by z_arm64_pendsv() if
 * a context-switch happened or not touched at all by the ISR if there was no
 * context-switch.
 *
 * @return N/A
 */

.macro z_arm64_exit_exc xreg0, xreg1, xreg2
	/*
	 * Restore SPSR_ELn and ELR_ELn. This is needed to support nested
	 * exception handlers
	 */
	ldp	\xreg0, \xreg1, [sp], #16
	msr	spsr_el1, \xreg0
	msr	elr_el1, \xreg1

	/*
	 * In x30 we can have:
	 *
	 * - The address of irq_unlock() in swap.c when swapping in a thread
	 *   that was cooperatively swapped out (used by ret in
	 *   z_arm64_call_svc())
	 * - A previos generic value if the thread that we are swapping in was
	 *   swapped out preemptively by the ISR.
	 */
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
	 * In general in the  ELR_ELn register we can find:
	 *
	 * - The address of ret in z_arm64_call_svc() in case of arch_swap()
	 *   (see swap.c)
	 * - The address of the next instruction at the time of the IRQ when the
	 *   thread was switched out.
	 * - The address of z_thread_entry() for new threads (see thread.c).
	 */
	eret
.endm

/**
 * @brief Increment nested counter
 *
 * @return N/A
 */

.macro inc_nest_counter xreg0, xreg1
	ldr	\xreg0, =_kernel
	ldr	\xreg1, [\xreg0, #_kernel_offset_to_nested]
	add	\xreg1, \xreg1, #1
	str	\xreg1, [\xreg0, #_kernel_offset_to_nested]
.endm

/**
 * @brief Decrement nested counter
 *
 * @return N/A
 */

.macro dec_nest_counter xreg0, xreg1
	ldr	\xreg0, =_kernel
	ldr	\xreg1, [\xreg0, #_kernel_offset_to_nested]
	sub	\xreg1, \xreg1, #1
	str	\xreg1, [\xreg0, #_kernel_offset_to_nested]
.endm

#endif /* _ASMLANGUAGE */

#endif /* _MACRO_PRIV_INC_ */
