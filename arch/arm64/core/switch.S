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

#include <toolchain.h>
#include <linker/sections.h>
#include <offsets_short.h>
#include <arch/cpu.h>
#include <syscall.h>
#include "macro_priv.inc"

_ASM_FILE_PROLOGUE

/*
 * Routine to handle context switches
 *
 * This function is directly called either by _isr_wrapper() in case of
 * preemption, or z_arm64_sync_exc() in case of cooperative switching.
 */

GTEXT(z_arm64_context_switch)
SECTION_FUNC(TEXT, z_arm64_context_switch)

	ldr	x3, =_thread_offset_to_callee_saved

	/* addr of callee-saved regs in thread in x2 */
	add	x2, x1, x3

	/* Save the current SP_EL0 */
	mrs	x4, sp_el0

	stp	x19, x20, [x2, ___callee_saved_t_x19_x20_OFFSET]
	stp	x21, x22, [x2, ___callee_saved_t_x21_x22_OFFSET]
	stp	x23, x24, [x2, ___callee_saved_t_x23_x24_OFFSET]
	stp	x25, x26, [x2, ___callee_saved_t_x25_x26_OFFSET]
	stp	x27, x28, [x2, ___callee_saved_t_x27_x28_OFFSET]
	stp	x29, x4,  [x2, ___callee_saved_t_x29_sp_el0_OFFSET]

	/* Save the current SP_ELx */
	mov	x4, sp
	str	x4, [x2, ___callee_saved_t_sp_elx_OFFSET]

	/* save current thread's exception depth */
	mrs	x4, tpidrro_el0
	lsr	x2, x4, #TPIDRROEL0_EXC_SHIFT
	strb	w2, [x1, #_thread_offset_to_exception_depth]

	/* retrieve next thread's exception depth */
	ldrb	w2, [x0, #_thread_offset_to_exception_depth]
	bic	x4, x4, #TPIDRROEL0_EXC_DEPTH
	orr	x4, x4, x2, lsl #TPIDRROEL0_EXC_SHIFT
	msr	tpidrro_el0, x4

#ifdef CONFIG_SMP
	/* save old thread into switch handle which is required by
	 * wait_for_switch
	 */
	 str	x1, [x1, #___thread_t_switch_handle_OFFSET]
#endif

#ifdef CONFIG_THREAD_LOCAL_STORAGE
	/* Grab the TLS pointer */
	ldr	x2, =_thread_offset_to_tls
	ldr	x2, [x0, x2]

	/* Store in the "Thread ID" register.
	 * This register is used as a base pointer to all
	 * thread variables with offsets added by toolchain.
	 */
	msr	tpidr_el0, x2
#endif

	/* addr of callee-saved regs in thread in x2 */
	add	x2, x0, x3

	ldp	x19, x20, [x2, ___callee_saved_t_x19_x20_OFFSET]
	ldp	x21, x22, [x2, ___callee_saved_t_x21_x22_OFFSET]
	ldp	x23, x24, [x2, ___callee_saved_t_x23_x24_OFFSET]
	ldp	x25, x26, [x2, ___callee_saved_t_x25_x26_OFFSET]
	ldp	x27, x28, [x2, ___callee_saved_t_x27_x28_OFFSET]
	ldp	x29, x1,  [x2, ___callee_saved_t_x29_sp_el0_OFFSET]

	/* Restore SP_EL0 */
	msr	sp_el0, x1

	/* Restore SP_EL1 */
	ldr	x1, [x2, ___callee_saved_t_sp_elx_OFFSET]
	mov	sp, x1

#ifdef CONFIG_USERSPACE
	stp     xzr, x30, [sp, #-16]!
	bl      z_arm64_swap_ptables
	ldp     xzr, x30, [sp], #16
#endif

#ifdef CONFIG_INSTRUMENT_THREAD_SWITCHING
	stp	xzr, x30, [sp, #-16]!
	bl	z_thread_mark_switched_in
	ldp	xzr, x30, [sp], #16
#endif

	/* Return to z_arm64_sync_exc() or _isr_wrapper() */
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

	cmp	x1, #_SVC_CALL_CONTEXT_SWITCH
	beq	context_switch

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
	/* ++(_kernel->nested) to be checked by arch_is_in_isr() */
	inc_nest_counter x0, x1

	bl	z_irq_do_offload

	/* --(_kernel->nested) */
	dec_nest_counter x0, x1
	b	z_arm64_exit_exc
#endif
	b	inv

oops:
	mov	x0, sp
	b	z_arm64_do_kernel_oops

context_switch:
	/*
	 * Retrieve x0 and x1 from the stack:
	 *
	 *  - x0 = new_thread->switch_handle = switch_to thread
	 *  - x1 = &old_thread->switch_handle = current thread
	 */
	ldp	x0, x1, [sp, ___esf_t_x0_x1_OFFSET]

	/* Get old thread from x1 */
	sub	x1, x1, ___thread_t_switch_handle_OFFSET

	/* Switch thread */
	bl	z_arm64_context_switch
	b	z_arm64_exit_exc

inv:
	mov	x0, #0 /* K_ERR_CPU_EXCEPTION */
	mov	x1, sp
	bl	z_arm64_fatal_error

	/* Return here only in case of recoverable error */
	b	z_arm64_exit_exc

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

