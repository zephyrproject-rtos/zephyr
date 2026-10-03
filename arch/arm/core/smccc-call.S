/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This file implements the common calling mechanism to be used with the Secure
 * Monitor Call (SMC) and Hypervisor Call (HVC).
 *
 * See https://developer.arm.com/docs/den0028/latest
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/arch/cpu.h>
#include <offsets_short.h>

_ASM_FILE_PROLOGUE

/*
 * -mcpu=<cpu> restricts the assembler to the instruction encodings that CPU
 * implements, so on cores without the Security/Virtualization Extensions
 * (e.g. Cortex-A9) GAS rejects the `smc`/`hvc` opcodes outright. These
 * directives just unlock the encodings in the assembler; whether the
 * instruction actually works still depends on the conduit the platform
 * firmware/hypervisor advertises at runtime.
 */
.arch_extension sec
.arch_extension virt

/*
 * a0-a3 arrive in r0-r3. a4, a5, a6, a7 and res arrive on the caller's stack,
 * in that order, at [sp, #0] through [sp, #16] on entry to this macro. The
 * push below shifts those stack slots down by 20 bytes (5 saved registers).
 */
.macro SMCCC instr
	push    {r4-r7, lr}

	ldr     r4, [sp, #20]
	ldr     r5, [sp, #24]
	ldr     r6, [sp, #28]
	ldr     r7, [sp, #32]

	\instr  #0

	ldr     r4, [sp, #36]
	str     r0, [r4, #__arm_smccc_res_t_a0_OFFSET]
	str     r1, [r4, #__arm_smccc_res_t_a1_OFFSET]
	str     r2, [r4, #__arm_smccc_res_t_a2_OFFSET]
	str     r3, [r4, #__arm_smccc_res_t_a3_OFFSET]

	pop     {r4-r7, lr}
	bx      lr
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
