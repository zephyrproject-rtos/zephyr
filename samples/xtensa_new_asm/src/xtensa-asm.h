/*
 * Copyright (c) 2017, Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <xtensa/config/core-isa.h>

#if XCHAL_HAVE_LOOPS
#define BASE_SAVE_AREA_SIZE 56
#else
#define BASE_SAVE_AREA_SIZE 44
#endif

/*
 * SPILL_ALL_WINDOWS
 *
 * Spills all windowed registers (i.e. registers not visible as
 * A0-A15) to their ABI-defined spill regions on the stack.
 *
 * Unlike the Xtensa HAL implementaiton, this code requires that the
 * EXCM and WOE bit be enabled in PS, and relies on repeated hardware
 * exception handling to do the register spills.  The trick is to do a
 * noop write to the high registers, which the hardware will trap
 * (into an overflow exception) in the case where those registers are
 * already used by an existing call frame.  Then it rotates the window
 * and repeats until all but the A0-A3 registers of the original frame
 * are guaranteed to be spilled, eventually rotating back around into
 * the original frame.  Advantages:
 *
 * - Vastly smaller code size
 *
 * - More easily maintained if changes are needed to window over/underflow
 *   exception handling.
 *
 * - Requires no scratch registers to do its work, so can be used safely in any
 *   context.
 *
 * - If the WOE bit is not enabled (for example, in code written for
 *   the CALL0 ABI), this becomes a silent noop and operates compatbily.
 *
 * - In memory protection situations, this relies on the existing
 *   exception handlers (and thus their use of the L/S32E
 *   instructions) to execute stores in the protected space.  AFAICT,
 *   the HAL routine does not handle this situation and isn't safe: it
 *   will happily write through the "stack pointers" found in
 *   registers regardless of where they might point.
 *
 * - Hilariously it's ACTUALLY FASTER than the HAL routine.  And not
 *   just a little bit, it's MUCH faster.  With a mostly full register
 *   file on an LX6 core (ESP-32) I'm measuring 145 cycles to spill
 *   registers with this vs. 279 (!) to do it with
 *   xthal_spill_windows().  Apparently Xtensa exception handling is
 *   really fast, and no one told their software people.
 *
 * Note that as with the Xtensa HAL spill routine, and unlike context
 * switching code on most sane architectures, the intermediate states
 * here will have an invalid stack pointer.  That means that this code
 * must not be preempted in any context (i.e. all Zephyr situations)
 * where the interrupt code will need to use the stack to save the
 * context.  But unlike the HAL, which runs with exceptions masked via
 * EXCM, this will not: hit needs the overflow handlers unmasked.  Use
 * INTLEVEL instead (which, happily, is what Zephyr's locking does
 * anyway).
 */
.macro SPILL_ALL_WINDOWS
#if XCHAL_NUM_AREGS == 64
	and a12, a12, a12
	rotw 3
	and a12, a12, a12
	rotw 3
	and a12, a12, a12
	rotw 3
	and a12, a12, a12
	rotw 3
	and a12, a12, a12
	rotw 4
#elif XCHAL_NUM_AREGS == 32
	and a12, a12, a12
	rotw 3
	and a12, a12, a12
	rotw 3
	and a4, a4, a4
	rotw 2
#else
#error Unrecognized XCHAL_NUM_AREGS
#endif
.endm

/*
 * ODD_REG_SAVE
 *
 * Stashes the oddball shift/loop context registers in the base save
 * area pointed to by the current stack pointer.  On exit, A0 will
 * have been modified but A2/A3 have not, and the shift/loop
 * instructions can be used freely (though note loops don't work in
 * exceptions for other reasons!).
 *
 * Does not populate or modify the PS/PC save locations.
 */
.macro ODD_REG_SAVE
	rsr.SAR a0
	s32i a0, a1, BASE_SAVE_AREA_SIZE-44
#if XCHAL_HAVE_LOOPS
	rsr.LBEG a0
	s32i a0, a1, BASE_SAVE_AREA_SIZE-48
	rsr.LEND a0
	s32i a0, a1, BASE_SAVE_AREA_SIZE-52
	rsr.LCOUNT a0
	s32i a0, a1, BASE_SAVE_AREA_SIZE-56
