/*
 * Copyright (c) 2025 Arm Limited (or its affiliates). All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <offsets_short.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/syscall.h>
#include <zephyr/kernel.h>
_ASM_FILE_PROLOGUE

GTEXT(z_arm_pendsv)
GTEXT(z_arm_svc)
GTEXT(z_arm_context_switch)
GTEXT(z_do_kernel_oops)
GTEXT(z_arm_int_exit)
#if defined(CONFIG_IRQ_OFFLOAD)
GTEXT(z_irq_do_offload)
#endif

GDATA(_kernel)

#if defined(CONFIG_THREAD_LOCAL_STORAGE)
GDATA(z_arm_tls_ptr)
#endif

/**
 *
 * @brief PendSV exception handler, handling context switches
 *
 * The PendSV exception is the only execution context in the system that can
 * perform context switching. When an execution context finds out it has to
 * switch contexts, it pends the PendSV exception.
 *
 * When PendSV is pended, the decision that a context switch must happen has
 * already been taken. In other words, when z_arm_pendsv() runs, we *know* we
 * have to swap *something*.
 *
 * For Cortex-M, z_arm_pendsv() is invoked with no arguments.
 */
SECTION_FUNC(TEXT, z_arm_pendsv)
	mrs r0, PSP
	ldr r1, [ r0, #___basic_sf_t_pc_OFFSET ]
	ldr r2, =.L_switch_resume_from_isr_pre_isb
	cmp r1, r2
	beq .L_process_resume_from_isr
	/* TODO: check if we can't get away with a simple addition */
	ldr r2, =.L_switch_resume_from_isr_post_isb
	cmp r1, r2
	bne .L_pendsv_try_get_next_switch_handle

.L_process_resume_from_isr:
	/* if the PC pushed on the psp stack is either the MSR or the ISB from the context switch
	 * fn, then we context switch with r0 and r1 from the psp stack.
	 * Otherwise try z_get_next_switch_handle
	 */
	ldr r1, [ r0, #___basic_sf_t_r1_OFFSET ]
	ldr r0, [ r0, #___basic_sf_t_r0_OFFSET ]

	push { r0, lr }
	bl z_arm_context_switch
	pop { r0, lr }

	bx lr

.L_pendsv_try_get_next_switch_handle:
	ldr r1, =_kernel
	ldr r1, [ r1, #_kernel_offset_to_current ]

	push { r1, lr }
	mov r0, #0
	bl z_get_next_switch_handle
	pop { r1, lr }
	cbz r0, .L_pendsv_return
	push { r0, lr }
	bl z_arm_context_switch
	pop { r0, lr }
.L_pendsv_return:
	bx lr

SECTION_FUNC(TEXT, z_arm_context_switch)
	/*
	 * Assert we are running from either:
	 * - process mode (ipsr == 0)
	 * - pendsv (ipsr == 14)
	 *
	 * if thread was saved from PendSV && restored from process
	 *     Delegate to PendSV
	 *       Trigger PendSV
	 *       Unlock IRQ
	 *     We will return here after we switch back in
	 *       Relock IRQ
	 *       return
	 *
	 * Store in thread the ctx switch source.
	 * Save callee-saved registers
	 * Save PSP & LR
	 *
	 * Load local thread storage pointer for new thread
	 * Restore callee-saved registers & states
	 * Disable PSPLIM, set PSP, set PSPLIM to new thread
	 *
	 * Setup MPU for new thread
         *
	 * if thread was saved from process && restored from PendSV:
	 *     No caller-saved was pushed to the stack so we need to compensate for what the will HW
	 *     undo.
	 *       push words (r0-r3, ip, lr, pc, xpsr) to the psp stack.
	 *       r0-r3, ip = 0; lr = saved LR; PC to the end of this fn, xpsr = EPSR.T;
	 *
	 * return
	 */

	mrs r2, ipsr
	cbz r2, .L_switch_ipsr_ok
	cmp r2, #14
	beq .L_switch_do_context_switch
	/* invalid context switch environment */
	udf #1

.L_switch_ipsr_ok:
	/* r0: new thread, r1: old thread, r2: IPSR */
	ldr r3, =#_thread_offset_to_exception_depth
	ldr r3, [ r0, r3 ]
	cbz r3, .L_switch_do_context_switch

	/* Delegate context switching to PendSV */

	/* trigger pendSV */
	ldr r2, =#0x10000000
	ldr r3, =#0xE000ED04
	str r2, [ r3 ]
	dsb

	/* exit critical section to enter PendSV */
	mrs r3, BASEPRI
	mov r2, #0
	msr BASEPRI, r2
.L_switch_resume_from_isr_pre_isb:
	isb
.L_switch_resume_from_isr_post_isb:
	/* relock IRQ, return */
	msr BASEPRI, r3
	/* return to regular code path */
	bx lr

.L_switch_do_context_switch:
	/* r0: new thread, r1: old thread, r2: IPSR */
	ldr r3, =#_thread_offset_to_exception_depth
	str r2, [ r1, r3 ]

	add r3, r1, #_thread_offset_to_callee_saved

	mrs ip, PSP
	stmia r3, {r4-r11, ip, lr} /* when saving from PendSV, LR is irrelevant */

	/* the thread has been fully saved. trigger the sync */
	dsb
	str r1, [ r1, #___thread_t_switch_handle_OFFSET ]

#if defined(CONFIG_THREAD_LOCAL_STORAGE)
	/* load tls pointer */
	ldr r1, =z_arm_tls_ptr
	ldr r3, [ r0, #_thread_offset_to_tls ]
	str r3, [ r1 ]
#endif

	/* addr of callee-saved regs in thread in r0 */
	add r1, r0, #_thread_offset_to_callee_saved

	/* load callee-saved + psp from thread (with psp in the IP register ) */
	ldmia r1, {r4-r11, ip}

#if defined(CONFIG_BUILTIN_STACK_GUARD)
	/* temporarily disable Process mode's stack limit until we set the new value. */
	mov r1, #0
	msr PSPLIM, r1
#endif
	/* restore PSP */
	msr PSP, ip

#if defined(CONFIG_BUILTIN_STACK_GUARD)
	/* Enable stack limit. */
	ldr r3, [ r0, #_thread_offset_to_stack_info_start ]    /* stack_info.start */
	msr PSPLIM, r3
#endif /* CONFIG_BUILTIN_STACK_GUARD */

	ldr r1, [ r0, #_thread_offset_to_basepri ]
	mov r3, #0
	str r3, [ r0, #_thread_offset_to_basepri ]

#if defined(CONFIG_MPU_STACK_GUARD) || defined(CONFIG_USERSPACE)
	/* Re-program dynamic memory map */
	push { r0, r1, r2, lr }
	bl z_arm_configure_dynamic_mpu_regions
	pop { r0, r1, r2, lr }
#endif

	/* if running from Process Mode, return */
	cbz r2, .L_switch_return_from_process

	/* push the saved BASEPRI to the interrupt stack */
	push { r1 }

	/* or the thread was save from PendSV, return */
	ldr r3, =#_thread_offset_to_exception_depth
	ldr r3, [ r0, r3 ]
	cbnz r3, .L_switch_pop_base_pri

	/* mark the thread as "resumable from IRQ "*/
	mov r1, #14
	ldr r2, =#_thread_offset_to_exception_depth
	str r1, [ r0, r2 ]

	/* else push a pretend frame for the hardware handled restoration */
	mrs ip, PSP
	/* push IP, LR, PC, XPSR */
	sub ip, #16
	ldr r1, =#(_thread_offset_to_callee_saved + ___callee_saved_t_lr_OFFSET)
	ldr r1, [ r0, r1 ]
	mov r0, #0
	ldr r2, =#(.L_switch_return + 1)
	ldr r3, =#0x01000000
	stmia ip, { r0-r3 }

	/* push r0-r3 */
	sub ip, #16
	mov r1, #0
	mov r2, #0
	mov r3, #0
	stmia ip, { r0-r3 }
	msr PSP, ip

.L_switch_pop_base_pri:
	pop { r1 }
	b .L_switch_restore_basepri_then_return

.L_switch_return_from_process:
	/* r0: new thread, r1: saved BASEPRI, r2: IPSR */
	ldr r3, =#(_thread_offset_to_callee_saved + ___callee_saved_t_lr_OFFSET)
	ldr lr, [ r0, r3 ]
.L_switch_restore_basepri_then_return:
	msr BASEPRI, r1
.L_switch_return:
	bx lr

/**
 *
 * @brief Service call handler
 *
 * The service call (svc) is used in the following occasions:
 * - IRQ offloading
 * - Kernel run-time exceptions
 * - System Calls (User mode)
 *
 */
SECTION_FUNC(TEXT, z_arm_svc)
	tst lr, #_EXC_RETURN_SPSEL_Msk /* did we come from thread mode ? */
	ite eq  /* if zero (equal), came from handler mode */
		mrseq r0, MSP   /* handler mode, stack frame is on MSP */
		mrsne r0, PSP   /* thread mode, stack frame is on PSP */
	/* Figure out what SVC call number was invoked */

	ldr r1, [r0, #24]   /* grab address of PC from stack frame */
	ldrb r1, [r1, #-2]
	/* SVC is a two-byte instruction, point to it and read the
	 * SVC number (lower byte of SCV instruction)
	 */

	cmp r1, #2
	beq .L_oops

#if defined(CONFIG_IRQ_OFFLOAD)
	push {r0, lr}
	bl z_irq_do_offload  /* call C routine which executes the offload */
	pop {r0, lr}
#endif /* CONFIG_IRQ_OFFLOAD */

	/* exception return is done in z_arm_int_exit() */
	b z_arm_int_exit

.L_oops:
	push {r0, lr}
#if defined(CONFIG_EXTRA_EXCEPTION_INFO)
	/* Build _callee_saved_t. To match the struct
	 * definition we push the psp & then r11-r4
	 */
	mrs r1, PSP
	push {r1, r2}
	push {r4-r11}
	mov  r1, sp /* pointer to _callee_saved_t */
#endif /* CONFIG_EXTRA_EXCEPTION_INFO */
	mov r2, lr /* EXC_RETURN */
	bl z_do_kernel_oops
	/* return from SVC exception is done here */
#if defined(CONFIG_EXTRA_EXCEPTION_INFO)
	/* We do not need to restore any register state here
	 * because we did not use any callee-saved registers
	 * in this routine. Therefore, we can just reset
	 * the MSP to its value prior to entering the function
	 */
	add sp, #40
#endif /* CONFIG_EXTRA_EXCEPTION_INFO */
	pop {r0, pc}
