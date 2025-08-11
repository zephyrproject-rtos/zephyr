/*
 * Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * ARM64 Cortex-A ISRs wrapper
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <offsets_short.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/sw_isr_table.h>
#include <zephyr/drivers/interrupt_controller/gic.h>
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

	/* ++_current_cpu->nested to be checked by arch_is_in_isr() */
	get_cpu	x0
	ldr	w1, [x0, #___cpu_t_nested_OFFSET]
	add	w2, w1, #1
	str	w2, [x0, #___cpu_t_nested_OFFSET]

	/* If not nested: switch to IRQ stack and save current sp on it. */
	cbnz	w1, 1f
	ldr	x1, [x0, #___cpu_t_irq_stack_OFFSET]
	mov	x2, sp
	mov	sp, x1
	str	x2, [sp, #-16]!
#if defined(CONFIG_ARM64_SAFE_EXCEPTION_STACK)
	sub	x1, x1, #CONFIG_ISR_STACK_SIZE
	str	x1, [x0, #_cpu_offset_to_current_stack_limit]
#endif
1:
#ifdef CONFIG_SCHED_THREAD_USAGE
	bl	z_sched_usage_stop
#endif

#ifdef CONFIG_TRACING
	bl	sys_trace_isr_enter
#endif

	/* Get active IRQ number from the interrupt controller */
#if !defined(CONFIG_ARM_CUSTOM_INTERRUPT_CONTROLLER)
	bl	arm_gic_get_active
#else
	bl	z_soc_irq_get_active
#endif /* !CONFIG_ARM_CUSTOM_INTERRUPT_CONTROLLER */

	/* Preserve original IAR value */
	str	x0, [sp, #-16]!

#if CONFIG_GIC_VER == 2 && defined(CONFIG_SMP)
	/* Mask out GICC_IAR.CPUID [12:10] */
	bic	x0, x0, #0x1c00
#endif

#if CONFIG_GIC_VER >= 3
	/*
	 * Ignore Special INTIDs 1020..1023 see 2.2.1 of Arm Generic Interrupt Controller
	 * Architecture Specification GIC architecture version 3 and version 4
	 */
	cmp	x0, 1019
	b.le	oob
	cmp	x0, 1023
	b.gt	oob
	b	spurious_continue

oob:
#endif
	/* IRQ out of bounds */
	mov	x1, #(CONFIG_NUM_IRQS - 1)
	cmp	x0, x1
	b.hi	spurious_continue

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

spurious_continue:

	/* Retrieve original IAR value */
	ldr	x0, [sp], #16

	/* Signal end-of-interrupt */
#if !defined(CONFIG_ARM_CUSTOM_INTERRUPT_CONTROLLER)
	bl	arm_gic_eoi
#else
	bl	z_soc_irq_eoi
#endif /* !CONFIG_ARM_CUSTOM_INTERRUPT_CONTROLLER */

#ifdef CONFIG_TRACING
	bl	sys_trace_isr_exit
#endif

GTEXT(z_arm64_irq_done)
z_arm64_irq_done:
	/* if (--_current_cpu->nested != 0) exit */
	get_cpu	x0
	ldr	w1, [x0, #___cpu_t_nested_OFFSET]
	subs	w1, w1, #1
	str	w1, [x0, #___cpu_t_nested_OFFSET]
	bne	exit

	/* No more nested: retrieve the task's stack. */
	ldr	x1, [sp]
	mov	sp, x1

	/* retrieve pointer to the current thread */
	ldr	x1, [x0, #___cpu_t_current_OFFSET]

#if defined(CONFIG_ARM64_SAFE_EXCEPTION_STACK)
	/* arch_curr_cpu()->arch.current_stack_limit = thread->arch.stack_limit */
	ldr	x2, [x1, #_thread_offset_to_stack_limit]
	str	x2, [x0, #_cpu_offset_to_current_stack_limit]
#endif

	/*
	 * Get next thread to schedule with z_get_next_switch_handle().
	 * We pass it a NULL as we didn't save the whole thread context yet.
	 * If no scheduling is necessary then NULL will be returned.
	 */
	str	x1, [sp, #-16]!
	mov	x0, xzr
	bl	z_get_next_switch_handle
	ldr	x1, [sp], #16
	cbz	x0, exit

	/*
	 * Switch thread
	 * x0: new thread
	 * x1: old thread
	 */
	bl	z_arm64_context_switch

exit:
#ifdef CONFIG_STACK_SENTINEL
	bl	z_check_stack_sentinel
#endif
	b	z_arm64_exit_exc

