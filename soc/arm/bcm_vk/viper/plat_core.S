/*
 * Copyright 2020 Broadcom
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 *@file
 *@brief plat/core specific init
*/

#include <toolchain.h>
#include <linker/sections.h>
#include <arch/cpu.h>

_ASM_FILE_PROLOGUE

GTEXT(z_arch_el3_plat_init)
GTEXT(plat_l2_init)

SECTION_FUNC(TEXT, z_arch_el3_plat_init)

	mov	x20, x30
	/* Enable GIC v3 system interface */
	mov_imm	x0, (ICC_SRE_ELx_DFB | ICC_SRE_ELx_DIB | \
		     ICC_SRE_ELx_SRE | ICC_SRE_EL3_EN)
	msr	ICC_SRE_EL3, x0
	/* L2 config */
	bl	plat_l2_init

	mov	x30, x20
	ret


SECTION_FUNC(TEXT,plat_l2_init)
	/*
	 * Set L2 Auxiliary Control Register of Cortex-A72
	 */
	/* Disable cluster coherency */
	mrs	x0, CORTEX_A72_L2ACTLR_EL1
	orr	x0, x0, #CORTEX_A72_L2ACTLR_DISABLE_ACE_SH_OR_CHI
	msr	CORTEX_A72_L2ACTLR_EL1, x0

	/* Set L2 Control Register */
	mov_imm	x1, ((CORTEX_A72_L2_DATA_RAM_LATENCY_MASK << \
		      CORTEX_A72_L2CTLR_DATA_RAM_LATENCY_SHIFT) | \
		     (CORTEX_A72_L2_TAG_RAM_LATENCY_MASK << \
		      CORTEX_A72_L2CTLR_TAG_RAM_LATENCY_SHIFT) | \
		     (CORTEX_A72_L2_TAG_RAM_SETUP_1_CYCLE << \
		      CORTEX_A72_L2CTLR_TAG_RAM_SETUP_SHIFT) | \
		     (CORTEX_A72_L2_DATA_RAM_SETUP_1_CYCLE << \
		      CORTEX_A72_L2CTLR_DATA_RAM_SETUP_SHIFT))
	bic	x0, x0, x1
	mov_imm	x1, ((CORTEX_A72_L2_DATA_RAM_LATENCY_3_CYCLES << \
		      CORTEX_A72_L2CTLR_DATA_RAM_LATENCY_SHIFT) | \
		     (CORTEX_A72_L2_TAG_RAM_SETUP_1_CYCLE << \
		      CORTEX_A72_L2CTLR_TAG_RAM_SETUP_SHIFT) | \
		     (CORTEX_A72_L2_DATA_RAM_SETUP_1_CYCLE << \
		      CORTEX_A72_L2CTLR_DATA_RAM_SETUP_SHIFT))
	orr	x0, x0, x1
	msr	CORTEX_A72_L2CTLR_EL1, x0

	dsb	sy
	isb
	ret
