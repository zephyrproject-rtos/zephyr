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
#include "macro.h"

_ASM_FILE_PROLOGUE

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

#ifdef CONFIG_SWITCH_TO_EL1
	switch_el x1, 3f, 2f, 1f
3:
	/* Disable MMU and async exceptions routing to EL1 */
	msr	sctlr_el1, xzr

	/* Disable EA/IRQ/FIQ routing to EL3 and set EL1 to AArch64 */
	mov	x0, xzr
	orr	x0, x0, #(SCR_EL3_RW)
	msr	scr_el3, x0

	/* On eret return to EL1 with DAIF masked */
	mov	x0, xzr
	orr	x0, x0, #(DAIF_MASK)
	orr	x0, x0, #(SPSR_EL3_TO_EL1)
	orr	x0, x0, #(SPSR_EL3_h)
	msr	spsr_el3, x0

	adr	x0, 1f
	msr	elr_el3, x0
	eret
2:
	/* Boot from EL2 not supported */
	bl	.
1:
#endif
	/* Setup vector table */
	adr	x0, _vector_table

	switch_el x1, 3f, 2f, 1f
3:
	/* Initialize VBAR */
	msr	vbar_el3, x0

	/* SError, IRQ and FIQ routing enablement in EL3 */
	mrs	x0, scr_el3
	orr	x0, x0, #(SCR_EL3_IRQ | SCR_EL3_FIQ | SCR_EL3_EA)
	msr	scr_el3, x0

	/* Disable access trapping in EL3 for NEON/FP */
	msr	cptr_el3, xzr

	/*
	 * Enable the instruction cache, stack pointer and data access
	 * alignment checks and disable speculative loads.
	 */
	mov	x1, #(SCTLR_I_BIT | SCTLR_A_BIT | SCTLR_SA_BIT)
	mrs	x0, sctlr_el3
	orr	x0, x0, x1
	msr	sctlr_el3, x0
	b	0f
2:
	/* Initialize VBAR */
	msr	vbar_el2, x0

	/* SError, IRQ and FIQ routing enablement in EL2 */
	mrs	x0, hcr_el2
	orr	x0, x0, #(HCR_EL2_FMO | HCR_EL2_IMO | HCR_EL2_AMO)
	msr	hcr_el2, x0

	/* Disable access trapping in EL2 for NEON/FP */
	msr	cptr_el2, xzr

	/*
	 * Enable the instruction cache, stack pointer and data access
	 * alignment checks and disable speculative loads.
	 */
	mov	x1, #(SCTLR_I_BIT | SCTLR_A_BIT | SCTLR_SA_BIT)
	mrs	x0, sctlr_el2
	orr	x0, x0, x1
	msr	sctlr_el2, x0
	b	0f
1:
	/* Initialize VBAR */
	msr	vbar_el1, x0

	/* Disable access trapping in EL1 for NEON/FP */
	mov	x1, #(CPACR_EL1_FPEN_NOTRAP)
	msr	cpacr_el1, x1

	/*
	 * Enable the instruction cache, stack pointer and data access
	 * alignment checks and disable speculative loads.
	 */
	mov	x1, #(SCTLR_I_BIT | SCTLR_A_BIT | SCTLR_SA_BIT)
	mrs	x0, sctlr_el1
	orr	x0, x0, x1
	msr	sctlr_el1, x0
0:
	isb

	/* Enable the SError interrupt */
	msr	daifclr, #(DAIFSET_ABT)

	/* Switch to SP_ELn and setup the stack */
	msr	spsel, #1
	ldr	x0, =(_interrupt_stack)
	add	x0, x0, #(CONFIG_ISR_STACK_SIZE)
	mov	sp, x0

	bl	z_arm64_prep_c
