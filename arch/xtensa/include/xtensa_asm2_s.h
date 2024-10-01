/*
 * Copyright (c) 2017, Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_XTENSA_INCLUDE_XTENSA_ASM2_S_H
#define ZEPHYR_ARCH_XTENSA_INCLUDE_XTENSA_ASM2_S_H

#include <zephyr/zsr.h>
#include "xtensa_asm2_context.h"

#include <zephyr/offsets.h>

/* Assembler header!  This file contains macros designed to be included
 * only by the assembler.
 */

#if defined(CONFIG_XTENSA_HIFI_SHARING)
.extern _xtensa_hifi_save
#endif

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
 *   the CALL0 ABI), this becomes a silent noop and operates compatibly.
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

#if XCHAL_HAVE_FP && defined(CONFIG_CPU_HAS_FPU) && defined(CONFIG_FPU_SHARING)
/*
 * FPU_REG_SAVE
 *
 * Saves the Float Point Unit context registers in the base save
 * area pointed to by the current stack pointer A1. The Floating-Point
 * Coprocessor Option adds the FR register file and two User Registers
 * called FCR and FSR.The FR register file consists of 16 registers of
 * 32 bits each and is used for all data computation.
 */
.macro FPU_REG_SAVE
	rur.fcr	a0
	s32i	a0, a1, ___xtensa_irq_bsa_t_fcr_OFFSET
	rur.fsr	a0
	s32i	a0, a1, ___xtensa_irq_bsa_t_fsr_OFFSET
	ssi	f0, a1, ___xtensa_irq_bsa_t_fpu0_OFFSET
	ssi	f1, a1, ___xtensa_irq_bsa_t_fpu1_OFFSET
	ssi	f2, a1, ___xtensa_irq_bsa_t_fpu2_OFFSET
	ssi	f3, a1, ___xtensa_irq_bsa_t_fpu3_OFFSET
	ssi	f4, a1, ___xtensa_irq_bsa_t_fpu4_OFFSET
	ssi	f5, a1, ___xtensa_irq_bsa_t_fpu5_OFFSET
	ssi	f6, a1, ___xtensa_irq_bsa_t_fpu6_OFFSET
	ssi	f7, a1, ___xtensa_irq_bsa_t_fpu7_OFFSET
	ssi	f8, a1, ___xtensa_irq_bsa_t_fpu8_OFFSET
	ssi	f9, a1, ___xtensa_irq_bsa_t_fpu9_OFFSET
	ssi	f10, a1, ___xtensa_irq_bsa_t_fpu10_OFFSET
	ssi	f11, a1, ___xtensa_irq_bsa_t_fpu11_OFFSET
	ssi	f12, a1, ___xtensa_irq_bsa_t_fpu12_OFFSET
	ssi	f13, a1, ___xtensa_irq_bsa_t_fpu13_OFFSET
	ssi	f14, a1, ___xtensa_irq_bsa_t_fpu14_OFFSET
	ssi	f15, a1, ___xtensa_irq_bsa_t_fpu15_OFFSET
.endm

.macro FPU_REG_RESTORE
	l32i.n	a0, a1, ___xtensa_irq_bsa_t_fcr_OFFSET
	wur.fcr	a0
	l32i.n	a0, a1, ___xtensa_irq_bsa_t_fsr_OFFSET
	wur.fsr	a0
	lsi	f0, a1, ___xtensa_irq_bsa_t_fpu0_OFFSET
	lsi	f1, a1, ___xtensa_irq_bsa_t_fpu1_OFFSET
	lsi	f2, a1, ___xtensa_irq_bsa_t_fpu2_OFFSET
	lsi	f3, a1, ___xtensa_irq_bsa_t_fpu3_OFFSET
	lsi	f4, a1, ___xtensa_irq_bsa_t_fpu4_OFFSET
	lsi	f5, a1, ___xtensa_irq_bsa_t_fpu5_OFFSET
	lsi	f6, a1, ___xtensa_irq_bsa_t_fpu6_OFFSET
	lsi	f7, a1, ___xtensa_irq_bsa_t_fpu7_OFFSET
	lsi	f8, a1, ___xtensa_irq_bsa_t_fpu8_OFFSET
	lsi	f9, a1, ___xtensa_irq_bsa_t_fpu9_OFFSET
	lsi	f10, a1, ___xtensa_irq_bsa_t_fpu10_OFFSET
	lsi	f11, a1, ___xtensa_irq_bsa_t_fpu11_OFFSET
	lsi	f12, a1, ___xtensa_irq_bsa_t_fpu12_OFFSET
	lsi	f13, a1, ___xtensa_irq_bsa_t_fpu13_OFFSET
	lsi	f14, a1, ___xtensa_irq_bsa_t_fpu14_OFFSET
	lsi	f15, a1, ___xtensa_irq_bsa_t_fpu15_OFFSET
