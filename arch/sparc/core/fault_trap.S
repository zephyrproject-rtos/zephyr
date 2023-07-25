/*
 * Copyright (c) 2019-2020 Cobham Gaisler AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <offsets_short.h>
#include <zephyr/arch/sparc/sparc.h>

GTEXT(__sparc_trap_except_reason)
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
 * %g1: reason
 *
 * This trap handler will trash some of the global registers, which is OK since
 * we will not return to where we trapped.
 */
SECTION_FUNC(TEXT, __sparc_trap_except_reason)
	mov	%g1, %l7
.Ldoit:
	/* %g2, %g3 are used at manual window overflow so save temporarily */
	mov	%g2, %l4
	mov	%g3, %l5

	/* We may have trapped into the invalid window. If so, make it valid. */
	rd	%wim, %g2
	mov	%g2, %l3
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
	mov	%l4, %g2
	mov	%l5, %g3

	/* Allocate an ABI stack frame and exception stack frame */
	sub	%fp, 96 + __z_arch_esf_t_SIZEOF, %sp
	/*
	 * %fp: %sp of interrupted task
	 * %sp: %sp of interrupted task - ABI_frame - esf
	 */

	mov	%l7, %o0
	/* Fill in the content of the exception stack frame */
#if defined(CONFIG_EXTRA_EXCEPTION_INFO)
	std	%i0, [%sp + 96 + __z_arch_esf_t_out_OFFSET + 0x00]
	std	%i2, [%sp + 96 + __z_arch_esf_t_out_OFFSET + 0x08]
	std	%i4, [%sp + 96 + __z_arch_esf_t_out_OFFSET + 0x10]
	std	%i6, [%sp + 96 + __z_arch_esf_t_out_OFFSET + 0x18]
	std	%g0, [%sp + 96 + __z_arch_esf_t_global_OFFSET + 0x00]
	std	%g2, [%sp + 96 + __z_arch_esf_t_global_OFFSET + 0x08]
	std	%g4, [%sp + 96 + __z_arch_esf_t_global_OFFSET + 0x10]
	std	%g6, [%sp + 96 + __z_arch_esf_t_global_OFFSET + 0x18]
#endif
	std	%l0, [%sp + 96 + __z_arch_esf_t_psr_OFFSET] /* psr pc */
	std	%l2, [%sp + 96 + __z_arch_esf_t_npc_OFFSET] /* npc wim */
	rd	%y, %l7
	std	%l6, [%sp + 96 + __z_arch_esf_t_tbr_OFFSET] /* tbr y */

	/* Enable traps, raise PIL to mask all maskable interrupts. */
	or	%l0, PSR_PIL, %o2
	wr	%o2, PSR_ET, %psr
	nop
	nop
	nop

#if defined(CONFIG_EXTRA_EXCEPTION_INFO)
	/* Flush all register windows to the stack. */
	.rept CONFIG_SPARC_NWIN-1
	save	%sp, -64, %sp
	.endr
	.rept CONFIG_SPARC_NWIN-1
	restore
	.endr
#endif

	/*
	 * reason is the first argument.
	 * Exception stack frame prepared earlier is the second argument.
	 */
	call	z_sparc_fatal_error
	 add	%sp, 96, %o1


/*
 * Entry for trap we don't handle explicitly
 *
 * Just drop into __sparc_trap_except_reason with reason set to
 * K_ERR_CPU_EXCEPTION. Note that "reason" is transported in %l7 of the
 * trapped-into window and global %g1 is preserved.
 */
SECTION_FUNC(TEXT, __sparc_trap_fault)
	b	.Ldoit
	/* K_ERR_CPU_EXCEPTION */
	 mov	%g0, %l7
