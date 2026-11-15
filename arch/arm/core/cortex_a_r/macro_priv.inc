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

.macro z_arm_cortex_ar_enter_exc fpu_off=1
#if defined(CONFIG_USERSPACE)
	/*
	 * USR and SYS modes share SP.  On an exception from user mode,
	 * save the user SP and switch to the thread's privileged stack
	 * before pushing the exception frame.
	 */
	push {r0, r1}
	mrs r0, spsr
	and r0, #MODE_MASK
	cmp r0, #MODE_USR
	bne 1f
	mrs r1, cpsr
	get_cpu r0
	ldr r0, [r0, #___cpu_t_current_OFFSET]
	cps #MODE_SYS
	str sp, [r0, #_thread_offset_to_sp_usr]
	ldr sp, [r0, #_thread_offset_to_priv_stack_end]
	msr cpsr_c, r1
1:
	pop {r0, r1}
#endif

	/*
	 * Store r0-r3, r12, lr into the stack to construct an exception
	 * stack frame.
	 */
	srsdb sp!, #MODE_SYS
	cps #MODE_SYS
	push {r0-r3, r12, lr}

	/* TODO: EXTRA_EXCEPTION_INFO */
	mov r0, sp

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
 * Called in SYS mode with the return state (PC, SPSR) left on the SYS
 * mode stack, before RFE.  With CONFIG_USERSPACE, the return state moves
 * to the SVC mode stack, as the user stack cannot be trusted, and the
 * user SP is restored when returning to user mode.  Ends in SVC mode in
 * that case.
 */
.macro userspace_exc_exit
#if defined(CONFIG_USERSPACE)
	cps #MODE_SVC
	sub sp, #8
	push {r0-r1}

	cps #MODE_SYS
	pop {r0-r1}

	cps #MODE_SVC
	str r0, [sp, #8]
	str r1, [sp, #12]

	/* Only switch the stacks if returning to a user thread */
	and r1, #MODE_MASK
	cmp r1, #MODE_USR
	bne system_thread_exit\@

	/* Restore user stack pointer */
	get_cpu r0
	ldr r0, [r0, #___cpu_t_current_OFFSET]
	cps #MODE_SYS
	ldr sp, [r0, #_thread_offset_to_sp_usr] /* sp_usr */
	cps #MODE_SVC
system_thread_exit\@:
	pop {r0-r1}
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
	 * and r1 there on interrupt entry with CONFIG_USERSPACE.
	 */
	sub sp, r1, #8
	/* r1 keeps the stack 8-byte aligned */
	push {r0, r1}
1:
.endm

#endif /* _MACRO_PRIV_INC_ */
