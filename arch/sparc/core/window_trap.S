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
 * This implementation uses the window overflow trap handler to perform the
 * actual window flush.
 *
 * On entry:
 * %l0: psr
 * %l1: pc
 * %l2: npc
 */
SECTION_FUNC(TEXT, __sparc_trap_flush_windows)
	/* push a few registers which are needed later to the stack */
	sub	%sp, 0x10, %sp
	std	%l0, [%sp + 0x40 + 0x00]
	st	%l2, [%sp + 0x40 + 0x08]
	st	%g2, [%sp + 0x40 + 0x0c]

	restore
	/* In window where we trapped from. This window will not be flushed. */

	/* Set highest processor interrupt level and enable traps. */
	rd	%psr, %g2
	or	%g2, PSR_PIL, %g2
	wr	%g2, PSR_ET, %psr
	nop
	nop

	/* Execute "save" NWINDOWS-1 times. */
	set     CONFIG_SPARC_NWIN-2, %g2
1:
	save
	cmp	%g2, %g0
	bne	1b
	 sub	%g2, 1, %g2

	/* Execute "restore" NWINDOWS-1 times. */
	set     CONFIG_SPARC_NWIN-2, %g2
2:
	restore
	cmp	%g2, %g0
	bne	2b
	 sub	%g2, 1, %g2

	save

	/* pop registers from stack which are used for the trap return */
	ldd	[%sp + 0x40 + 0x00], %l0
	ld	[%sp + 0x40 + 0x08], %l2
	ld	[%sp + 0x40 + 0x0c], %g2
	add	%sp, 0x10, %sp

	/* Restore %psr as it was on trap entry. */
	wr	%l0, %psr
	nop
	nop
	nop

	jmp	%l2
	 rett	%l2 + 4
