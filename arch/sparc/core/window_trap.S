/*
 * Copyright (c) 2019-2020 Cobham Gaisler AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This file contains standard handlers for the SPARC V8 window overflow and
 * underflow traps.
 */

#include <toolchain.h>
#include <linker/sections.h>

GTEXT(__sparc_trap_window_overflow)
GTEXT(__sparc_trap_window_underflow)

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
