/*
 * Copyright (c) 2019-2020 Cobham Gaisler AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <toolchain.h>
#include <linker/sections.h>
#include <offsets_short.h>
#include <arch/sparc/sparc.h>

GTEXT(__sparc_trap_fault)

/*
 * Fault trap handler
 *
 * - IU state is saved and restored
 *
 * On entry:
 * %l0: psr (set by trap code)
 * %l1: pc
 * %l2: npc
 * %l6: tbr (set by trap code)
 * %fp: %sp of current register window at trap time
 *
 * This trap handler will trash some of the global registers, which is OK since
 * we will not return to where we trapped.
 */
SECTION_FUNC(TEXT, __sparc_trap_fault)
	/* We may have trapped into the invalid window. If so, make it valid. */
	rd	%wim, %g2
	srl	%g2, %l0, %g3
	cmp	%g3, 1
	bne	.Lwodone
	 nop

	/* Do the window overflow. */
	sll	%g2, (CONFIG_SPARC_NWIN-1), %g3
	srl	%g2, 1, %g2
	or	%g2, %g3, %g2

	/* Enter window to save. */
	save
	/* Install new wim calculated above. */
	mov	%g2, %wim
	nop
	nop
	nop
	/* Put registers on the dedicated save area of the ABI stack frame. */
	std	%l0, [%sp + 0x00]
	std	%l2, [%sp + 0x08]
	std	%l4, [%sp + 0x10]
	std	%l6, [%sp + 0x18]
	std	%i0, [%sp + 0x20]
	std	%i2, [%sp + 0x28]
	std	%i4, [%sp + 0x30]
	std	%i6, [%sp + 0x38]
	/* Leave saved window. */
	restore

.Lwodone:
	/* Allocate an ABI stack frame and exception stack frame */
	sub	%fp, 96 + __z_arch_esf_t_SIZEOF, %sp
	/*
	 * %fp: %sp of interrupted task
	 * %sp: %sp of interrupted task - ABI_frame - esf
	 */

	/* Fill in the content of the exception stack frame */
	st	%l1, [%sp + 96 + __z_arch_esf_t_pc_OFFSET]
	st	%l2, [%sp + 96 + __z_arch_esf_t_npc_OFFSET]
	st	%l0, [%sp + 96 + __z_arch_esf_t_psr_OFFSET]
	st	%l6, [%sp + 96 + __z_arch_esf_t_tbr_OFFSET]
	st	%fp, [%sp + 96 + __z_arch_esf_t_sp_OFFSET]
	rd	%y, %g1
	st	%g1, [%sp + 96 + __z_arch_esf_t_y_OFFSET]

	/* Enable traps, raise PIL to mask all maskable interrupts. */
	or	%l0, PSR_PIL, %o0
	wr	%o0, PSR_ET, %psr
	nop
	nop
	nop
	/* Exception stack frame prepared earlier is the first argument. */
	call	_Fault
	 add	%sp, 96, %o0
