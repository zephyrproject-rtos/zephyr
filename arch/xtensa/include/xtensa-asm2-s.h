/*
 * Copyright (c) 2017, Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_XTENSA_INCLUDE_XTENSA_ASM2_S_H
#define ZEPHYR_ARCH_XTENSA_INCLUDE_XTENSA_ASM2_S_H

#include "xtensa-asm2-context.h"

/* Assembler header!  This file contains macros designed to be included
 * only by the assembler.
 */

/*
 * SPILL_ALL_WINDOWS
 *
 * Spills all windowed registers (i.e. registers not visible as
 * A0-A15) to their ABI-defined spill regions on the stack.
 *
 * Unlike the Xtensa HAL implementation, this code requires that the
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
	s32i a0, a1, BSA_SAR_OFF
#if XCHAL_HAVE_LOOPS
	rsr.LBEG a0
	s32i a0, a1, BSA_LBEG_OFF
	rsr.LEND a0
	s32i a0, a1, BSA_LEND_OFF
	rsr.LCOUNT a0
	s32i a0, a1, BSA_LCOUNT_OFF
#endif
	rsr.exccause a0
	s32i a0, a1, BSA_EXCCAUSE_OFF
#if XCHAL_HAVE_S32C1I
	rsr.SCOMPARE1 a0
	s32i a0, a1, BSA_SCOMPARE1_OFF
#endif
#if XCHAL_HAVE_THREADPTR && defined(CONFIG_THREAD_LOCAL_STORAGE)
	rur.THREADPTR a0
	s32i a0, a1, BSA_THREADPTR_OFF
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
 *    be spilled, because it already has been as part of the context
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
 * problem would be to use a SINGLE frame between the two stacks,
 * pre-spill it with one stack pointer for the "lower" call to see and
 * leave the register SP in place for the "upper" frame to use.
 * Would require modifying the Window{Over|Under}flow4 exceptions to
 * know not to spill/fill these special frames, but that's not too
 * hard, maybe...
 *
 * Enter this macro with a valid "context saved" pointer (i.e. SP
 * should point to a stored pointer which points to one BSA below the
 * interrupted/old stack) in A1, a handler function in A2, and a "new"
 * stack pointer (i.e. a pointer to the word ABOVE the allocated stack
 * area) in A3.  On return A0/1 will be unchanged, A2 has the return
 * value of the called function, and A3 is clobbered.  A4-A15 become
 * part of called frames and MUST NOT BE IN USE by the code that
 * expands this macro.  The called function gets the context save
 * handle in A1 as it's first argument.
 */
.macro CROSS_STACK_CALL
	mov a6, a3		/* place "new sp" in the next frame's A2 */
	mov a10, a1             /* pass "context handle" in 2nd frame's A2 */
	mov a3, a1		/* stash it locally in A3 too */
	mov a11, a2		/* handler in 2nd frame's A3, next frame's A7 */

	/* Recover the interrupted SP from the BSA */
	l32i a1, a1, 0
	l32i a0, a1, BSA_A0_OFF
	addi a1, a1, BASE_SAVE_AREA_SIZE

	call4 _xstack_call0_\@
	mov a1, a3		/* restore original SP */
	mov a2, a6		/* copy return value */
	j _xstack_returned_\@
.align 4
_xstack_call0_\@:
	/* We want an ENTRY to set a bit in windowstart and do the
	 * rotation, but we want our own SP
	 */
	entry a1, 16
	mov a1, a2
	call4 _xstack_call1_\@
	mov a2, a6		/* copy return value */
	retw
.align 4
_xstack_call1_\@:
	/* Remember the handler is going to do our ENTRY, so the
	 * handler pointer is still in A6 (not A2) even though this is
	 * after the second CALL4.
	 */
	jx a7
_xstack_returned_\@:
.endm

