/*
 * Copyright (c) 2025-2026, Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ARM9_MACRO_PRIV_INC_
#define _ARM9_MACRO_PRIV_INC_

/* Workaround for the unsupported instruction "cps" in ARM9 */
.macro change_processor_state mode scratch_reg
	mrs \scratch_reg, cpsr
	bic \scratch_reg, \scratch_reg, #MODE_MASK
	orr \scratch_reg, \scratch_reg, \mode
	msr cpsr, \scratch_reg
.endm

/* Workaround for the unsupported instruction "cpsid i" in ARM9 */
.macro z_arm_cpsid_i
	/* Use r0 as a temp */
	push {r0}
	mrs r0, cpsr
	orr r0, r0, #I_BIT
	msr cpsr, r0
	pop {r0}
.endm

/* Workaround for the unsupported instruction "cpsie i" in ARM9 */
.macro z_arm_cpsie_i
	/* Use r0 as a temp */
	push {r0}
	mrs r0, cpsr
	bic r0, r0, #I_BIT
	msr cpsr, r0
	pop {r0}
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
#endif /* CONFIG_ARMV5 */
.endm

#endif /* _ARM9_MACRO_PRIV_INC_ */
