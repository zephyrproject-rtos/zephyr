/*
 * Copyright (c) 2026 Takayuki Goto
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARMv5 IRQ entry point
 *
 * Demuxes the active interrupt through _sw_isr_table and, on the way out of
 * the outermost interrupt, asks the scheduler whether another thread should
 * run.
 *
 * ARMv5 cores have no architectural interrupt controller, so
 * CONFIG_ARM_CUSTOM_INTERRUPT_CONTROLLER is mandatory and the SoC provides
 * z_soc_irq_get_active()/z_soc_irq_eoi().
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <offsets_short.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/sw_isr_table.h>
#include "macro_priv.inc"

_ASM_FILE_PROLOGUE

GDATA(_sw_isr_table)
GDATA(z_interrupt_stacks)

GTEXT(_isr_wrapper)
GTEXT(z_armv5_irq_done)
GTEXT(z_armv5_exit_exc)
GTEXT(z_arm_context_switch)

SECTION_FUNC(TEXT, _isr_wrapper)
	sub	lr, lr, #4
	z_armv5_enter_exc

	/* Increment interrupt nesting count */
	get_cpu	r2
	ldr	r0, [r2, #___cpu_t_nested_OFFSET]
	add	r0, r0, #1
	str	r0, [r2, #___cpu_t_nested_OFFSET]

	/*
	 * On the outermost interrupt, move off the interrupted thread's stack
	 * and onto the dedicated interrupt stack so that ISR stack usage does
	 * not depend on whichever thread happened to be running. The old sp is
	 * pushed so that z_armv5_irq_done() can restore it.
	 */
	cmp	r0, #1
	bhi	1f
	mov	r0, sp
	ldr	sp, =(z_interrupt_stacks + CONFIG_ISR_STACK_SIZE)
	stmdb	sp!, {r0, r1}
1:

#ifdef CONFIG_TRACING_ISR
	bl	sys_trace_isr_enter
#endif

#ifdef CONFIG_PM
	/*
	 * All interrupts are still disabled here, so the tickless idle
	 * bookkeeping cannot be interrupted halfway through.
	 */
	ldr	r2, =_kernel
	ldr	r0, [r2, #_kernel_offset_to_idle]
	cmp	r0, #0
	beq	_idle_state_cleared
	mov	r1, #0
	str	r1, [r2, #_kernel_offset_to_idle]
	bl	pm_system_resume
_idle_state_cleared:
#endif /* CONFIG_PM */

	bl	z_soc_irq_get_active
	stmdb	sp!, {r0, r1}

	/* Skip the dispatch entirely for a spurious interrupt. */
	ldr	r1, =(CONFIG_NUM_IRQS * 8)
	mov	r2, r0, lsl #3
	cmp	r2, r1
	bhs	spurious_continue

	ldr	r1, =_sw_isr_table
	add	r1, r1, r2
	ldm	r1, {r0, r3}			/* arg in r0, ISR in r3 */

#ifdef CONFIG_ARMV5_IRQ_NESTING
	/*
	 * Only unmask while running the ISR when the SoC's interrupt
	 * controller masks the active source itself. ARMv5 designs usually
	 * use a level sensitive controller (PL190, PXA ICU, ...) that keeps
	 * the line asserted until the peripheral is serviced, in which case
	 * unmasking here would re-enter this handler immediately and recurse
	 * until the interrupt stack is gone.
	 */
	z_armv5_irq_enable r1
	blx	r3
	z_armv5_irq_disable r1
#else
	blx	r3
#endif

spurious_continue:
	ldmia	sp!, {r0, r1}
	bl	z_soc_irq_eoi

#ifdef CONFIG_TRACING_ISR
	bl	sys_trace_isr_exit
#endif

	/*
	 * Shared with the irq_offload path in exc.S. This has to stay part of
	 * _isr_wrapper's own section: the code above falls through into it.
	 */
z_armv5_irq_done:
	/* Decrement interrupt nesting count */
	get_cpu	r2
	ldr	r0, [r2, #___cpu_t_nested_OFFSET]
	sub	r0, r0, #1
	str	r0, [r2, #___cpu_t_nested_OFFSET]

	/* A nested interrupt never reschedules. */
	cmp	r0, #0
	bhi	__EXIT_INT

	/* Back onto the interrupted thread's stack. */
	ldmia	sp!, {r0, r1}
	mov	sp, r0

	ldr	r1, [r2, #___cpu_t_current_OFFSET]
	stmdb	sp!, {r1, r2}
	mov	r0, #0
	bl	z_get_next_switch_handle
	ldmia	sp!, {r1, r2}

	cmp	r0, #0
	beq	__EXIT_INT

	/*
	 * r0: incoming thread (switch handle)
	 * r1: outgoing thread
	 */
	bl	z_arm_context_switch

__EXIT_INT:
#ifdef CONFIG_STACK_SENTINEL
	bl	z_check_stack_sentinel
#endif
	b	z_armv5_exit_exc
