/*
 * Copyright (c) 2019-2020 Cobham Gaisler AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <toolchain.h>
#include <linker/sections.h>
#include <arch/sparc/sparc.h>

GTEXT(__sparc_trap_sw_set_pil)

/*
 * Set processor interrupt level
 *
 * Handler for SPARC trap 0x89: trap_instruction, defined as "Reserved for the
 * operating system" by SPARC-ABI.
 *
 * entry:
 * - %l0: psr
 * - %l1: pc
 * - %l2: npc
 * - %i0: New processor interrupt level
 *
 * return:
 * - %i0: Old processor interrupt level
 */
SECTION_FUNC(TEXT, __sparc_trap_sw_set_pil)
	/* %l5: new %psr */
	sll	%i0, PSR_PIL_BIT, %i0
	andn	%l0, PSR_PIL, %l5
	or	%l5, %i0, %l5

	wr	%l5, %psr
	nop
	nop
	nop

	and	%l0, PSR_PIL, %l3
	srl	%l3, PSR_PIL_BIT, %i0

	jmp	%l2
	 rett	%l2 + 4
