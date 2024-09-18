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

.macro z_arm_cortex_ar_enter_exc
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
.endm

#endif /* _MACRO_PRIV_INC_ */
