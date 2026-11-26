/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 * Copyright (c) 2020 Stephanos Ioannidis <root@stephanos.io>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARM Cortex-A and Cortex-R wrapper for ISRs with parameter
 *
 * Wrapper installed in vector table for handling dynamic interrupts that accept
 * a parameter.
 */
/*
 * Tell armclang that stack alignment are ensured.
 */
.eabi_attribute Tag_ABI_align_preserved, 1

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <offsets_short.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/sw_isr_table.h>
#include "macro_priv.inc"


_ASM_FILE_PROLOGUE

GDATA(_sw_isr_table)

GTEXT(_isr_wrapper)

/**
 *
 * @brief Wrapper around ISRs when inserted in software ISR table
 *
 * When inserted in the vector table, _isr_wrapper() demuxes the ISR table
 * using the running interrupt number as the index, and invokes the registered
 * ISR with its corresponding argument. When returning from the ISR, it
 * determines if a context switch needs to happen and calls
 * z_arm_context_switch() if so.
 *
 */
SECTION_FUNC(TEXT, _isr_wrapper)
	sub lr, #4
	z_arm_cortex_ar_enter_exc

	/* Increment interrupt nesting count, r2 is the CPU pointer */
	ldr r0, [r2, #___cpu_t_nested_OFFSET]
	add r0, #1
	str r0, [r2, #___cpu_t_nested_OFFSET]

	irq_stack_enter r0

#ifdef CONFIG_TRACING_ISR
	bl sys_trace_isr_enter
#endif /* CONFIG_TRACING_ISR */

#ifdef CONFIG_PM
	/*
	 * All interrupts are disabled when handling idle wakeup.  For tickless
	 * idle, this ensures that the calculation and programming of the
	 * device for the next timer deadline is not interrupted.  For
	 * non-tickless idle, this ensures that the clearing of the kernel idle
	 * state is not interrupted.  In each case, pm_system_resume
	 * is called with interrupts disabled.
	 */

	/* is this a wakeup from idle ? */
	ldr r2, =_kernel
	/* requested idle duration, in ticks */
	ldr r0, [r2, #_kernel_offset_to_idle]
	cmp r0, #0

	beq _idle_state_cleared
	movs r1, #0
	/* clear kernel idle state */
	str r1, [r2, #_kernel_offset_to_idle]
	bl pm_system_resume
_idle_state_cleared:
#endif /* CONFIG_PM */

	/* Get active IRQ number from the interrupt controller */
#if !defined(CONFIG_ARM_CUSTOM_INTERRUPT_CONTROLLER)
	bl arm_gic_get_active
#else
	bl z_soc_irq_get_active
#endif /* !CONFIG_ARM_CUSTOM_INTERRUPT_CONTROLLER */

	push {r0, r1}

#if defined(CONFIG_GIC_V1) || defined(CONFIG_GIC_V2)
	/* Mask out GICC_IAR.CPUID [12:10] */
	bic r0, r0, #0x1c00
#endif

#ifdef CONFIG_ARM_TRACK_ACTIVE_IRQ
	/*
	 * Publish the acknowledged INTID for arch_irq_get_active(). The value
	 * is stored biased by one so that the zero-initialized boot state
	 * reads as "no interrupt active" on every CPU. The previous value
	 * and the CPU pointer are kept on the stack so the exit path
	 * unwinds a nested interrupt to the preempted INTID and the
	 * outermost exit back to "none".
	 */
	get_cpu r2
	ldr r1, [r2, #_cpu_offset_to_active_irq]
	add r3, r0, #1
	str r3, [r2, #_cpu_offset_to_active_irq]
	push {r1, r2}
#endif /* CONFIG_ARM_TRACK_ACTIVE_IRQ */

#if CONFIG_GIC_VER >= 3
	/*
	 * Ignore Special INTIDs 1020..1023 see 2.2.1 of Arm Generic Interrupt Controller
	 * Architecture Specification GIC architecture version 3 and version 4
	 */
	mov r1, #1019
	cmp r0, r1
	ble oob
	mov r1, #1023
	cmp r0, r1
	bgt oob
	b spurious_continue

oob:
#endif

	/*
	 * Skip calling the isr if it is a spurious interrupt.
	 */
	mov r1, #CONFIG_NUM_IRQS
	cmp r0, r1
	bge spurious_continue

	lsl r0, r0, #3	/* table is 8-byte wide */
	ldr r1, =_sw_isr_table
	add r1, r1, r0	/* table entry: ISRs must have their MSB set to stay
			 * in thumb mode */
	ldm r1!,{r0,r3}	/* arg in r0, ISR in r3 */

	/*
	 * Enable and disable interrupts again to allow nested in exception handlers.
	 */
	cpsie i
	blx r3		/* call ISR */
	cpsid i

spurious_continue:
#ifdef CONFIG_ARM_TRACK_ACTIVE_IRQ
	/* Unwind the published active INTID to the preempted context's */
	pop {r1, r2}
	str r1, [r2, #_cpu_offset_to_active_irq]
#endif /* CONFIG_ARM_TRACK_ACTIVE_IRQ */

	/* Signal end-of-interrupt */
	pop {r0, r1}
#if !defined(CONFIG_ARM_CUSTOM_INTERRUPT_CONTROLLER)
	bl arm_gic_eoi
#else
	bl z_soc_irq_eoi
#endif /* !CONFIG_ARM_CUSTOM_INTERRUPT_CONTROLLER */

#ifdef CONFIG_TRACING_ISR
	bl sys_trace_isr_exit
#endif

GTEXT(z_arm_cortex_ar_irq_done)
z_arm_cortex_ar_irq_done:
	/* Decrement interrupt nesting count */
	get_cpu r2
	ldr r0, [r2, #___cpu_t_nested_OFFSET]
	subs r0, r0, #1
	str r0, [r2, #___cpu_t_nested_OFFSET]
	/* Do not context switch if exiting a nested interrupt */
	bne __EXIT_INT

	/* Leave the IRQ stack, see irq_stack_enter */
	ldr sp, [sp]

	ldr r1, [r2, #___cpu_t_current_OFFSET]
#if !defined(CONFIG_SMP) && !defined(CONFIG_SCHED_THREAD_USAGE)
	/*
	 * The ready queue cache names the current thread when no switch is
	 * needed. z_get_next_switch_handle() would then return NULL. Its
	 * other work is the stack sentinel check done below, and clearing
	 * the switch handle of the current thread, which is written again
	 * when that thread is switched out. r2 is &_kernel.cpus[0], which is
	 * &_kernel.
	 */
	ldr r0, [r2, #_kernel_offset_to_ready_q_cache]
	cmp r0, r1
	beq __EXIT_INT
#endif
	push {r1, r2}
	mov r0, #0
	bl z_get_next_switch_handle

	pop {r1, r2}
	cmp r0, #0
	beq __EXIT_INT

	/*
	 * Switch thread
	 * r0: new thread
	 * r1: old thread
	 */
	bl z_arm_context_switch

#ifndef CONFIG_STACK_SENTINEL
	b z_arm_cortex_ar_exit_exc
#endif

__EXIT_INT:

#ifdef CONFIG_STACK_SENTINEL
	bl z_check_stack_sentinel
	b z_arm_cortex_ar_exit_exc
#else
	/* r2 is still the CPU pointer */
	b z_arm_cortex_ar_exit_exc_cpu
#endif
