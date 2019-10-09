/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Reset handler
 *
 * Reset handler that prepares the system for running C code.
 */

#include <toolchain.h>
#include <linker/sections.h>
#include <arch/cpu.h>
#include <offsets_short.h>
#include "vector_table.h"

_ASM_FILE_PROLOGUE

GTEXT(z_arm_reset)
GDATA(_interrupt_stack)
GDATA(z_arm_svc_stack)
GDATA(z_arm_sys_stack)
GDATA(z_arm_fiq_stack)
GDATA(z_arm_abort_stack)
GDATA(z_arm_undef_stack)

#define STACK_MARGIN	4


/**
 *
 * @brief Reset vector
 *
 * Ran when the system comes out of reset. The processor is in thread mode with
 * privileged level. At this point, the main stack pointer (MSP) is already
 * pointing to a valid area in SRAM.
 *
 * When these steps are completed, jump to z_arm_prep_c(), which will finish
 * setting up the system for running C code.
 *
 * @return N/A
 */
SECTION_SUBSEC_FUNC(TEXT, _reset_section, z_arm_reset)
SECTION_SUBSEC_FUNC(TEXT,_reset_section,__start)
	mov r0,  #0
	mov r1,  #0
	mov r2,  #0
	mov r3,  #0
	mov r4,  #0
	mov r5,  #0
	mov r6,  #0
	mov r7,  #0
	mov r8,  #0
	mov r9,  #0
	mov r10, #0
	mov r11, #0
	mov r12, #0
	mov r14, #0

	/* lock interrupts: will get unlocked when switch to main task */
	cpsid if

	/* Setup FIQ stack */
	msr CPSR_c, #(MODE_FIQ | I_BIT | F_BIT)
	ldr sp, =(z_arm_fiq_stack + CONFIG_ARMV7_FIQ_STACK_SIZE - STACK_MARGIN)

	/* Setup IRQ stack */
	msr CPSR_c, #(MODE_IRQ | I_BIT | F_BIT)
	ldr sp, =(_interrupt_stack + CONFIG_ISR_STACK_SIZE - STACK_MARGIN)

	/* Setup data abort stack */
	msr CPSR_c, #(MODE_ABT | I_BIT | F_BIT)
	ldr sp, =(z_arm_abort_stack + CONFIG_ARMV7_EXCEPTION_STACK_SIZE - \
				STACK_MARGIN)

	/* Setup undefined mode stack */
	msr CPSR_c, #(MODE_UDF | I_BIT | F_BIT)
	ldr sp, =(z_arm_undef_stack + CONFIG_ARMV7_EXCEPTION_STACK_SIZE - \
				STACK_MARGIN)

	/* Setup SVC mode stack */
	msr CPSR_c, #(MODE_SVC | I_BIT | F_BIT)
	ldr sp, =(z_arm_svc_stack + CONFIG_ARMV7_SVC_STACK_SIZE - STACK_MARGIN)

	/* Setup System mode stack */
	msr CPSR_c, #(MODE_SYS | I_BIT | F_BIT)
	ldr sp, =(z_arm_sys_stack + CONFIG_ARMV7_SYS_STACK_SIZE - STACK_MARGIN)

	/* Setup system control register */
	mrc p15, 0, r0, c1, c0, 0	/* SCTLR */
	bic r0, r0, #HIVECS		/* Exception vectors from 0-0x1c */
	mcr p15, 0, r0, c1, c0, 0

#if defined(CONFIG_WDOG_INIT)
	/* board-specific watchdog initialization is necessary */
	bl z_arm_watchdog_init
#endif

	b z_arm_prep_c
