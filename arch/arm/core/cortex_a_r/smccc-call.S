/*
 * Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>

.macro SMCCC instr
	push	{r4-r7, lr}
	ldr	r4, [sp, #20]
	ldr	r5, [sp, #24]
	ldr	r6, [sp, #28]
	ldr	r7, [sp, #32]
	\instr	#0
	ldr	ip, [sp, #36]
	stmia	ip, {r0-r7}
	pop	{r4-r7, pc}
.endm

GTEXT(arm_smccc_smc)
SECTION_FUNC(TEXT, arm_smccc_smc)
	SMCCC	smc

GTEXT(arm_smccc_hvc)
SECTION_FUNC(TEXT, arm_smccc_hvc)
	SMCCC	hvc
