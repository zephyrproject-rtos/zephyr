/*
 * Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <toolchain.h>
#include <linker/sections.h>
#include <arch/cpu.h>
#include <offsets.h>
#include "vector_table.h"
#include "macro_priv.inc"

_ASM_FILE_PROLOGUE

/*
 * Platform specific pre-C init code
 *
 * Note: - Stack is not yet available
 *       - x23 must be preserved
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

#if CONFIG_MP_NUM_CPUS > 1
/*
 * Global variable secondary_core_boot:
 * 0: is master Core booting
 * 1: is secondary Core booting
 */
SECTION_VAR(DATA,secondary_core_boot)
	.align 8
	.long 0
	.long 0
#endif

/*
 * Set the minimum necessary to safely call C code
 */

GTEXT(__reset_prep_c)
SECTION_SUBSEC_FUNC(TEXT,_reset_section,__reset_prep_c)
	/* return address: x23 */
	mov	x23, x30

	switch_el x0, 3f, 2f, 1f
3:
	/* Reinitialize SCTLR from scratch in EL3 */
	ldr	w0, =(SCTLR_EL3_RES1 | SCTLR_I_BIT | SCTLR_SA_BIT)
	msr	sctlr_el3, x0

	/* Custom plat prep_c init */
	bl	z_arm64_el3_plat_prep_c

	b	out
2:
	/* Disable alignment fault checking */
	mrs	x0, sctlr_el2
	bic	x0, x0, SCTLR_A_BIT
	msr	sctlr_el2, x0

	/* Custom plat prep_c init */
	bl	z_arm64_el2_plat_prep_c

	b	out
1:
	/* Disable alignment fault checking */
	mrs	x0, sctlr_el1
	bic	x0, x0, SCTLR_A_BIT
	msr	sctlr_el1, x0

	/* Custom plat prep_c init */
	bl	z_arm64_el1_plat_prep_c

out:
	isb

	/* Select SP_EL0 */
	msr	SPSel, #0

#if CONFIG_MP_NUM_CPUS > 1
	adr	x0, secondary_core_boot
	ldr	x0, [x0]
	cbnz    x0, L_secondary_stack
#endif

	/* Initialize stack */
	ldr	x0, =(z_interrupt_stacks)
	add	x0, x0, #(CONFIG_ISR_STACK_SIZE)
	mov	sp, x0

	ret	x23

#if CONFIG_MP_NUM_CPUS > 1
L_secondary_stack:
	get_cpu_id x3
	adr	x0, arm64_cpu_init
	mov	x2, #ARM64_CPU_INIT_SIZE
	/* Loop and find its table */
	mov	x1, xzr
4:
	madd	x4, x1, x2, x0
	add	x4, x4, #(__arm64_cpu_init_data_t_mpid_OFFSET)
	ldr	x4, [x4]
	cmp	x4, x3
	dmb	ld
	b.eq	5f
	add	x1, x1, #1
	cmp	x1, #CONFIG_MP_NUM_CPUS
	b.lt	4b
	/* can't find related table */
	b	L_enable_secondary
5:
	/* found related table */
	madd	x4, x1, x2, x0
	/* Get SP address */
	add     x4, x4, #(__arm64_cpu_init_data_t_sp_OFFSET)
	ldr	x4, [x4]
	cbz	x4, L_enable_secondary
	dmb	ld

	mov	sp, x4
	ret	x23
#endif

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
	/* Mask all exceptions */
	msr	DAIFSet, #0xf

	/* Prepare for calling C code */
	bl	__reset_prep_c

	/* Platform hook for highest EL */
	bl	z_arm64_el_highest_init

switch_el:
	switch_el x0, 3f, 2f, 1f
3:
	/* EL3 init */
	bl	z_arm64_el3_init

	/* Get next EL */
	adr	x0, switch_el
	bl	z_arm64_el3_get_next_el
	eret

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

	/* Enable SError interrupts */
	msr	DAIFClr, #(DAIFCLR_ABT_BIT)
	isb

#if CONFIG_MP_NUM_CPUS > 1
	adr	x0, secondary_core_boot
	ldr	x0, [x0]
	cbnz    x0, L_enable_secondary

	/*
	 * Is booting from master Core, change it to be "1" for all
	 * secondary core boot.
	 */
	adr	x0, secondary_core_boot
	mov	x1, #1
	str	x1, [x0]
#endif

	b	z_arm64_prep_c

#if CONFIG_MP_NUM_CPUS > 1
L_enable_secondary:
	b	z_arm64_secondary_prep_c
#endif