#endif
.endm

/*
 * CROSS_STACK_CALL
 *
 * Sets the stack up carefully such that a "cross stack" call can spill
 * correctly, then invokes an immediate handler.  Note that:
 *
 * 0. When spilling a frame, functions find their callEE's stack pointer
 *    (to save A0-A3) from registers.  But they find their
 *    already-spilled callER's stack pointer (to save higher GPRs) from
 *    their own stack memory.
 *
 * 1. The function that was interrupted ("interruptee") does not need to
 *    be spilled, becuase it already has been as part of the context
 *    save.  So it doesn't need registers allocated for it anywhere.
 *
 * 2. Interruptee's caller needs to spill into the space below the
 *    interrupted stack frame, which means that the A1 register it finds
 *    below it needs to contain the old/interrupted stack and not the
 *    context saved one.
 *
 * 3. The ISR dispatcher (called "underneath" interruptee) needs to spill
 *    high registers into the space immediately above its own stack frame,
 *    so it needs to find a caller with the "new" stack pointer instead.
 *
 * We make this work by inserting TWO 4-register frames between
 * "interruptee's caller" and "ISR dispatcher".  The top one (which
 * occupies the slot formerly held by "interruptee", whose registers
 * were saved via external means) holds the "interrupted A1" and the
 * bottom has the "top of the interrupt stack" which can be either the
 * word above a new memory area (when handling an interrupt from user
 * mode) OR the existing "post-context-save" stack pointer (when
 * handling a nested interrupt).  The code works either way.  Because
 * these are both only 4-registers, neither needs its own caller for
 * spilling.
 *
 * The net cost is 32 wasted bytes on the interrupt stack frame to
 * spill our two "phantom frames" (actually not quite, as we'd need a
 * few of those words used somewhere for tracking the stack pointers
 * anyway).  But the benefit is that NO REGISTER FRAMES NEED TO BE
 * SPILLED on interrupt entry.  And if we return back into the same
 * context we interrupted (a common case) no windows need to be
 * explicitly spilled at all.  And in fact in the case where the ISR
 * uses significant depth on its own stack, the interrupted frames
 * will be spilled naturally as a standard cost of a function call,
 * giving register windows something like "zero cost interrupts".
 *
 * FIXME: a terrible awful really nifty idea to fix the stack waste
 * problem would be to have a special version of the window overflow
 * handlers (e.g. indirect to a different handler when EXCM is set or
 * somesuch) that could understand somehow that the two special frames
 * did not need to be spilled, then put their "spill" location "above"
 * the new stack which will then never be touched.  We would need a
 * few words stored somewhere (per-cpu) to manage the tracking.  Maybe
 * too clever, but worth investigation.
 *
 * Enter this macro with a valid "bottom of stack" pointer (in the
 * sense of "no data underneath it will be used" -- a stack pointer in
 * use in the normal ABI does NOT MEET THIS REQUIREMENT, the intent is
 * to pass a context-saved handle here) in A1, an "old" stack pointer
 * to use as a caller spill top in A2, and a "new" stack pointer in
 * A3.  On return A0/A1/A2 will be unchanged.  A4-A15 become part of
 * called frames and must not be in use by the code that expands this macro.
 */
.macro CROSS_STACK_CALL handler_imm
	mov a6, a3		// place "new sp" in the next frame's A2
	mov a7, a1              // stash "true stack" in A7 (next frame's A3)
	mov a1, a2              // current frame gets "old sp"
	call4 _xstack_call0
	mov a1, a7		// restore original SP
	j _xstack_returned
.align 4
_xstack_call0:
	// We want an ENTRY to set a bit in windowstart and do the
	// rotation, but we want our own SP
	entry a1, 16
	mov a1, a2
	call4 _xstack_call1
	retw
.align 4
_xstack_call1:
	// Let the handler do the ENTRY
	j \handler_imm
_xstack_returned:
.endm
