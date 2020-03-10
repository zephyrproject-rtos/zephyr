/*
 * Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARM64 Cortex-A ISRs wrapper
 */

#include <toolchain.h>
#include <linker/sections.h>
#include <offsets_short.h>
#include <arch/cpu.h>
#include <sw_isr_table.h>

_ASM_FILE_PROLOGUE

GDATA(_sw_isr_table)

/**
 * @brief Wrapper around ISRs when inserted in software ISR table
 *
 * When inserted in the vector table, _isr_wrapper() demuxes the ISR table
 * using the running interrupt number as the index, and invokes the registered
 * ISR with its corresponding argument. When returning from the ISR, it
 * determines if a context switch needs to happen.
 *
 * @return N/A
 */

GTEXT(_isr_wrapper)
SECTION_FUNC(TEXT, _isr_wrapper)
	/*
	 * Save x0-x18 (and x30) on the process stack because they can be
	 * clobbered by the ISR and/or context switch.
	 *
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
	stp	x16, x17, [sp, #-16]!
	stp	x18, x30, [sp, #-16]!

	/* ++(_kernel->nested) to be checked by arch_is_in_isr() */
	ldr	x1, =_kernel
	ldr	x2, [x1, #_kernel_offset_to_nested]
	add	x2, x2, #1
	str	x2, [x1, #_kernel_offset_to_nested]

#ifdef CONFIG_TRACING
	bl	sys_trace_isr_enter
#endif

	/* Cortex-A has one IRQ line so the main handler will be at offset 0 */
	ldr	x1, =_sw_isr_table
	ldp	x0, x3, [x1] /* arg in x0, ISR in x3 */
	blr	x3

#ifdef CONFIG_TRACING
	bl	sys_trace_isr_exit
#endif

	/* --(_kernel->nested) */
	ldr	x1, =_kernel
	ldr	x2, [x1, #_kernel_offset_to_nested]
	sub	x2, x2, #1
	str	x2, [x1, #_kernel_offset_to_nested]

	/* Check if we need to context switch */
	ldr	x2, [x1, #_kernel_offset_to_current]
	ldr	x3, [x1, #_kernel_offset_to_ready_q_cache]
	cmp	x2, x3
	beq	exit

	/* Switch thread */
	bl	z_arm64_pendsv

	/* We return here in two cases:
	 *
	 * - The ISR was taken and no context switch was performed.
	 * - A context-switch was performed during the ISR in the past and now
	 *   the thread has been switched in again and we return here from the
	 *   ret in z_arm64_pendsv() because x30 was saved and restored.
	 */
exit:
#ifdef CONFIG_STACK_SENTINEL
	bl	z_check_stack_sentinel
#endif
	b	z_arm64_exit_exc

