/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>

_ASM_FILE_PROLOGUE

GTEXT(soc_clock_call_on_stack)

/*
 * void soc_clock_call_on_stack(void (*fn)(void), void *stack_top);
 *
 * Call fn() with the stack pointer moved to stack_top, then restore the caller's
 * stack and return. soc_clock.c uses it to run the crystal hand-over window on a
 * DTCM scratch stack: the window parks XSPI1, so the PSRAM the running stack
 * normally lives in is unreachable for as long as the window is open.
 *
 * Plain assembly rather than a naked C function with inline asm: the compiler
 * gets no opportunity to emit a prologue, spill a local or address anything
 * relative to the stack pointer being moved, and the ITCM placement cannot be
 * defeated by inlining.
 *
 * fn and stack_top arrive in r0 and r1 by AAPCS. The push/pop pair uses the
 * caller's stack, which is live both before the window opens and after it
 * closes; only the span between them runs on stack_top. r4 is callee-saved, so
 * fn() preserves the caller's stack pointer held there across the call.
 *
 * stack_top must be 8-byte aligned, as AAPCS requires at a public interface.
 * PSPLIM, where the build has it, must be released by the caller: the scratch
 * stack sits below the limit the current stack was given.
 */
SECTION_FUNC(itcm, soc_clock_call_on_stack)
	push	{r4, lr}
	mov	r4, sp
	mov	sp, r1
	blx	r0
	mov	sp, r4
	pop	{r4, pc}
