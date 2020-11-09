/*
 * Copyright 2020 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * ARM64 Cortex-A Cache management
 */

#include <toolchain.h>
#include <linker/sections.h>
#include <arch/cpu.h>

_ASM_FILE_PROLOGUE

.macro	dcache_range_op, dcache op
	/* Calculate dcache line size */
	mrs	x3, ctr_el0
	mov	x2, #4
	ubfm	x3, x3, #16, #19
	lsl	x2, x2, x3

	/*
	 * x2 = cacheline_size
	 * x1 = start + end
	 * x3 = cacheline_size - 1
	 * x0 = x0 & ~(x3)
	 */
	add	x1, x0, x1
	sub	x3, x2, #1
	bic	x0, x0, x3
1:
	\dcache \op, x0
	add	x0, x0, x2
	cmp	x0, x1
	blo	1b
	dsb	sy
.endm

GTEXT(arch_dcache_flush)
SECTION_FUNC(TEXT, arch_dcache_flush)
	dcache_range_op dc civac
	ret

GTEXT(arch_dcache_invd)
SECTION_FUNC(TEXT, arch_dcache_invd)
	dcache_range_op dc ivac
	ret