.endm
#endif

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
	rsr.sar a0
	s32i a0, a1, ___xtensa_irq_bsa_t_sar_OFFSET
#if XCHAL_HAVE_LOOPS
	rsr.lbeg a0
	s32i a0, a1, ___xtensa_irq_bsa_t_lbeg_OFFSET
	rsr.lend a0
	s32i a0, a1, ___xtensa_irq_bsa_t_lend_OFFSET
	rsr.lcount a0
	s32i a0, a1, ___xtensa_irq_bsa_t_lcount_OFFSET
#endif
	rsr.exccause a0
	s32i a0, a1, ___xtensa_irq_bsa_t_exccause_OFFSET
#if XCHAL_HAVE_S32C1I
	rsr.scompare1 a0
	s32i a0, a1, ___xtensa_irq_bsa_t_scompare1_OFFSET
#endif
#if XCHAL_HAVE_THREADPTR && \
	(defined(CONFIG_USERSPACE) || defined(CONFIG_THREAD_LOCAL_STORAGE))
	rur.THREADPTR a0
	s32i a0, a1, ___xtensa_irq_bsa_t_threadptr_OFFSET
#endif
#if XCHAL_HAVE_FP && defined(CONFIG_CPU_HAS_FPU) && defined(CONFIG_FPU_SHARING)
	FPU_REG_SAVE
#endif

.endm

#ifdef CONFIG_XTENSA_MMU
/*
 * CALC_PTEVADDR_BASE
 *
 * This calculates the virtual address of the first PTE page
 * (PTEVADDR base, the one mapping 0x00000000) so that we can
 * use this to obtain the virtual address of the PTE page we are
 * interested in. This can be obtained via
 * (1 << CONFIG_XTENSA_MMU_PTEVADDR_SHIFT).
 *
 * Note that this is done this way is to avoid any TLB
 * miss if we are to use l32r to load the PTEVADDR base.
 * If the page containing the PTEVADDR base address is
 * not in TLB, we will need to handle the TLB miss which
 * we are trying to avoid here.
 *
 * @param ADDR_REG Register to store the calculated
 *                 PTEVADDR base address.
 *
 * @note The content of ADDR_REG will be modified.
 *       Save and restore it around this macro usage.
 */
.macro CALC_PTEVADDR_BASE ADDR_REG
	movi \ADDR_REG, 1
	slli \ADDR_REG, \ADDR_REG, CONFIG_XTENSA_MMU_PTEVADDR_SHIFT
.endm

/*
 * PRELOAD_PTEVADDR
 *
 * This preloads the page table entries for a 4MB region to avoid TLB
 * misses. This 4MB region is mapped via a page (4KB) of page table
 * entries (PTE). Each entry is 4 bytes mapping a 4KB region. Each page,
 * then, has 1024 entries mapping a 4MB region. Filling TLB entries is
 * automatically done via hardware, as long as the PTE page associated
 * with a particular address is also in TLB. If the PTE page is not in
 * TLB, an exception will be raised that must be handled. This TLB miss
 * is problematic when we are in the middle of dealing with another
 * exception or handling an interrupt. So we need to put the PTE page
 * into TLB by simply do a load operation.
 *
 * @param ADDR_REG Register containing the target address
 * @param PTEVADDR_BASE_REG Register containing the PTEVADDR base
 *
 * @note Both the content of ADDR_REG will be modified.
 *       Save and restore it around this macro usage.
 */
