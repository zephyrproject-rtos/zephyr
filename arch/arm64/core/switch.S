/*
 * Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Thread context switching for ARM64 Cortex-A (AArch64)
 *
 * This module implements the routines necessary for thread context switching
 * on ARM64 Cortex-A (AArch64)
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <offsets_short.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/syscall.h>
#include "macro_priv.inc"

_ASM_FILE_PROLOGUE

/*
 * Routine to handle context switches
 *
 * This function is directly called either by _isr_wrapper() in case of
 * preemption, or arch_switch() in case of cooperative switching.
 *
 * void z_arm64_context_switch(struct k_thread *new, struct k_thread *old);
 */

GTEXT(z_arm64_context_switch)
SECTION_FUNC(TEXT, z_arm64_context_switch)

#ifndef CONFIG_ARM64_SAFE_EXCEPTION_STACK
	/* Save the current SP_EL0 */
	mrs	x4, sp_el0
#endif

	stp	x19, x20, [x1, #_thread_offset_to_callee_saved_x19_x20]
	stp	x21, x22, [x1, #_thread_offset_to_callee_saved_x21_x22]
	stp	x23, x24, [x1, #_thread_offset_to_callee_saved_x23_x24]
	stp	x25, x26, [x1, #_thread_offset_to_callee_saved_x25_x26]
	stp	x27, x28, [x1, #_thread_offset_to_callee_saved_x27_x28]
#ifndef CONFIG_ARM64_SAFE_EXCEPTION_STACK
	stp	x29, x4,  [x1, #_thread_offset_to_callee_saved_x29_sp_el0]
#else
	str	x29,      [x1, #_thread_offset_to_callee_saved_x29_sp_el0]
#endif

	/* Save the current SP_ELx and return address */
	mov	x4, sp
	stp	x4, lr, [x1, #_thread_offset_to_callee_saved_sp_elx_lr]

	/* save current thread's exception depth */
	mrs	x4, tpidrro_el0
	lsr	x2, x4, #TPIDRROEL0_EXC_SHIFT
	strb	w2, [x1, #_thread_offset_to_exception_depth]

	/* retrieve next thread's exception depth */
	ldrb	w2, [x0, #_thread_offset_to_exception_depth]
	bic	x4, x4, #TPIDRROEL0_EXC_DEPTH
	orr	x4, x4, x2, lsl #TPIDRROEL0_EXC_SHIFT
	msr	tpidrro_el0, x4

#ifdef CONFIG_FPU_SHARING
	/*
	 * Do this after tpidrro_el0 is updated with the new exception
	 * depth value, and before old->switch_handle is updated (making
	 * it available for grab by another CPU) as we still use its stack.
	 */
	stp	x0, x1, [sp, #-16]!
	bl	z_arm64_fpu_thread_context_switch
	ldp	x0, x1, [sp], #16
#endif

	/* save old thread into switch handle which is required by
	 * wait_for_switch
	 */
	 str	x1, [x1, #___thread_t_switch_handle_OFFSET]

#ifdef CONFIG_THREAD_LOCAL_STORAGE
	/* Grab the TLS pointer */
	ldr	x2, [x0, #_thread_offset_to_tls]

	/* Store in the "Thread ID" register.
	 * This register is used as a base pointer to all
	 * thread variables with offsets added by toolchain.
	 */
	msr	tpidr_el0, x2
#endif

	ldp	x19, x20, [x0, #_thread_offset_to_callee_saved_x19_x20]
	ldp	x21, x22, [x0, #_thread_offset_to_callee_saved_x21_x22]
	ldp	x23, x24, [x0, #_thread_offset_to_callee_saved_x23_x24]
	ldp	x25, x26, [x0, #_thread_offset_to_callee_saved_x25_x26]
	ldp	x27, x28, [x0, #_thread_offset_to_callee_saved_x27_x28]
#ifndef CONFIG_ARM64_SAFE_EXCEPTION_STACK
	ldp	x29, x4,  [x0, #_thread_offset_to_callee_saved_x29_sp_el0]

	/* Restore SP_EL0 */
	msr	sp_el0, x4
#else
	ldr	x29,      [x0, #_thread_offset_to_callee_saved_x29_sp_el0]
#endif

	/* Restore SP_EL1 and return address */
	ldp	x4, lr, [x0, #_thread_offset_to_callee_saved_sp_elx_lr]
	mov	sp, x4

#if defined(CONFIG_ARM64_SAFE_EXCEPTION_STACK)
	/* arch_curr_cpu()->arch.current_stack_limit = thread->arch.stack_limit */
	get_cpu	x4
	ldr	x2, [x0, #_thread_offset_to_stack_limit]
	str	x2, [x4, #_cpu_offset_to_current_stack_limit]
#endif

#ifdef CONFIG_USERSPACE
	str     lr, [sp, #-16]!
	bl      z_arm64_swap_mem_domains
	ldr     lr, [sp], #16
#endif

#ifdef CONFIG_INSTRUMENT_THREAD_SWITCHING
	str	lr, [sp, #-16]!
	bl	z_thread_mark_switched_in
	ldr	lr, [sp], #16
#endif

	/* Return to arch_switch() or _isr_wrapper() */
	ret

/*
 * Synchronous exceptions handler
 *
 * The service call (SVC) is used in the following occasions:
 * - Cooperative context switching
 * - IRQ offloading
 */

GTEXT(z_arm64_sync_exc)
SECTION_FUNC(TEXT, z_arm64_sync_exc)

	mrs	x0, esr_el1
	lsr	x1, x0, #26

#ifdef CONFIG_FPU_SHARING
	cmp	x1, #0x07 /*Access to SIMD or floating-point */
	bne	1f
	mov	x0, sp
	bl	z_arm64_fpu_trap
	b	z_arm64_exit_exc_fpu_done
1:
#endif

	cmp	x1, #0x15 /* 0x15 = SVC */
	bne	inv

	/* Demux the SVC call */
	and	x1, x0, #0xff

	cmp	x1, #_SVC_CALL_RUNTIME_EXCEPT
	beq	oops

#ifdef CONFIG_USERSPACE
	cmp	x1, #_SVC_CALL_SYSTEM_CALL
	beq	z_arm64_do_syscall
#endif

#ifdef CONFIG_IRQ_OFFLOAD
	cmp	x1, #_SVC_CALL_IRQ_OFFLOAD
	beq	offload
	b	inv
offload:
	/*
	 * Retrieve provided routine and argument from the stack.
	 * Routine pointer is in saved x0, argument in saved x1
	 * so we load them with x1/x0 (reversed).
	 */
	ldp	x1, x0, [sp, ___esf_t_x0_x1_OFFSET]

	/* ++_current_cpu->nested to be checked by arch_is_in_isr() */
	get_cpu	x2
	ldr	w3, [x2, #___cpu_t_nested_OFFSET]
	add	w4, w3, #1
	str	w4, [x2, #___cpu_t_nested_OFFSET]

	/* If not nested: switch to IRQ stack and save current sp on it. */
	cbnz	w3, 1f
	ldr	x3, [x2, #___cpu_t_irq_stack_OFFSET]
	mov	x4, sp
	mov	sp, x3
	str	x4, [sp, #-16]!
#if defined(CONFIG_ARM64_SAFE_EXCEPTION_STACK)
	/* update the stack limit with IRQ stack limit */
	sub	x3, x3, #CONFIG_ISR_STACK_SIZE
	str	x3, [x2, #_cpu_offset_to_current_stack_limit]
#endif
1:
	/* Execute provided routine (argument is in x0 already). */
	blr	x1

	/* Exit through regular IRQ exit path */
	b	z_arm64_irq_done
#endif
	b	inv

oops:
	mov	x0, sp
	b	z_arm64_do_kernel_oops

inv:
	mov	x0, #0 /* K_ERR_CPU_EXCEPTION */
	mov	x1, sp
	bl	z_arm64_fatal_error

	/* Return here only in case of recoverable error */
	b	z_arm64_exit_exc
