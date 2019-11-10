/*
 * Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
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
#include "vector_table.h"

/**
 *
 * @brief Reset vector
 *
 * Ran when the system comes out of reset. The processor is in thread mode with
 * privileged level. At this point, neither SP_EL0 nor SP_ELx point to a valid
 * area in SRAM.
 *
 * When these steps are completed, jump to z_arm64_prep_c(), which will finish
 * setting up the system for running C code.
 *
 * @return N/A
 */
GTEXT(__reset)
SECTION_SUBSEC_FUNC(TEXT,_reset_section,__reset)

/*
 * The entry point is located at the __reset symbol, which is fetched by a XIP
 * image playing the role of a bootloader, which jumps to it, not through the
 * reset vector mechanism. Such bootloaders might want to search for a __start
 * symbol instead, so create that alias here.
 */
GTEXT(__start)
SECTION_SUBSEC_FUNC(TEXT,_reset_section,__start)

	/* Setup vector table */
	adr	x0, _vector_table
	msr	vbar_el1, x0
	isb

	/* Disable access trapping in EL1 for NEON/FP */
	mov	x1, #(CPACR_EL1_FPEN_NOTRAP)
	msr	cpacr_el1, x1
	isb

	/*
	 * Enable the instruction cache, stack pointer and data access
	 * alignment checks and disable speculative loads.
	 */
	mov	x1, #(SCTLR_I_BIT | SCTLR_A_BIT | SCTLR_SA_BIT)
	mrs	x0, sctlr_el1
	orr	x0, x0, x1
	msr	sctlr_el1, x0
	isb

	/* Enable the SError interrupt */
	msr	daifclr, #(DAIFSET_ABT)

	/* Switch to SP_ELn and setup the stack */
	msr	spsel, #1
	ldr	x0, =(_interrupt_stack)
	add	x0, x0, #(CONFIG_ISR_STACK_SIZE)
	mov	sp, x0

	bl	z_arm64_prep_c
