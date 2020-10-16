/*
 * Copyright (c) 2019-2020 Cobham Gaisler AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <toolchain.h>
#include <linker/sections.h>
#include <arch/sparc/sparc.h>

/* The trap table reset entry jumps to here. */

GTEXT(__sparc_trap_reset)
SECTION_FUNC(TEXT, __sparc_trap_reset)
	set	__sparc_trap_table, %g1
	wr	%g1, %tbr
	wr	%g0, 4, %wim
	/* %psr := pil=0, et=0, cwp=1 */
	set	(PSR_S | PSR_PS | 1), %g7
	wr	%g7, %psr
	nop
	nop
	nop
	/* NOTE: wrpsr above may have changed the current register window. */

	/*
	 * According to SPARC ABI, Chapter 3: The system marks the deepest
	 * stack frame by setting the frame pointer to zero. No other frame's
	 * %fp has a zero value.
	 */
	set	z_interrupt_stacks, %o0
	set	CONFIG_ISR_STACK_SIZE, %o2
	add	%o0, %o2, %l2
	and	%l2, 0xfffffff0, %l3
	sub	%l3, 96, %sp
	clr	%fp
	clr	%i7

#ifdef CONFIG_INIT_STACKS
	/* already have z_interrupt_stacks and CONFIG_ISR_STACK_SIZE in place */
	call	memset
	 mov	0xaa, %o1
#endif

	call	z_bss_zero
	 nop

	/* Enable traps for the first time */
	/* %psr := pil=0, et=1, cwp=1 */
	wr	%g7, PSR_ET, %psr
	nop
	nop
	nop

	call    _PrepC
	 nop

/* We halt the system by generating a "trap in trap" condition. */
GTEXT(arch_system_halt)
SECTION_FUNC(TEXT, arch_system_halt)
	mov	%o0, %g0
	mov	%g1, %g0
	set	1, %g1
	ta	0x00
