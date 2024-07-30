/*
 * Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/offsets.h>
#include "boot.h"
#include "macro_priv.inc"

_ASM_FILE_PROLOGUE

/*
 * Platform specific pre-C init code
 *
 * Note: - Stack is not yet available
 *       - x23, x24 and x25 must be preserved
 */

WTEXT(z_arm64_el3_plat_prep_c)
SECTION_FUNC(TEXT,z_arm64_el3_plat_prep_c)
	ret

WTEXT(z_arm64_el2_plat_prep_c)
SECTION_FUNC(TEXT,z_arm64_el2_plat_prep_c)
	ret

WTEXT(z_arm64_el1_plat_prep_c)
SECTION_FUNC(TEXT,z_arm64_el1_plat_prep_c)
	ret

/*
 * Set the minimum necessary to safely call C code
 */

GTEXT(__reset_prep_c)
SECTION_SUBSEC_FUNC(TEXT,_reset_section,__reset_prep_c)
	/* return address: x23 */
	mov	x23, lr

	switch_el x0, 3f, 2f, 1f
3:
#if !defined(CONFIG_ARMV8_R)
	/* Reinitialize SCTLR from scratch in EL3 */
	ldr	w0, =(SCTLR_EL3_RES1 | SCTLR_I_BIT | SCTLR_SA_BIT)
	msr	sctlr_el3, x0
	isb

	/* Custom plat prep_c init */
	bl	z_arm64_el3_plat_prep_c

	/* Set SP_EL1 */
	msr     sp_el1, x24

	b	out
#endif /* CONFIG_ARMV8_R */
2:
	/* Disable alignment fault checking */
	mrs	x0, sctlr_el2
	bic	x0, x0, SCTLR_A_BIT
	msr	sctlr_el2, x0
	isb

	/* Custom plat prep_c init */
	bl	z_arm64_el2_plat_prep_c

	/* Set SP_EL1 */
	msr     sp_el1, x24

	b	out
1:
	/* Disable alignment fault checking */
	mrs	x0, sctlr_el1
	bic	x0, x0, SCTLR_A_BIT
	msr	sctlr_el1, x0
	isb

	/* Custom plat prep_c init */
	bl	z_arm64_el1_plat_prep_c

	/* Set SP_EL1. We cannot use sp_el1 at EL1 */
	msr     SPSel, #1
	mov     sp, x24
out:
	isb

	/* Select SP_EL0 */
	msr	SPSel, #0

	/* Initialize stack */
	mov	sp, x24

	/* fp = NULL */
	mov	fp, xzr

	ret	x23

/*
 * Reset vector
 *
 * Ran when the system comes out of reset. The processor is in thread mode with
 * privileged level. At this point, neither SP_EL0 nor SP_ELx point to a valid
 * area in SRAM.
 */

GTEXT(__reset)
SECTION_SUBSEC_FUNC(TEXT,_reset_section,__reset)

GTEXT(__start)
SECTION_SUBSEC_FUNC(TEXT,_reset_section,__start)

#ifdef CONFIG_WAIT_AT_RESET_VECTOR
resetwait:
	wfe
	b       resetwait
#endif

	/* Mask all exceptions */
	msr	DAIFSet, #0xf

#if CONFIG_MP_MAX_NUM_CPUS > 1

	/*
	 * Deal with multi core booting simultaneously to race for being the primary core.
	 * Use voting lock[1] with reasonable but minimal requirements on the memory system
	 * to make sure only one core wins at last.
	 *
	 * [1] kernel.org/doc/html/next/arch/arm/vlocks.html
	 */
	ldr	x0, =arm64_cpu_boot_params

	/*
	 * Get the "logic" id defined by cpu_node_list statically for voting lock self-identify.
	 * It is worth noting that this is NOT the final logic id (arch_curr_cpu()->id)
	 */
	get_cpu_logic_id	x1, x2, x3, x4	//x1: MPID, x2: logic id

	add	x4, x0, #BOOT_PARAM_VOTING_OFFSET

	/* signal our desire to vote */
	mov	w5, #1
	strb	w5, [x4, x2]
	ldr	x3, [x0, #BOOT_PARAM_MPID_OFFSET]
	cmn	x3, #1
	beq	1f

	/* some core already won, release */
	strb	wzr, [x4, x2]
	b	secondary_core

	/* suggest current core then release */
1:	str	x1, [x0, #BOOT_PARAM_MPID_OFFSET]
	strb	wzr, [x4, x2]
	dmb	ish

	/* then wait until every core else is done voting */
	mov	x5, #0
2:	ldrb	w3, [x4, x5]
	tst	w3, #255
	/* wait */
	bne	2b
	add	x5, x5, #1
	cmp	x5, #CONFIG_MP_MAX_NUM_CPUS
	bne	2b


	/* check if current core won */
	dmb	ish
	ldr	x3, [x0, #BOOT_PARAM_MPID_OFFSET]
	cmp	x3, x1
	beq	primary_core
	/* fallthrough secondary */

	/* loop until our turn comes */
secondary_core:
	dmb	ish
	ldr	x2, [x0, #BOOT_PARAM_MPID_OFFSET]
	cmp	x1, x2
	bne	secondary_core

	/* we can now load our stack pointer value and move on */
	ldr	x24, [x0, #BOOT_PARAM_SP_OFFSET]
	ldr	x25, =z_arm64_secondary_prep_c
	b	boot

primary_core:
#endif
	/* load primary stack and entry point */
	ldr	x24, =(z_interrupt_stacks + __z_interrupt_stack_SIZEOF)
	ldr	x25, =z_prep_c
boot:
	/* Prepare for calling C code */
	bl	__reset_prep_c

	/*
	 * Initialize the interrupt stack with 0xaa so stack utilization
	 * can be measured. This needs to be done before using the stack
	 * so that we don't clobber any data.
	 */
#ifdef CONFIG_INIT_STACKS
	mov_imm	x0, CONFIG_ISR_STACK_SIZE
	sub	x0, sp, x0
	sub     x9, sp, #8
	mov     x10, 0xaaaaaaaaaaaaaaaa
stack_init_loop:
	cmp     x0, x9
	beq     stack_init_done
	str     x10, [x0], #8
	b       stack_init_loop
stack_init_done:
#endif

	/* Platform hook for highest EL */
	bl	z_arm64_el_highest_init

switch_el:
	switch_el x0, 3f, 2f, 1f

3:
#if !defined(CONFIG_ARMV8_R)
	/* EL3 init */
	bl	z_arm64_el3_init

	/* Get next EL */
	adr	x0, switch_el
	bl	z_arm64_el3_get_next_el
	eret
#endif /* CONFIG_ARMV8_R */

2:
	/* EL2 init */
	bl	z_arm64_el2_init

	/* Move to EL1 with all exceptions masked */
	mov_imm	x0, (SPSR_DAIF_MASK | SPSR_MODE_EL1T)
	msr	spsr_el2, x0

	adr	x0, 1f
	msr	elr_el2, x0
	eret

1:
	/* EL1 init */
	bl	z_arm64_el1_init

	/* We want to use SP_ELx from now on */
	msr	SPSel, #1

	/* Enable SError interrupts */
	msr	DAIFClr, #(DAIFCLR_ABT_BIT)
	isb

	ret	x25  /* either z_prep_c or z_arm64_secondary_prep_c */
