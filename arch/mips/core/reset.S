/*
 * Copyright (c) 2021 Antony Pavlov <antonynpavlov@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <mips/regdef.h>
#include <mips/mipsregs.h>

GTEXT(__initialize)
GTEXT(__stack)
GTEXT(z_prep_c)

/*
 * Remainder of asm-land initialization code before we can jump into
 * the C domain.
 */
SECTION_FUNC(TEXT, __initialize)
	.set noreorder

	mtc0 zero, CP0_CAUSE
	ehb

	mfc0 k0, CP0_STATUS
	li k1, ~(ST0_ERL | ST0_IE)
	and k0, k1
	mtc0 k0, CP0_STATUS
	ehb

#ifdef CONFIG_INIT_STACKS
	/* Pre-populate all bytes in z_interrupt_stacks with 0xAA */
	la t0, z_interrupt_stacks
	li t1, CONFIG_ISR_STACK_SIZE
	add t1, t1, t0

	/* Populate z_interrupt_stacks with 0xaaaaaaaa */
	li t2, 0xaaaaaaaa
aa_loop:
	sw t2, 0(t0)
	addi t0, t0, 4
	blt t0, t1, aa_loop
	nop /* delay slot */
#endif

	/*
	 * Setup stack pointer.
	 */
	la sp, __stack

	/*
	 * Jump into C domain.
	 */
	la v0, z_prep_c
	jal v0
	nop /* delay slot */