.macro PRELOAD_PTEVADDR ADDR_REG, PTEVADDR_BASE_REG
	/*
	 * Calculate the offset to first PTE page of all memory.
	 *
	 * Every page (4KB) of page table entries contains
	 * 1024 entires (as each entry is 4 bytes). Each entry
	 * maps one 4KB page. So one page of entries maps 4MB of
	 * memory.
	 *
	 * 1. We need to find the virtual address of the PTE page
	 *    having the page table entry mapping the address in
	 *    register ADDR_REG. To do this, we first need to find
	 *    the offset of this PTE page from the first PTE page
	 *    (the one mapping memory 0x00000000):
	 *    a. Find the beginning address of the 4KB page
	 *       containing address in ADDR_REG. This can simply
	 *       be done by discarding 11 bits (or shifting right
	 *	 and then left 12 bits).
	 *    b. Since each PTE page contains 1024 entries,
	 *	 we divide the address obtained in step (a) by
	 *       further dividing it by 1024 (shifting right and
	 *       then left 10 bits) to obtain the offset of
	 *       the PTE page.
	 *
	 *    Step (a) and (b) can be obtained together so that
	 *    we can shift right 22 bits, and then shift left
	 *    12 bits.
	 *
	 * 2. Once we have combine the results from step (1) and
	 *    PTEVADDR_BASE_REG to get the virtual address of
	 *    the PTE page.
	 *
	 * 3. Do a l32i to force the PTE page to be in TLB.
	 */

	/* Step 1 */
	srli \ADDR_REG, \ADDR_REG, 22
	slli \ADDR_REG, \ADDR_REG, 12

	/* Step 2 */
	add \ADDR_REG, \ADDR_REG, \PTEVADDR_BASE_REG

	/* Step 3 */
	l32i \ADDR_REG, \ADDR_REG, 0
.endm
#endif /* CONFIG_XTENSA_MMU */

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
 * area) in A3.  Exceptions should be enabled via PS.EXCM, but
 * PS.INTLEVEL must (!) be set such that no nested interrupts can
 * arrive (we restore the natural INTLEVEL from the value in ZSR_EPS
 * just before entering the call).  On return A0/1 will be unchanged,
 * A2 has the return value of the called function, and A3 is
 * clobbered.  A4-A15 become part of called frames and MUST NOT BE IN
 * USE by the code that expands this macro.  The called function gets
 * the context save handle in A1 as it's first argument.
 */
.macro CROSS_STACK_CALL
	mov a6, a3		/* place "new sp" in the next frame's A2 */
	mov a10, a1             /* pass "context handle" in 2nd frame's A2 */
	mov a3, a1		/* stash it locally in A3 too */
	mov a11, a2		/* handler in 2nd frame's A3, next frame's A7 */

	/* Recover the interrupted SP from the BSA */
	l32i a1, a1, 0
	l32i a0, a1, ___xtensa_irq_bsa_t_a0_OFFSET
	addi a1, a1, ___xtensa_irq_bsa_t_SIZEOF

	call4 _xstack_call0_\@
	mov a1, a3		/* restore original SP */
	mov a2, a6		/* copy return value */
	j _xstack_returned_\@
.align 4
_xstack_call0_\@:
	/* We want an ENTRY to set a bit in windowstart and do the
	 * rotation, but we want our own SP.  After that, we are
	 * running in a valid frame, so re-enable interrupts.
	 */
	entry a1, 16
	mov a1, a2
	rsr.ZSR_EPS a2
	wsr.ps a2
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
.macro EXCINT_HANDLER NEST_OFF, INTSTACK_OFF
	/* A2 contains our handler function which will get clobbered
	 * by the save.  Stash it into the unused "a1" slot in the
	 * BSA and recover it immediately after.  Kind of a hack.
	 */
	s32i a2, a1, ___xtensa_irq_bsa_t_scratch_OFFSET

	ODD_REG_SAVE

#if defined(CONFIG_XTENSA_HIFI_SHARING)
	call0 _xtensa_hifi_save    /* Save HiFi registers */
#endif

	call0 xtensa_save_high_regs

	l32i a2, a1, 0
	l32i a2, a2, ___xtensa_irq_bsa_t_scratch_OFFSET

