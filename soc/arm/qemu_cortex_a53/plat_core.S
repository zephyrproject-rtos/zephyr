/*
 * Copyright 2020 Carlo Caione <ccaione@baylibre.com>
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

SECTION_FUNC(TEXT, z_arch_el3_plat_init)

	mov	x20, x30

#ifdef CONFIG_GIC_V3
	/* Enable GIC v3 system interface */
	mov_imm	x0, (ICC_SRE_ELx_DFB | ICC_SRE_ELx_DIB | \
		     ICC_SRE_ELx_SRE | ICC_SRE_EL3_EN)
	msr	ICC_SRE_EL3, x0
#endif

	mov	x30, x20
	ret
