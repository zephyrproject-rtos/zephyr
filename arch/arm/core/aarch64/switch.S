/*
 * Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Thread context switching for ARM64 Cortex-A
 *
 * This module implements the routines necessary for thread context switching
 * on ARM64 Cortex-A.
 */

#include <toolchain.h>
#include <linker/sections.h>
#include <offsets_short.h>
#include <arch/cpu.h>
#include <syscall.h>
#include "macro_priv.inc"

_ASM_FILE_PROLOGUE

GDATA(_kernel)

/**
 * @brief Routine to handle context switches
 *
 * This function is directly called either by _isr_wrapper() in case of
 * preemption, or z_arm64_svc() in case of cooperative switching.
 */

GTEXT(z_arm64_context_switch)
SECTION_FUNC(TEXT, z_arm64_context_switch)
	/* addr of callee-saved regs in thread in x2 */
	ldr	x2, =_thread_offset_to_callee_saved
	add	x2, x2, x1

	/* Store rest of process context including x30 */
	stp	x19, x20, [x2], #16
	stp	x21, x22, [x2], #16
	stp	x23, x24, [x2], #16
	stp	x25, x26, [x2], #16
	stp	x27, x28, [x2], #16
	stp	x29, x30, [x2], #16

	/* Save the current SP */
	mov	x1, sp
	str	x1, [x2]

	/* addr of callee-saved regs in thread in x2 */
	ldr	x2, =_thread_offset_to_callee_saved
	add	x2, x2, x0

	/* Restore x19-x29 plus x30 */
	ldp	x19, x20, [x2], #16
	ldp	x21, x22, [x2], #16
	ldp	x23, x24, [x2], #16
	ldp	x25, x26, [x2], #16
	ldp	x27, x28, [x2], #16
	ldp	x29, x30, [x2], #16

	ldr	x1, [x2]
	mov	sp, x1

#ifdef CONFIG_TRACING
	stp	xzr, x30, [sp, #-16]!
	bl	sys_trace_thread_switched_in
	ldp	xzr, x30, [sp], #16
#endif

	/* We restored x30 from the process stack. There are three possible
	 * cases:
	 *
	 * - We return to z_arm64_svc() when swapping in a thread that was
	 *   swapped out by z_arm64_svc() before jumping into
	 *   z_arm64_exit_exc()
	 * - We return to _isr_wrapper() when swapping in a thread that was
	 *   swapped out by _isr_wrapper() before jumping into
	 *   z_arm64_exit_exc()
	 * - We return (jump) into z_thread_entry_wrapper() for new threads
	 *   (see thread.c)
	 */
	ret

/**
 *
 * @brief Entry wrapper for new threads
 *
 * @return N/A
 */

GTEXT(z_thread_entry_wrapper)
SECTION_FUNC(TEXT, z_thread_entry_wrapper)
	/*
	 * Restore SPSR_ELn and ELR_ELn saved in the temporary stack by
	 * arch_new_thread()
	 */
	ldp	x0, x1, [sp], #16
	switch_el x3, 3f, 2f, 1f
3:
	msr	spsr_el3, x0
	msr	elr_el3, x1
	b	0f
2:
	msr	spsr_el2, x0
	msr	elr_el2, x1
	b	0f
1:
	msr	spsr_el1, x0
	msr	elr_el1, x1
0:
	/*
	 * z_thread_entry_wrapper is called for every new thread upon the return
	 * of arch_swap() or ISR. Its address, as well as its input function
	 * arguments thread_entry_t, void *, void *, void * are restored from
	 * the thread stack (see thread.c).
	 * In this case, thread_entry_t, * void *, void * and void * are stored
	 * in registers x0, x1, x2 and x3. These registers are used as arguments
	 * to function z_thread_entry.
	 */
	ldp	x0, x1, [sp], #16
	ldp	x2, x3, [sp], #16

	/* ELR_ELn was set in thread.c to z_thread_entry() */
	eret

/**
 *
 * @brief Service call handler
 *
 * The service call (SVC) is used in the following occasions:
 * - Cooperative context switching
 * - IRQ offloading
 *
 * @return N/A
 */

GTEXT(z_arm64_svc)
SECTION_FUNC(TEXT, z_arm64_svc)
	z_arm64_enter_exc x2, x3, x4

	switch_el x1, 3f, 2f, 1f
3:
	mrs	x0, esr_el3
	b	0f
2:
	mrs	x0, esr_el2
	b	0f
1:
	mrs	x0, esr_el1
0:
	lsr	x1, x0, #26

	cmp	x1, #0x15 /* 0x15 = SVC */
	bne	inv

	/* Demux the SVC call */
	and	x1, x0, #0xff
	cmp	x1, #_SVC_CALL_CONTEXT_SWITCH
	beq	context_switch

#ifdef CONFIG_IRQ_OFFLOAD
	cmp	x1, #_SVC_CALL_IRQ_OFFLOAD
	beq	offload
	b	inv
offload:
	/* ++(_kernel->nested) to be checked by arch_is_in_isr() */
	ldr	x0, =_kernel
	ldr	x1, [x0, #_kernel_offset_to_nested]
	add	x1, x1, #1
	str	x1, [x0, #_kernel_offset_to_nested]

	bl	z_irq_do_offload

	/* --(_kernel->nested) */
	ldr	x0, =_kernel
	ldr	x1, [x0, #_kernel_offset_to_nested]
	sub	x1, x1, #1
	str	x1, [x0, #_kernel_offset_to_nested]
	b	exit
#endif
	b	inv

context_switch:
	/*
	 * Retrieve x0 and x1 from the stack:
	 *  - x0 = new_thread->switch_handle = switch_to thread
	 *  - x1 = x1 = &old_thread->switch_handle = current thread
	 */
	ldp	x0, x1, [sp, #(16 * 10)]

	/* Get old thread from x1 */
	sub	x1, x1, ___thread_t_switch_handle_OFFSET

	/* Switch thread */
	bl	z_arm64_context_switch

exit:
	z_arm64_exit_exc x0, x1, x2

inv:
	mov	x0, #0 /* K_ERR_CPU_EXCEPTION */
	mov	x1, sp
	b	z_arm64_fatal_error

GTEXT(z_arm64_call_svc)
SECTION_FUNC(TEXT, z_arm64_call_svc)
	svc	#_SVC_CALL_CONTEXT_SWITCH
	ret

#ifdef CONFIG_IRQ_OFFLOAD
GTEXT(z_arm64_offload)
SECTION_FUNC(TEXT, z_arm64_offload)
	svc	#_SVC_CALL_IRQ_OFFLOAD
	ret
#endif

