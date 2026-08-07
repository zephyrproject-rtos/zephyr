/*
 * Copyright (c) 2019-2020 Cobham Gaisler AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/arch/sparc/sparc.h>

/* The trap table reset entry jumps to here. */

GTEXT(__sparc_trap_reset)
SECTION_FUNC(TEXT, __sparc_trap_reset)
#ifdef CONFIG_SPARC_SVT
#ifdef CONFIG_SOC_SPARC_LEON
	/* On LEON, enable single vector trapping by setting ASR17.SV. */
	rd	%asr17, %g1
	set	(1<<13), %g2
	or	%g1, %g2, %g1
	wr	%g1, %asr17
#else
#error "Don't know how to enable SVT on this SOC"
#endif
#endif

	set	__sparc_trap_table, %g1
	wr	%g1, %tbr
	wr	2, %wim
	wr	PSR_PIL | PSR_S | PSR_PS | PSR_ET, %psr
	/* NOTE: wrpsr above may have changed the current register window. */

	/* We are in the 3 instruction wrpsr delay so use global registers. */
	set	z_interrupt_stacks, %g2
	set	CONFIG_ISR_STACK_SIZE, %g4
	add	%g2, %g4, %g1
	and	%g1, 0xfffffff0, %l3

	/*
	 * According to SPARC ABI, Chapter 3: The system marks the deepest
	 * stack frame by setting the frame pointer to zero. No other frame's
	 * %fp has a zero value.
	 */
	sub	%l3, 96, %sp
	clr	%fp
	clr	%i7

#ifdef CONFIG_INIT_STACKS
	/* In-place memset() to avoid register window related traps. */
	set	0xaaaaaaaa, %l0
	mov	%l0, %l1
1:
	std	%l0, [%g2]
	add	%g2, 8, %g2
	cmp	%g2, %l3
	bne	1b
	 nop
#endif

	call	arch_bss_zero
	 nop

	call    z_prep_c
	 nop

/* We halt the system by generating a "trap in trap" condition. */
GTEXT(arch_system_halt)
SECTION_FUNC(TEXT, arch_system_halt)
	mov	%o0, %g0
	mov	%g1, %g0
	set	1, %g1
	ta	0x00
