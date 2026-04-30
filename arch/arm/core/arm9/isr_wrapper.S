/*
 * Copyright (c) 2025-2026, Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

.eabi_attribute Tag_ABI_align_preserved, 1

#include <offsets_short.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/linker/sections.h>
#include <zephyr/sw_isr_table.h>
#include <zephyr/toolchain.h>

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
 * determines if a context switch needs to happen and invoke the arch_switch
 * function if so.
 *
 */
SECTION_FUNC(TEXT, _isr_wrapper)
	sub lr, #4

	/*
	 * Store r0-r3, r12, lr into the stack to construct an exception
	 * stack frame.
	 */
	stm sp, {r0-r3}
	mov r1, lr
	mrs r2, spsr
	mov r3, sp
	z_arm_arm9_mode r0, #MODE_SYS
	push {r1, r2}
	ldm r3, {r0-r3}
	push {r0-r3, r12, lr}

	/* increment exception depth */
	ldr r2, =_kernel
	ldrb r1, [r2, #_cpu_offset_to_exc_depth]
	add r1, r1, #1
	strb r1, [r2, #_cpu_offset_to_exc_depth]

	/* Increment interrupt nesting count */
	ldr r0, [r2, #___cpu_t_nested_OFFSET]
	add r0, r0, #1
	str r0, [r2, #___cpu_t_nested_OFFSET]

	/* If not nested: switch to IRQ stack and save current sp on it. */
	cmp r0, #1
	bhi 1f

	mov r0, sp
	stm sp, {r1}
	z_arm_arm9_mode r1, #MODE_IRQ
	push {r0}
	z_arm_arm9_mode r0, #MODE_SYS
	ldm sp, {r1}
	z_arm_arm9_mode r0, #MODE_IRQ
	ldm sp, {r0}

1:
#ifdef CONFIG_TRACING_ISR
	bl sys_trace_isr_enter
#endif /* CONFIG_TRACING_ISR */

	/* Get active IRQ number from the interrupt controller */
	bl z_soc_irq_get_active

	push {r0, r1}
	lsl r0, r0, #3 /* table is 8-byte wide */

	/* Skip calling the isr if it is a spurious interrupt. */
	mov r1, #CONFIG_NUM_IRQS
	lsl r1, r1, #3
	cmp r0, r1
	bge spurious_continue

	ldr r1, =_sw_isr_table
	add r1, r1, r0 /* table entry: ISRs must have their MSB set to stay in thumb mode */
	ldm r1!, {r0, r3} /* arg in r0, ISR in r3 */

	/* Enable and disable interrupts again to allow nested in exception handlers. */
	z_arm_arm9_cpsr_clear I_BIT
	blx r3 /* call ISR */
	z_arm_arm9_cpsr_set I_BIT

spurious_continue:
	/* Signal end-of-interrupt */
	pop {r0, r1}
	bl z_soc_irq_eoi

#ifdef CONFIG_TRACING_ISR
	bl sys_trace_isr_exit
#endif

GTEXT(z_arm_arm9_irq_done)
z_arm_arm9_irq_done:
	/* Decrement interrupt nesting count */
	ldr r2, =_kernel
	ldr r0, [r2, #___cpu_t_nested_OFFSET]
	sub r0, r0, #1
	str r0, [r2, #___cpu_t_nested_OFFSET]
	/* Do not context switch if exiting a nested interrupt */
	cmp r0, #0
	bhi __EXIT_INT

	/* retrieve pointer to the current thread */
	pop {r0}
	z_arm_arm9_mode r1, #MODE_SYS

	mov sp, r0

	ldr r1, [r2, #___cpu_t_current_OFFSET]
	push {r1}
	mov r0, #0
	bl z_get_next_switch_handle

	pop {r1}
	cmp r0, #0
	beq __EXIT_INT

	/* void z_arm_context_switch(struct k_thread *new, struct k_thread *old); */
	bl z_arm_context_switch

__EXIT_INT:

#ifdef CONFIG_STACK_SENTINEL
	bl z_check_stack_sentinel
#endif /* CONFIG_STACK_SENTINEL */

	b z_arm_arm9_exit_exc