#if XCHAL_HAVE_THREADPTR && defined(CONFIG_USERSPACE)
	/* Clear up the threadptr because it is used
	 * to check if a thread is runnig on user mode. Since
	 * we are in a interruption we don't want the system
	 * thinking it is possbly running in user mode.
	 */
	movi.n a0, 0
	wur.THREADPTR a0
#endif /* XCHAL_HAVE_THREADPTR && CONFIG_USERSPACE */

#ifdef CONFIG_XTENSA_INTERRUPT_NONPREEMPTABLE

	/* Setting the interrupt mask to the max non-debug level
	 * to prevent lower priority interrupts being preempted by
	 * high level interrupts until processing of that lower level
	 * interrupt has completed.
	 */
	rsr.ps a0
	movi a3, ~(PS_INTLEVEL_MASK)
	and a0, a0, a3
	movi a3, PS_INTLEVEL(ZSR_RFI_LEVEL)
	or a0, a0, a3
	wsr.ps a0

#else

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
	rsr.ps a0
	movi a3, PS_INTLEVEL_MASK
	and a0, a0, a3
	bnez a0, _not_l1
	rsr.ps a0
	movi a3, PS_INTLEVEL(1)
	or a0, a0, a3
	wsr.ps a0

_not_l1:
#endif /* CONFIG_XTENSA_INTERRUPT_NONPREEMPTABLE */

	/* Setting up the cross stack call below has states where the
	 * resulting frames are invalid/non-reentrant, so we can't
	 * allow nested interrupts.  But we do need EXCM unmasked, as
	 * we use CALL/ENTRY instructions in the process and need to
	 * handle exceptions to spill caller/interruptee frames.  Use
	 * PS.INTLEVEL at maximum to mask all interrupts and stash the
	 * current value in our designated EPS register (which is
	 * guaranteed unused across the call)
	 */
	rsil a0, 0xf

	/* Since we are unmasking EXCM, we need to set RING bits to kernel
	 * mode, otherwise we won't be able to run the exception handler in C.
	 */
	movi a3, ~(PS_EXCM_MASK) & ~(PS_RING_MASK)
	and a0, a0, a3
	wsr.ZSR_EPS a0
	wsr.ps a0
	rsync

	/* A1 already contains our saved stack, and A2 our handler.
	 * So all that's needed for CROSS_STACK_CALL is to put the
	 * "new" stack into A3.  This can be either a copy of A1 or an
	 * entirely new area depending on whether we find a 1 in our
	 * SR[off] macro argument.
	 */
	rsr.ZSR_CPU a3
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
	rsil a0, XCHAL_NUM_INTLEVELS

	/* Decrement nest count */
	rsr.ZSR_CPU a3
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

#ifndef CONFIG_USERSPACE
	l32i a1, a1, 0
	l32i a0, a1, ___xtensa_irq_bsa_t_a0_OFFSET
	addi a1, a1, ___xtensa_irq_bsa_t_SIZEOF
#ifndef CONFIG_KERNEL_COHERENCE
	/* When using coherence, the registers of the interrupted
	 * context got spilled upstream in arch_cohere_stacks()
	 */
	SPILL_ALL_WINDOWS
#endif

	/* Restore A1 stack pointer from "next" handle. */
	mov a1, a6
#else
	/* With userspace, we cannot simply restore A1 stack pointer
	 * at this pointer because we need to swap page tables to
	 * the incoming thread, and we do not want to call that
	 * function with thread's stack. So we stash the new stack
	 * pointer into A2 first, then move it to A1 after we have
	 * swapped the page table.
	 */
	mov a2, a6

	/* Need to switch page tables because the "next" handle
	 * returned above is not the same handle as we started
	 * with. This means we are being restored to another
	 * thread.
	 */
	rsr a6, ZSR_CPU
	l32i a6, a6, ___cpu_t_current_OFFSET

#ifdef CONFIG_XTENSA_MMU
	call4 xtensa_swap_update_page_tables
#endif
#ifdef CONFIG_XTENSA_MPU
	call4 xtensa_mpu_map_write
#endif
	l32i a1, a1, 0
	l32i a0, a1, ___xtensa_irq_bsa_t_a0_OFFSET
	addi a1, a1, ___xtensa_irq_bsa_t_SIZEOF

	SPILL_ALL_WINDOWS

	/* Moved stashed stack pointer to A1 to restore stack. */
	mov a1, a2
