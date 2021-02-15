/*
 * Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This file implements the common calling mechanism to be used with the Secure
 * Monitor Call (SMC) and Hypervisor Call (HVC).
 *
 * See http://infocenter.arm.com/help/topic/com.arm.doc.den0028a/index.html
 */

#include <toolchain.h>
#include <linker/sections.h>
#include <arch/cpu.h>

.macro SMCCC instr
	\instr  #0
	ldr     x4, [sp]
	stp     x0, x1, [x4]
	stp     x2, x3, [x4, #16]
	ret
.endm

/*
 * The SMC instruction is used to generate a synchronous exception that is
 * handled by Secure Monitor code running in EL3.
 */
GTEXT(arm_smccc_smc)
SECTION_FUNC(TEXT, arm_smccc_smc)
	SMCCC	smc

/*
 * The HVC instruction is used to generate a synchronous exception that is
 * handled by a hypervisor running in EL2.
 */
GTEXT(arm_smccc_hvc)
SECTION_FUNC(TEXT, arm_smccc_hvc)
	SMCCC	hvc
