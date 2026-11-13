/*
 * Copyright (c) 2026 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/arch/arm64/macro.inc>

_ASM_FILE_PROLOGUE

/* Clean and invalidate [\start, \end) to the PoC. Clobbers \start, x9 and x10. */
.macro dcache_civac_range, start, end
	mrs	x9, ctr_el0
	ubfx	x9, x9, #16, #4		/* DminLine: log2 of words per line */
	mov	x10, #4
	lsl	x9, x10, x9		/* line size in bytes */
	sub	x10, x9, #1
	bic	\start, \start, x10
1:	dc	civac, \start
	add	\start, \start, x9
	cmp	\start, \end
	b.lo	1b
	dsb	sy
.endm

/*
 * UEFI entry point, set as AddressOfEntryPoint in the PE header.
 * x0: image handle, x1: system table.
 *
 * UEFI runs us wherever it loaded the image, with its own page tables.
 * Hand the load and link addresses of the image start to the C stub, which
 * returns to UEFI on failure.
 */
GTEXT(arm64_efi_entry)
SECTION_FUNC(TEXT, arm64_efi_entry)
	adrp	x2, __text_region_start
	add	x2, x2, :lo12:__text_region_start
	ldr	x3, =__text_region_start
	b	arm64_efi_stub

/*
 * void arm64_efi_enter_zephyr(uintptr_t start, uintptr_t end, uintptr_t entry)
 *
 * Runs after ExitBootServices(), still on the UEFI page tables.
 * Cleans the copied image in [start, end) to the PoC, turns off the MMU and
 * the data cache, and jumps to entry the way a bootloader would.
 */
GTEXT(arm64_efi_enter_zephyr)
SECTION_FUNC(TEXT, arm64_efi_enter_zephyr)
	msr	DAIFSet, #0xf
	mov	x19, x2

	dcache_civac_range x0, x1

	/*
	 * The code from .Lmmu_off on runs with the MMU off, fetched from memory
	 * UEFI wrote with the data cache on. Push it to the PoC too.
	 */
	adr	x0, .Lmmu_off
	adr	x1, .Lmmu_off_end
	dcache_civac_range x0, x1
	ic	iallu
	dsb	sy
	isb

.Lmmu_off:
	mov	x1, #(SCTLR_M_BIT | SCTLR_C_BIT)
	switch_el x0, .Lel3, .Lel2, .Lel1
.Lel2:
	mrs	x0, sctlr_el2
	bic	x0, x0, x1
	msr	sctlr_el2, x0
	isb
	/* Drop whatever UEFI set in HCR_EL2 (VHE, stage 2, traps): EL1 is AArch64, that's all */
	mov	x0, #HCR_RW_BIT
	msr	hcr_el2, x0
	isb
	b	.Ljump
.Lel1:
	mrs	x0, sctlr_el1
	bic	x0, x0, x1
	msr	sctlr_el1, x0
	isb
.Ljump:
	mov	x0, xzr
	mov	x1, xzr
	mov	x2, xzr
	mov	x3, xzr
	br	x19

	/* UEFI never runs at EL3 */
.Lel3:	b	.Lel3
.Lmmu_off_end:
