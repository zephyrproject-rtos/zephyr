/*
 * Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * ARM64 Cortex-A ISRs wrapper
 */

#include <toolchain.h>
#include <linker/sections.h>
#include <offsets_short.h>
#include <arch/cpu.h>
#include <sw_isr_table.h>
#include <drivers/interrupt_controller/gic.h>
#include "macro_priv.inc"

_ASM_FILE_PROLOGUE

GDATA(_sw_isr_table)

/*
 * Wrapper around ISRs when inserted in software ISR table
 *
 * When inserted in the vector table, _isr_wrapper() demuxes the ISR table
 * using the running interrupt number as the index, and invokes the registered
 * ISR with its corresponding argument. When returning from the ISR, it
 * determines if a context switch needs to happen.
 */

GTEXT(_isr_wrapper)
SECTION_FUNC(TEXT, _isr_wrapper)

	/* ++(_kernel->nested) to be checked by arch_is_in_isr() */
	inc_nest_counter x0, x1

#ifdef CONFIG_TRACING
	bl	sys_trace_isr_enter
#endif

	/* Get active IRQ number from the interrupt controller */
#if !defined(CONFIG_ARM_CUSTOM_INTERRUPT_CONTROLLER)
	bl	arm_gic_get_active
#else
	bl	z_soc_irq_get_active
#endif /* !CONFIG_ARM_CUSTOM_INTERRUPT_CONTROLLER */

	/* Spurious */
	cmp	x0, #(GIC_INTID_SPURIOUS)
	beq	spurious_continue

	stp	x0, x1, [sp, #-16]!

	/* Retrieve the interrupt service routine */
	ldr	x1, =_sw_isr_table
	add	x1, x1, x0, lsl #4	/* table is 16-byte wide */
	ldp	x0, x3, [x1] /* arg in x0, ISR in x3 */

	/*
	 * Call the ISR. Unmask and mask again the IRQs to support nested
	 * exception handlers
	 */
	msr	daifclr, #(DAIFCLR_IRQ_BIT)
	blr	x3
	msr	daifset, #(DAIFSET_IRQ_BIT)

	/* Signal end-of-interrupt */
	ldp	x0, x1, [sp], #16

spurious_continue:
#if !defined(CONFIG_ARM_CUSTOM_INTERRUPT_CONTROLLER)
	bl	arm_gic_eoi
#else
	bl	z_soc_irq_eoi
#endif /* !CONFIG_ARM_CUSTOM_INTERRUPT_CONTROLLER */

#ifdef CONFIG_TRACING
	bl	sys_trace_isr_exit
#endif

	/* if (--(_kernel->nested) != 0) exit */
	dec_nest_counter x0, x1
	bne	exit

	/*
	 * z_arch_get_next_switch_handle() is returning:
	 *
	 * - The next thread to schedule in x0
	 * - The current thread in x1. This value is returned using the
	 *   **old_thread parameter, so we need to make space on the stack for
	 *   that.
	 */
	stp	x1, xzr, [sp, #-16]!
	mov	x0, sp
	bl	z_arch_get_next_switch_handle
	ldp	x1, xzr, [sp], #16

	/*
	 * x0: 1st thread in the ready queue
	 * x1: _current thread
	 */
	cmp	x0, x1
	beq	exit

	/* Switch thread */
	bl	z_arm64_context_switch

exit:
#ifdef CONFIG_STACK_SENTINEL
	bl	z_check_stack_sentinel
#endif
	b	z_arm64_exit_exc