/* Entry setup for all exceptions and interrupts.  Arrive here with
 * the stack pointer decremented across a base save area, A0-A3 and
 * PS/PC already spilled to the stack in the BSA, and A2 containing a
 * level-specific C handler function.
 *
 * This is a macro (to allow for unit testing) that expands to a
 * handler body to which the vectors can jump.  It takes two static
 * (!) arguments: a special register name (which should be set up to
 * point to some kind of per-CPU record struct) and offsets within
 * that struct which contains an interrupt stack top and a "nest
 * count" word.
 */
.macro EXCINT_HANDLER SR, NEST_OFF, INTSTACK_OFF
	/* A2 contains our handler function which will get clobbered
	 * by the save.  Stash it into the unused "a1" slot in the
	 * BSA and recover it immediately after.  Kind of a hack.
	 */
	s32i a2, a1, BSA_SCRATCH_OFF

	ODD_REG_SAVE
	call0 xtensa_save_high_regs

	l32i a2, a1, 0
	l32i a2, a2, BSA_SCRATCH_OFF

	/* There's a gotcha with level 1 handlers: the INTLEVEL field
	 * gets left at zero and not set like high priority interrupts
	 * do.  That works fine for exceptions, but for L1 interrupts,
	 * when we unmask EXCM below, the CPU will just fire the
	 * interrupt again and get stuck in a loop blasting save
	 * frames down the stack to the bottom of memory.  It would be
	 * good to put this code into the L1 handler only, but there's
	 * not enough room in the vector without some work there to
	 * squash it some.  Next choice would be to make this a macro
	 * argument and expand two versions of this handler.  An
	 * optimization FIXME, I guess.
	 */
	rsr.PS a0
	movi a3, PS_INTLEVEL_MASK
	and a0, a0, a3
	bnez a0, _not_l1
	rsr.PS a0
	movi a3, PS_INTLEVEL(1)
	or a0, a0, a3
	wsr.PS a0
_not_l1:

	/* Unmask EXCM bit so C code can spill/fill in window
	 * exceptions.  Note interrupts are already fully masked by
	 * INTLEVEL, so this is safe.
	 */
	rsr.PS a0
	movi a3, ~(PS_EXCM_MASK)
	and a0, a0, a3
	wsr.PS a0
	rsync

	/* A1 already contains our saved stack, and A2 our handler.
	 * So all that's needed for CROSS_STACK_CALL is to put the
	 * "new" stack into A3.  This can be either a copy of A1 or an
	 * entirely new area depending on whether we find a 1 in our
	 * SR[off] macro argument.
	 */
	rsr.\SR a3
	l32i a0, a3, \NEST_OFF
	beqz a0, _switch_stacks_\@

	/* Use the same stack, just copy A1 to A3 after incrementing NEST */
	addi a0, a0, 1
	s32i a0, a3, \NEST_OFF
	mov a3, a1
	j _do_call_\@

_switch_stacks_\@:
	addi a0, a0, 1
	s32i a0, a3, \NEST_OFF
	l32i a3, a3, \INTSTACK_OFF

_do_call_\@:
	CROSS_STACK_CALL

	/* Mask interrupts (which have been unmasked during the handler
	 * execution) while we muck with the windows and decrement the nested
	 * count.  The restore will unmask them correctly.
	 */
	rsil a0, XCHAL_NMILEVEL

	/* Decrement nest count */
	rsr.\SR a3
	l32i a0, a3, \NEST_OFF
	addi a0, a0, -1
	s32i a0, a3, \NEST_OFF

	/* Last trick: the called function returned the "next" handle
	 * to restore to in A6 (the call4'd function's A2).  If this
	 * is not the same handle as we started with, we need to do a
	 * register spill before restoring, for obvious reasons.
	 * Remember to restore the A1 stack pointer as it existed at
	 * interrupt time so the caller of the interrupted function
	 * spills to the right place.
	 */
	beq a6, a1, _restore_\@
	l32i a1, a1, 0
	l32i a0, a1, BSA_A0_OFF
	addi a1, a1, BASE_SAVE_AREA_SIZE
