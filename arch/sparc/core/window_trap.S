/*
 * Copyright (c) 2019-2020 Cobham Gaisler AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This file contains standard handlers for the SPARC V8 window overflow and
 * underflow traps. It also implements the handler for SPARC-ABI
 * "Flush windows" which is used for example by longjmp() and C++ exceptions.
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/arch/sparc/sparc.h>

GTEXT(__sparc_trap_window_overflow)
GTEXT(__sparc_trap_window_underflow)
GTEXT(__sparc_trap_flush_windows)

SECTION_FUNC(TEXT, __sparc_trap_window_overflow)
	/* Enter the window to be stored. */
	save
	/* Save local register set. */
	std	%l0, [%sp + 0x00]
	std	%l2, [%sp + 0x08]
	std	%l4, [%sp + 0x10]
	rd	%wim, %l3
	std	%l6, [%sp + 0x18]
	/* l2 := WIM << (NWIN-1) */
	sll	%l3, (CONFIG_SPARC_NWIN-1), %l2
	/* Save input register set. */
	std	%i0, [%sp + 0x20]
	/* l3 := WIM >> 1 */
	srl	%l3, 1, %l3
	std	%i2, [%sp + 0x28]
	/* WIM := (WIM >> 1) ^ (WIM << (NWIN-1)) */
	wr	%l3, %l2, %wim
	/* NOTE: 3 instruction before restore (delayed write instruction) */
	std	%i4, [%sp + 0x30]
	nop
	std	%i6, [%sp + 0x38]
	/* Go back to trap window. */
	restore
	/* Re-execute save. */
	jmp	%l1
	 rett	%l2

SECTION_FUNC(TEXT, __sparc_trap_window_underflow)
	rd	%wim, %l3
	/* l4 := WIM << 1 */
	sll	%l3, 1, %l4
	/* l5 := WIM >> (NWIN-1) */
	srl	%l3, (CONFIG_SPARC_NWIN-1), %l5
	/* WIM := (WIM << 1) ^ (WIM >> (NWIN-1)) */
	wr	%l4, %l5, %wim
	/* WIM is implicitly read so nops are needed. */
	nop
	nop
	nop

	/* Enter the window to restore requires two restore instructions. */
	restore
	restore
	ldd	[%sp + 0x00], %l0
	ldd	[%sp + 0x08], %l2
	ldd	[%sp + 0x10], %l4
	ldd	[%sp + 0x18], %l6
	ldd	[%sp + 0x20], %i0
	ldd	[%sp + 0x28], %i2
	ldd	[%sp + 0x30], %i4
	ldd	[%sp + 0x38], %i6
	/* Go back to the trap window. */
	save
	save
	/* Re-execute restore. */
	jmp	%l1
	 rett	%l2

/*
 * Handler for SPARC trap 0x83: trap_instruction, defined as "Flush windows" by
 * SPARC-ABI:
 *   "By executing a type 3 trap, a process asks the system to flush all its
 *   register windows to the stack."
 *
 * On entry:
 * %l0: psr
 * %l1: pc
 * %l2: npc
 */
SECTION_FUNC(TEXT, __sparc_trap_flush_windows)
	/* Save global registers used by the routine */
	mov	%g3, %l3
	mov	%g4, %l4
	mov	%g5, %l5
	mov	%g1, %l6
	mov	%g2, %l7

	/* Uses g3=psr, g4=1, g2=wim, g1,g5=scratch */
	mov	%l0, %g3
	set	1, %g4
	rd	%wim, %g2

	/*
	 * We can always restore the previous window. Check if we can restore
	 * the window after that.
	 */
	and	%l0, PSR_CWP, %g1
	add	%g1, 2, %g1
	ba	.LcheckNextWindow
	 restore

	/* Flush window to stack */
.LflushWindow:
	std	%l0, [%sp + 0x00]
	std	%l2, [%sp + 0x08]
	std	%l4, [%sp + 0x10]
	std	%l6, [%sp + 0x18]
	std	%i0, [%sp + 0x20]
	std	%i2, [%sp + 0x28]
	std	%i4, [%sp + 0x30]
	std	%i6, [%sp + 0x38]

	/*
	 * Check if next window is invalid by comparing
	 * (1 << ((cwp + 1) % NWIN)) with WIM
	 */
.LcheckNextWindow:
	set	CONFIG_SPARC_NWIN, %g5
	cmp	%g1, %g5
	bge,a	.Lnowrap
	 sub	%g1, %g5, %g1
.Lnowrap:
	sll	%g4, %g1, %g5
	cmp	%g5, %g2
	be	.LflushWindowDone
	 inc	%g1

	/* We need to flush the next window */
	ba	.LflushWindow
	 restore

	/*
	 * All used windows have been flushed. Set WIM to cause trap for CWP+2.
	 * When we return from this trap it will be CWP+1 that will trap, that
	 * is, the next restore or rett.
	 */
.LflushWindowDone:
	/* We can not restore %psr from %l0 because we may be in any window. */
	wr	%g3, %psr
	and	%g3, PSR_CWP, %g1
	add	%g1, 2, %g1
	set	CONFIG_SPARC_NWIN, %g5
	/* We are now back in the trap window. */
	cmp	%g1, %g5
	bge,a	.Lnowrap2
	 sub	%g1, %g5, %g1
.Lnowrap2:
	sll	%g4, %g1, %g1
	wr	%g1, %wim
	mov	%l3, %g3
	mov	%l4, %g4
	mov	%l5, %g5
	mov	%l6, %g1
	mov	%l7, %g2
	jmp	%l2
	 rett	%l2 + 4