#endif

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
#if defined(CONFIG_XTENSA_SMALL_VECTOR_TABLE_ENTRY)
.pushsection .iram.text, "ax"
.global _Level\LVL\()VectorHelper
_Level\LVL\()VectorHelper :
#else
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
#endif

#ifdef CONFIG_XTENSA_MMU
.if \LVL == 1
	/* If there are any TLB misses during interrupt handling,
	 * the user/kernel/double exception vector will be triggered
	 * to handle these misses. This results in DEPC and EXCCAUSE
	 * being overwritten, and then execution returned back to
	 * this site of TLB misses. When it gets to the C handler,
	 * it will not see the original cause. So stash
	 * the EXCCAUSE here so C handler can see the original cause.
	 *
	 * For double exception, DEPC in saved in earlier vector
	 * code.
	 */
	wsr a0, ZSR_EXCCAUSE_SAVE

	esync

	rsr a0, ZSR_DEPC_SAVE
	beqz a0, _not_triple_fault

	/* If stashed DEPC is not zero, we have started servicing
	 * a double exception and yet we are here because there is
	 * another exception (through user/kernel if PS.EXCM is
	 * cleared, or through double if PS.EXCM is set). This can
	 * be considered triple fault. Although there is no triple
	 * faults on Xtensa. Once PS.EXCM is set, it keeps going
	 * through double exception vector for any new exceptions.
	 * However, our exception code needs to unmask PS.EXCM to
	 * enable register window operations. So after that, any
	 * new exceptions will go through the kernel or user vectors
	 * depending on PS.UM. If there is continuous faults, it may
	 * keep ping-ponging between double and kernel/user exception
	 * vectors that may never get resolved. Since we stash DEPC
	 * during double exception, and the stashed one is only cleared
	 * once the double exception has been processed, we can use
	 * the stashed DEPC value to detect if the next exception could
	 * be considered a triple fault. If such a case exists, simply
	 * jump to an infinite loop, or quit the simulator, or invoke
	 * debugger.
	 */
	rsr a0, ZSR_EXCCAUSE_SAVE
	j _TripleFault

_not_triple_fault:
	rsr.exccause a0

	xsr a0, ZSR_EXCCAUSE_SAVE

	esync
.endif
#endif

	addi a1, a1, -___xtensa_irq_bsa_t_SIZEOF
	s32i a0, a1, ___xtensa_irq_bsa_t_a0_OFFSET
	s32i a2, a1, ___xtensa_irq_bsa_t_a2_OFFSET
	s32i a3, a1, ___xtensa_irq_bsa_t_a3_OFFSET

	/* Level "1" is the exception handler, which uses a different
	 * calling convention.  No special register holds the
	 * interrupted PS, instead we just assume that the CPU has
	 * turned on the EXCM bit and set INTLEVEL.
	 */
.if \LVL == 1
	rsr.ps a0
#ifdef CONFIG_XTENSA_MMU
	/* TLB misses also come through level 1 interrupts.
	 * We do not want to unconditionally unmask interrupts.
	 * Execution continues after a TLB miss is handled,
	 * and we need to preserve the interrupt mask.
	 * The interrupt mask will be cleared for non-TLB-misses
	 * level 1 interrupt later in the handler code.
	 */
	movi a2, ~PS_EXCM_MASK
#else
	movi a2, ~(PS_EXCM_MASK | PS_INTLEVEL_MASK)
#endif
	and a0, a0, a2
	s32i a0, a1, ___xtensa_irq_bsa_t_ps_OFFSET
.else
	rsr.eps\LVL a0
	s32i a0, a1, ___xtensa_irq_bsa_t_ps_OFFSET
.endif

	rsr.epc\LVL a0
	s32i a0, a1, ___xtensa_irq_bsa_t_pc_OFFSET

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

#if defined(CONFIG_XTENSA_SMALL_VECTOR_TABLE_ENTRY)
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
_Level\LVL\()Vector :
j _Level\LVL\()VectorHelper
.popsection
#endif

.endm

#endif	/* ZEPHYR_ARCH_XTENSA_INCLUDE_XTENSA_ASM2_S_H */