#ifndef CONFIG_KERNEL_COHERENCE
	/* When using coherence, the registers of the interrupted
	 * context got spilled upstream in arch_cohere_stacks()
	 */
	SPILL_ALL_WINDOWS
#endif
	mov a1, a6

_restore_\@:
	j _restore_context
.endm

/* Defines an exception/interrupt vector for a specified level.  Saves
 * off the interrupted A0-A3 registers and the per-level PS/PC
 * registers to the stack before jumping to a handler (defined with
 * EXCINT_HANDLER) to do the rest of the work.
 *
 * Arguments are a numeric interrupt level and symbol names for the
 * entry code (defined via EXCINT_HANDLER) and a C handler for this
 * particular level.
 *
 * Note that the linker sections for some levels get special names for
 * no particularly good reason.  Only level 1 has any code generation
 * difference, because it is the legacy exception level that predates
 * the EPS/EPC registers.  It also lives in the "iram0.text" segment
 * (which is linked immediately after the vectors) so that an assembly
 * stub can be loaded into the vector area instead and reach this code
 * with a simple jump instruction.
 */
.macro DEF_EXCINT LVL, ENTRY_SYM, C_HANDLER_SYM
.if \LVL == 1
.pushsection .iram0.text, "ax"
.elseif \LVL == XCHAL_DEBUGLEVEL
.pushsection .DebugExceptionVector.text, "ax"
.elseif \LVL == XCHAL_NMILEVEL
.pushsection .NMIExceptionVector.text, "ax"
.else
.pushsection .Level\LVL\()InterruptVector.text, "ax"
.endif
.global _Level\LVL\()Vector
_Level\LVL\()Vector:
	addi a1, a1, -BASE_SAVE_AREA_SIZE
	s32i a0, a1, BSA_A0_OFF
	s32i a2, a1, BSA_A2_OFF
	s32i a3, a1, BSA_A3_OFF

	/* Level "1" is the exception handler, which uses a different
	 * calling convention.  No special register holds the
	 * interrupted PS, instead we just assume that the CPU has
	 * turned on the EXCM bit and set INTLEVEL.
	 */
.if \LVL == 1
	rsr.PS a0
	movi a2, ~(PS_EXCM_MASK | PS_INTLEVEL_MASK)
	and a0, a0, a2
	s32i a0, a1, BSA_PS_OFF
.else
	rsr.EPS\LVL a0
	s32i a0, a1, BSA_PS_OFF
.endif

	rsr.EPC\LVL a0
	s32i a0, a1, BSA_PC_OFF

	/* What's happening with this jump is that the L32R
	 * instruction to load a full 32 bit immediate must use an
	 * offset that is negative from PC.  Normally the assembler
	 * fixes this up for you by putting the "literal pool"
	 * somewhere at the start of the section.  But vectors start
	 * at a fixed address in their own section, and don't (in our
	 * current linker setup) have anywhere "definitely before
	 * vectors" to place immediates.  Some platforms and apps will
	 * link by dumb luck, others won't.  We add an extra jump just
	 * to clear space we know to be legal.
	 *
	 * The right way to fix this would be to use a "literal_prefix"
	 * to put the literals into a per-vector section, then link
	 * that section into the PREVIOUS vector's area right after
	 * the vector code.  Requires touching a lot of linker scripts
	 * though.
	 */
	j _after_imms\LVL\()
.align 4
_handle_excint_imm\LVL:
	.word \ENTRY_SYM
_c_handler_imm\LVL:
	.word \C_HANDLER_SYM
_after_imms\LVL:
	l32r a2, _c_handler_imm\LVL
	l32r a0, _handle_excint_imm\LVL
	jx a0
.popsection
.endm

#endif	/* ZEPHYR_ARCH_XTENSA_INCLUDE_XTENSA_ASM2_S_H */
