/*
 * Copyright (c) 2025-2026, Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ARM9_MACRO_PRIV_INC_
#define _ARM9_MACRO_PRIV_INC_

.macro z_arm_arm9_cpsr_set bit
	push {r0}
	mrs r0, cpsr
	orr r0, r0, #\bit
	msr cpsr, r0
	pop {r0}
.endm

.macro z_arm_arm9_cpsr_clear bit
	push {r0}
	mrs r0, cpsr
	bic r0, r0, #\bit
	msr cpsr, r0
	pop {r0}
.endm

.macro z_arm_arm9_mode reg mode
	mrs \reg, cpsr
	bic \reg, \reg, #MODE_MASK
	orr \reg, \reg, \mode
	msr cpsr, \reg
.endm

.macro z_arm_arm9_drain_write_buffer reg
	mov \reg, #0
	mcr p15, 0, \reg, c7, c10, 4
.endm

#endif /* _ARM9_MACRO_PRIV_INC_ */
