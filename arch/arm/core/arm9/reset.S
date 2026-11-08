/*
 * Copyright (c) 2026 Takayuki Goto
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARMv5 reset handler
 *
 * Brings the core into a known state (MMU and caches off, caches and TLBs
 * invalidated), installs one stack per processor mode and hands over to
 * z_prep_c().
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/arch/cpu.h>
#include <offsets_short.h>
#include "vector_table.h"
#include "macro_priv.inc"

_ASM_FILE_PROLOGUE

GTEXT(z_arm_reset)
GDATA(z_arm_svc_stack)
GDATA(z_arm_sys_stack)
GDATA(z_arm_fiq_stack)
GDATA(z_arm_abort_stack)
GDATA(z_arm_undef_stack)
#if defined(CONFIG_SOC_RESET_HOOK)
GTEXT(soc_reset_hook)
#endif

/*
 * Each exception mode keeps a three word scratch area at the very top of its
 * stack; z_armv5_enter_exc() parks r0, lr_<exc> and spsr_<exc> there.
 */
#define EXC_SCRATCH_SIZE	12

SECTION_SUBSEC_FUNC(TEXT, _reset_section, z_arm_reset)
SECTION_SUBSEC_FUNC(TEXT, _reset_section, __start)

	/*
	 * Make sure we are in SVC mode with both IRQ and FIQ masked, whatever
	 * the boot loader left behind.
	 */
	msr	cpsr_c, #(MODE_SVC | I_BIT | F_BIT)

	/*
	 * Turn the MMU, the caches and the branch predictor off. Zephyr sets
	 * these up itself later on; a boot loader (or a warm reset) may well
	 * have left them enabled with stale translations.
	 */
	mrc	p15, 0, r0, c1, c0, 0
	bic	r0, r0, #(SCTLR_M_BIT | SCTLR_C_BIT | SCTLR_A_BIT)
	bic	r0, r0, #SCTLR_I_BIT
	bic	r0, r0, #SCTLR_Z_BIT
	bic	r0, r0, #SCTLR_V_BIT
	mcr	p15, 0, r0, c1, c0, 0

	mov	r0, #0
	mcr	p15, 0, r0, c7, c7, 0		/* invalidate I and D caches */
	mcr	p15, 0, r0, c7, c10, 4		/* drain write buffer */
	mcr	p15, 0, r0, c8, c7, 0		/* invalidate I and D TLBs */
	mcr	p15, 0, r0, c13, c0, 0		/* reset FCSE PID */

	/*
	 * Install one stack per mode. The exception modes only ever use the
	 * three word scratch area at the top of their stack: the handlers
	 * themselves run in system mode on the interrupted stack.
	 */
	ldr	r0, =(z_arm_fiq_stack + CONFIG_ARMV5_FIQ_STACK_SIZE - EXC_SCRATCH_SIZE)
	msr	cpsr_c, #(MODE_FIQ | I_BIT | F_BIT)
	mov	sp, r0

	ldr	r0, =(z_arm_irq_stack + CONFIG_ARMV5_EXCEPTION_STACK_SIZE - EXC_SCRATCH_SIZE)
	msr	cpsr_c, #(MODE_IRQ | I_BIT | F_BIT)
	mov	sp, r0

	ldr	r0, =(z_arm_abort_stack + CONFIG_ARMV5_EXCEPTION_STACK_SIZE - EXC_SCRATCH_SIZE)
	msr	cpsr_c, #(MODE_ABT | I_BIT | F_BIT)
	mov	sp, r0

	ldr	r0, =(z_arm_undef_stack + CONFIG_ARMV5_EXCEPTION_STACK_SIZE - EXC_SCRATCH_SIZE)
	msr	cpsr_c, #(MODE_UND | I_BIT | F_BIT)
	mov	sp, r0

	ldr	r0, =(z_arm_svc_stack + CONFIG_ARMV5_SVC_STACK_SIZE - EXC_SCRATCH_SIZE)
	msr	cpsr_c, #(MODE_SVC | I_BIT | F_BIT)
	mov	sp, r0

	/* Threads and exception handlers run in system mode. */
	ldr	r0, =(z_arm_sys_stack + CONFIG_ARMV5_SYS_STACK_SIZE)
	msr	cpsr_c, #(MODE_SYS | I_BIT | F_BIT)
	mov	sp, r0

#if defined(CONFIG_SOC_RESET_HOOK)
	bl	soc_reset_hook
#endif

	ldr	r0, =z_prep_c
	bx	r0
