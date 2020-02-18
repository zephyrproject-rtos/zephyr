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
#include "macro.inc"

_ASM_FILE_PROLOGUE

GDATA(_kernel)
GDATA(_k_neg_eagain)

/**
 * @brief Routine to handle context switches
 *
 * This function is directly called either by _isr_wrapper() in case of
 * preemption, or z_arm64_svc() in case of cooperative switching.
 */

GTEXT(z_arm64_context_switch)
SECTION_FUNC(TEXT, z_arm64_context_switch)
#ifdef CONFIG_TRACING
	stp	xzr, x30, [sp, #-16]!
	bl	sys_trace_thread_switched_in
	ldp	xzr, x30, [sp], #16
#endif
	/* load _kernel into x1 and current k_thread into x2 */
	ldr	x1, =_kernel
	ldr	x2, [x1, #_kernel_offset_to_current]

	/* addr of callee-saved regs in thread in x0 */
	ldr	x0, =_thread_offset_to_callee_saved
	add	x0, x0, x2

	/* Store rest of process context including x30, SPSR_ELn and ELR_ELn */
	stp	x19, x20, [x0], #16
	stp	x21, x22, [x0], #16
	stp	x23, x24, [x0], #16
	stp	x25, x26, [x0], #16
	stp	x27, x28, [x0], #16
	stp	x29, x30, [x0], #16

	switch_el x3, 3f, 2f, 1f
3:
	mrs	x4, spsr_el3
	mrs	x5, elr_el3
	b	0f
2:
	mrs	x4, spsr_el2
	mrs	x5, elr_el2
	b	0f
1:
	mrs	x4, spsr_el1
	mrs	x5, elr_el1
0:
	stp	x4, x5, [x0], #16

	/* Save the current SP */
	mov	x6, sp
	str	x6, [x0]

	/* fetch the thread to run from the ready queue cache */
	ldr	x2, [x1, #_kernel_offset_to_ready_q_cache]
	str	x2, [x1, #_kernel_offset_to_current]

	/* addr of callee-saved regs in thread in x0 */
	ldr	x0, =_thread_offset_to_callee_saved
	add	x0, x0, x2

	/* Restore x19-x29 plus x30, SPSR_ELn and ELR_ELn */
	ldp	x19, x20, [x0], #16
	ldp	x21, x22, [x0], #16
	ldp	x23, x24, [x0], #16
	ldp	x25, x26, [x0], #16
	ldp	x27, x28, [x0], #16
	ldp	x29, x30, [x0], #16

	ldp	x4, x5, [x0], #16

	switch_el x3, 3f, 2f, 1f
3:
	msr	spsr_el3, x4
	msr	elr_el3, x5
	b	0f
2:
	msr	spsr_el2, x4
	msr	elr_el2, x5
	b	0f
1:
	msr	spsr_el1, x4
	msr	elr_el1, x5
0:
	ldr	x6, [x0]
	mov	sp, x6

#ifdef CONFIG_TRACING
	stp	xzr, x30, [sp, #-16]!
	bl	sys_trace_thread_switched_out
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
	z_arm64_enter_exc

	switch_el x3, 3f, 2f, 1f
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
	and	x2, x0, #0xff
	cmp	x2, #_SVC_CALL_CONTEXT_SWITCH
	beq	context_switch

#ifdef CONFIG_IRQ_OFFLOAD
	cmp	x2, #_SVC_CALL_IRQ_OFFLOAD
	beq	offload
	b	inv
offload:
	/* ++(_kernel->nested) to be checked by arch_is_in_isr() */
	ldr	x1, =_kernel
	ldr	x2, [x1, #_kernel_offset_to_nested]
	add	x2, x2, #1
	str	x2, [x1, #_kernel_offset_to_nested]

	bl	z_irq_do_offload

	/* --(_kernel->nested) */
	ldr	x1, =_kernel
	ldr	x2, [x1, #_kernel_offset_to_nested]
	sub	x2, x2, #1
	str	x2, [x1, #_kernel_offset_to_nested]
	b	exit
#endif
	b	inv

context_switch:
	bl	z_arm64_context_switch

exit:
	z_arm64_exit_exc

inv:
	mov	x1, sp
	mov	x0, #0 /* K_ERR_CPU_EXCEPTION */
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

