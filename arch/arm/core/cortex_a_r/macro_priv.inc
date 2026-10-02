/*
 * Copyright (c) 2023 Arm Limited (or its affiliates). All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MACRO_PRIV_INC_
#define _MACRO_PRIV_INC_

#include <zephyr/arch/arm/cortex_a_r/tpidruro.h>

/*
 * Get CPU id
 */

.macro get_cpu_id rreg0
	/* Read MPIDR register */
	mrc p15, 0, \rreg0, c0, c0, 5
	ubfx	\rreg0, \rreg0, #0, #24
.endm

/*
 * Get CPU logic id by looking up cpu_node_list
 * returns
 *   reg0: MPID
 *   reg1: logic id (0 ~ CONFIG_MP_MAX_NUM_CPUS - 1)
 * clobbers: reg0, reg1, reg2, reg3
 */
.macro get_cpu_logic_id reg0, reg1, reg2, reg3
	get_cpu_id \reg0
	ldr	\reg3, =cpu_node_list
	mov	\reg1, #0
1:	ldr	\reg2, [\reg3, \reg1, lsl #2]
	cmp	\reg2, \reg0
	beq	2f
	add	\reg1, \reg1, #1
	cmp	\reg1, #CONFIG_MP_MAX_NUM_CPUS
	bne	1b
	b	.
2:
.endm

.macro get_cpu rreg0
	/*
         * Get CPU pointer.
	 */
	mrc p15, 0, \rreg0, c13, c0, 3
        and \rreg0, #TPIDRURO_CURR_CPU
.endm

/*
 * Build the exception frame on the SYS mode stack and switch to SYS mode.
 * Leaves r0 pointing to the struct arch_esf and r2 to the current CPU.
 */
.macro z_arm_cortex_ar_enter_exc fpu_off=1
#if defined(CONFIG_USERSPACE)
	/*
	 * USR and SYS modes share SP.  On an exception from user mode,
	 * save the user SP and switch to the thread's privileged stack
	 * before pushing the exception frame. The ^ forms of STM and LDM
	 * access the user mode SP from the exception mode. USR is the only
	 * mode with the low 4 bits clear.
	 */
	push {r0}
	mrs r0, spsr
	tst r0, #0xf
	bne 1f
	get_cpu r0
	ldr r0, [r0, #___cpu_t_current_OFFSET]
	add r0, r0, #_thread_offset_to_priv_stack_end
.if _thread_offset_to_sp_usr != (_thread_offset_to_priv_stack_end + 4)
.error "sp_usr must follow priv_stack_end"
.endif
	stmib r0, {sp}^
	ldmia r0, {sp}^
1:
	pop {r0}
#endif

	/*
	 * Store r0-r3, r12, lr into the stack to construct an exception
	 * stack frame.
	 */
	srsdb sp!, #MODE_SYS
	cps #MODE_SYS
	push {r0-r3, r12, lr}

	/*
	 * The interrupted SP may be 4 modulo 8, and so the frame. Align SP to
	 * 8 bytes for the C calls that follow, and store the frame address
	 * at SP for z_arm_cortex_ar_exit_exc. r0 points to the struct
	 * arch_esf.
	 */
	mov r0, sp
#if defined(CONFIG_EXTRA_EXCEPTION_INFO)
	/*
	 * Room for the callee-saved registers and the extra info below the
	 * frame. Only the fault paths fill it, with z_arm_cortex_ar_save_extra.
	 */
	sub r1, r0, #___exc_extra_SIZEOF
	bic r1, r1, #7
#else
	bic r1, r0, #7
#endif
	/*
	 * Store the frame address before moving SP: if the store hits the MPU
	 * guard of the stack, SP is left at the frame, and the data abort
	 * entry hits the guard as well. With CONFIG_EXTRA_EXCEPTION_INFO, SP
	 * moves down by more than the 64 bytes of the guard.
	 * z_arm_fault_stack_overflow() recognizes these two instructions.
	 */
	str r0, [r1, #-8]
	sub sp, r1, #8
#if defined(CONFIG_EXTRA_EXCEPTION_INFO)
	sub r0, r0, #___extra_esf_info_t_SIZEOF
#endif

	/* increment exception depth */
	get_cpu r2
	ldrb r1, [r2, #_cpu_offset_to_exc_depth]
	add r1, r1, #1
	strb r1, [r2, #_cpu_offset_to_exc_depth]

#if defined(CONFIG_FPU_SHARING)
.if \fpu_off
	/* Exception handlers run with the FPU disabled */
	mov r1, #0
	vmsr fpexc, r1
.endif
#endif
.endm

/*
 * Fill the callee-saved registers and the extra info of the exception frame
 * built by z_arm_cortex_ar_enter_exc, and point r0 to that frame. SP must be
 * where z_arm_cortex_ar_enter_exc left it, and r4-r11 must still hold the
 * values of the interrupted context: only C functions, which preserve them,
 * may run in between. Clobbers r1-r3.
 */
.macro z_arm_cortex_ar_save_extra
	ldr r0, [sp]
#if defined(CONFIG_EXTRA_EXCEPTION_INFO)
	sub r1, r0, #___exc_extra_SIZEOF
	stm r1, {r4-r11}
	/* The interrupted SP is above the exception frame... */
	add r2, r0, #(___esf_t_SIZEOF - ___extra_esf_info_t_SIZEOF)
#if defined(CONFIG_USERSPACE)
	/* ...or in the thread for an exception from user mode */
	ldr r3, [r0, #___basic_sf_t_xpsr_OFFSET]
	tst r3, #0xf
	bne 1f
	get_cpu r2
	ldr r2, [r2, #___cpu_t_current_OFFSET]
	ldr r2, [r2, #_thread_offset_to_sp_usr]
1:
#endif
	str r2, [r1, #___callee_saved_t_psp_OFFSET]
	sub r0, r0, #___extra_esf_info_t_SIZEOF
	str r1, [r0]
	mov r1, #0
	str r1, [r0, #4]
#endif
.endm

/*
 * Return from the exception whose frame address z_arm_cortex_ar_enter_exc
 * stored at SP. Called in SYS mode. With CONFIG_USERSPACE, a return to user
 * mode restores the user SP, then restores the registers and returns from
 * SVC mode, with SVC mode LR pointing to the frame: the user stack cannot
 * be trusted. The ^ form of LDM loads the user mode LR.
 */
.macro z_arm_cortex_ar_exc_return
	ldr sp, [sp]
#if defined(CONFIG_USERSPACE)
	ldr r3, [sp, #___basic_sf_t_xpsr_OFFSET]
	tst r3, #0xf
	beq 1f
#endif
	pop {r0-r3, r12, lr}
	rfeia sp!
#if defined(CONFIG_USERSPACE)
1:
	get_cpu r0
	ldr r0, [r0, #___cpu_t_current_OFFSET]
	mov r1, sp
	ldr sp, [r0, #_thread_offset_to_sp_usr]
	cps #MODE_SVC
	add lr, r1, #___basic_sf_t_pc_OFFSET
	ldmdb lr, {r0-r3, r12, lr}^
	rfeia lr
#endif
.endm

/*
 * Move to the IRQ stack if \rnested, the interrupt nesting count after its
 * increment, is 1, and push the current SP there. ISRs and irq_offload
 * routines run in system mode on the IRQ stack: in IRQ mode, a nested
 * interrupt would overwrite LR_irq. A nested interrupt then pushes its
 * exception frame on the IRQ stack. z_arm_cortex_ar_irq_done undoes this.
 * Clobbers r0 and r1.
 */
.macro irq_stack_enter rnested
	cmp \rnested, #1
	bhi 1f
	mov r0, sp
	cps #MODE_IRQ
	mov r1, sp
	cps #MODE_SYS
	/*
	 * Leave the top 8 bytes to z_arm_cortex_ar_enter_exc, which saves r0
	 * there on interrupt entry with CONFIG_USERSPACE.
	 */
	sub sp, r1, #8
	/* r1 keeps the stack 8-byte aligned */
	push {r0, r1}
1:
.endm

#endif /* _MACRO_PRIV_INC_ */
