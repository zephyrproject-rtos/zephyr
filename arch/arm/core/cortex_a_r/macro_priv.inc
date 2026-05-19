/*
 * Copyright (c) 2023 Arm Limited (or its affiliates). All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MACRO_PRIV_INC_
#define _MACRO_PRIV_INC_

#ifndef CONFIG_ARMV5
#include <zephyr/arch/arm/cortex_a_r/tpidruro.h>
#endif

.macro change_processor_state mode scratch_reg
#ifdef CONFIG_ARMV5
	mrs \scratch_reg, cpsr
	bic \scratch_reg, \scratch_reg, #MODE_MASK
	orr \scratch_reg, \scratch_reg, \mode
	msr cpsr, \scratch_reg
#else
	cps \mode
#endif
.endm

.macro z_arm_cpsid_i
#ifdef CONFIG_ARMV5
	/* Use r0 as a temp */
	push {r0}
	mrs r0, cpsr
	orr r0, r0, #I_BIT
	msr cpsr, r0
	pop {r0}
#else
	cpsid i
#endif
.endm

.macro z_arm_cpsie_i
#ifdef CONFIG_ARMV5
	/* Use r0 as a temp */
	push {r0}
	mrs r0, cpsr
	bic r0, r0, #I_BIT
	msr cpsr, r0
	pop {r0}
#else
	cpsie i
#endif
.endm

/*
 * data_sync_barrier scratch
 *
 * Issues a data synchronisation barrier. On ARMv5 this requires a scratch
 * register (\scratch) to hold the immediate zero for the MCR instruction;
 * on ARMv6+ the DSB instruction is used directly and scratch is not touched.
 */
.macro data_sync_barrier scratch
#ifdef CONFIG_ARMV5
	mov \scratch, #0
	mcr p15, 0, \scratch, c7, c10, 4
#else
	dsb
#endif
.endm

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
#ifdef CONFIG_ARMV5
	ldr \rreg0, =_kernel
#else
	/*
         * Get CPU pointer.
	 */
	mrc p15, 0, \rreg0, c13, c0, 3
        and \rreg0, #TPIDRURO_CURR_CPU
#endif
.endm

.macro z_arm_cortex_ar_enter_exc
#ifdef CONFIG_ARMV5
	stmdb sp, {r0-r3}
	mov r1, lr
	mrs r2, spsr
	mov r3, sp
	change_processor_state #MODE_SYS, r0
	push {r1, r2}
	ldmdb r3, {r0-r3}
	push {r0-r3, r12, lr}
#else
	/*
	 * Store r0-r3, r12, lr into the stack to construct an exception
	 * stack frame.
	 */
	srsdb sp!, #MODE_SYS
	cps #MODE_SYS
	push {r0-r3, r12, lr}
#endif /* CONFIG_ARMV5 */

	/* TODO: EXTRA_EXCEPTION_INFO */
	mov r0, sp

	/* increment exception depth */
	get_cpu r2
	ldrb r1, [r2, #_cpu_offset_to_exc_depth]
	add r1, r1, #1
	strb r1, [r2, #_cpu_offset_to_exc_depth]
.endm

#endif /* _MACRO_PRIV_INC_ */
