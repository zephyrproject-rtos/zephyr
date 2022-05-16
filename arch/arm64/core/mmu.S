/*
 * Copyright (c) 2021 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/arch/cpu.h>

_ASM_FILE_PROLOGUE

/*
 * Switch TTBR0
 */

GTEXT(z_arm64_set_ttbr0)
SECTION_FUNC(TEXT, z_arm64_set_ttbr0)

	/* Disable all the caches */
	mrs	x2, sctlr_el1
	mov_imm	x1, (SCTLR_M_BIT | SCTLR_C_BIT | SCTLR_I_BIT)
	and	x1, x2, x1
	msr	sctlr_el1, x1
	isb

	/* Switch the TTBR0 */
	msr	ttbr0_el1, x0
	isb

	/* Invalidate the TLBs */
	tlbi	vmalle1
	dsb	sy
	isb

	/* Restore the saved SCTLR_EL1 */
	msr	sctlr_el1, x2
	isb

	ret
