/*
 * Copyright (c) 2017, Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <xtensa-asm2-s.h>
#include <offsets.h>
#include <toolchain.h>

/*
 * xtensa_spill_reg_windows
 *
 * Globally visible symbol to do register spills.  Useful for unit
 * testing, or maybe as part of a debug/watchdog/error handler.  Not a
 * C function, call this via CALL0 (so you probably have to save off
 * A0, but no other registers need to be spilled).  On return, all
 * registers not part of the current function will be spilled to
 * memory.
 */
.global xtensa_spill_reg_windows
.align 4
xtensa_spill_reg_windows:
        SPILL_ALL_WINDOWS
        ret

/*
 * xtensa_save_high_regs
 *
 * Call with CALL0, with A2/A3 available as scratch.  Pushes the high
 * A4-A15 GPRs to the stack if needed (i.e. if those registers are not
 * part of wrapped-around frames higher up the call stack), returning
 * to the caller with the stack pointer HAVING BEEN MODIFIED to
 * contain them.
 */
.global xtensa_save_high_regs
.align 4
xtensa_save_high_regs:
	/* Generate a rotated (modulo NREGS/4 bits!) WINDOWSTART in A2
	 * by duplicating the bits twice and shifting down by WINDOWBASE
	 * bits.  Now the LSB is the register quad at WINDOWBASE.
	 */
	rsr.WINDOWSTART a2
	slli a3, a2, (XCHAL_NUM_AREGS / 4)
	or a2, a2, a3
	rsr.WINDOWBASE a3
	ssr a3
	srl a2, a2

	mov a3, a1 /* Stash our original stack pointer */

	/* For the next three bits in WINDOWSTART (which correspond to
	 * the A4-A7, A8-A11 and A12-A15 quads), if we find a one,
	 * that means that the quad is owned by a wrapped-around call
	 * in the registers, so we don't need to spill it or any
	 * further registers from the GPRs and can skip to the end.
	 */
	bbsi a2, 1, _high_gpr_spill_done
	addi a1, a1, -16
	s32i a4, a1, 0
	s32i a5, a1, 4
	s32i a6, a1, 8
	s32i a7, a1, 12

	bbsi a2, 2, _high_gpr_spill_done
	addi a1, a1, -16
	s32i a8, a1, 0
	s32i a9, a1, 4
	s32i a10, a1, 8
	s32i a11, a1, 12

	bbsi a2, 3, _high_gpr_spill_done
	addi a1, a1, -16
	s32i a12, a1, 0
	s32i a13, a1, 4
	s32i a14, a1, 8
	s32i a15, a1, 12

_high_gpr_spill_done:
	/* Push the original stack pointer so we know at restore
	 * time how many registers were spilled, then return, leaving the
	 * modified SP in A1.
	 */
	addi a1, a1, -4
	s32i a3, a1, 0

	ret

/*
 * xtensa_restore_high_regs
 *
 * Does the inverse of xtensa_save_high_regs, taking a stack pointer
 * in A1 that resulted and restoring the A4-A15 state (and the stack
 * pointer) to the state they had at the earlier call.  Call with
 * CALL0, leaving A2/A3 available as scratch.
 */
.global xtensa_restore_high_regs
.align 4
xtensa_restore_high_regs:
	/* pop our "original" stack pointer into a2, stash in a3 also */
	l32i a2, a1, 0
	addi a1, a1, 4
	mov a3, a2

	beq a1, a2, _high_restore_done
	addi a2, a2, -16
	l32i a4, a2, 0
	l32i a5, a2, 4
	l32i a6, a2, 8
	l32i a7, a2, 12

	beq a1, a2, _high_restore_done
	addi a2, a2, -16
	l32i a8, a2, 0
	l32i a9, a2, 4
	l32i a10, a2, 8
	l32i a11, a2, 12

	beq a1, a2, _high_restore_done
	addi a2, a2, -16
	l32i a12, a2, 0
	l32i a13, a2, 4
	l32i a14, a2, 8
	l32i a15, a2, 12

_high_restore_done:
	mov a1, a3 /* Original stack */
	ret

/*
 * _restore_context
 *
 * Arrive here via a jump.  Enters into the restored context and does
 * not return.  A1 should have a context pointer in it as received
 * from switch or an interrupt exit.  Interrupts must be disabled,
 * and register windows should have been spilled.
 *
 * Note that exit from the restore is done with the RFI instruction,
 * using the EPCn/EPSn registers.  Those will have been saved already
 * by any interrupt entry so they are save to use.  Note that EPC1 and
 * RFE are NOT usable (they can't preserve PS).  Per the ISA spec, all
 * RFI levels do the same thing and differ only in the special
 * registers used to hold PC/PS, but Qemu has been observed to behave
 * strangely when RFI doesn't "return" to a INTLEVEL strictly lower
 * than it started from.  So pick level 6 (the highest that works on
 * Qemu, hardware doesn't care so it doesn't matter).  In theory we
 * should test to be able to support hardware with less than 6 levels,
 * though...
 */

#define RCTX_EPC_REG		_CONCAT(EPC, XCHAL_NUM_INTLEVELS)
#define RCTX_EPS_REG		_CONCAT(EPS, XCHAL_NUM_INTLEVELS)

.global _restore_context
_restore_context:
	call0 xtensa_restore_high_regs

	l32i a0, a1, BSA_PC_OFF
	wsr.RCTX_EPC_REG a0
	l32i a0, a1, BSA_PS_OFF
	wsr.RCTX_EPS_REG a0

	l32i a0, a1, BSA_SAR_OFF
	wsr.SAR a0
#if XCHAL_HAVE_LOOPS
	l32i a0, a1, BSA_LBEG_OFF
	wsr.LBEG a0
	l32i a0, a1, BSA_LEND_OFF
	wsr.LEND a0
	l32i a0, a1, BSA_LCOUNT_OFF
	wsr.LCOUNT a0
#endif
#if XCHAL_HAVE_S32C1I
	l32i a0, a1, BSA_SCOMPARE1_OFF
	wsr.SCOMPARE1 a0
#endif
#if XCHAL_HAVE_THREADPTR && defined(CONFIG_THREAD_LOCAL_STORAGE)
	l32i a0, a1, BSA_THREADPTR_OFF
	wur.THREADPTR a0
#endif
	rsync

	l32i a0, a1, BSA_A0_OFF
	l32i a2, a1, BSA_A2_OFF
	l32i a3, a1, BSA_A3_OFF
	addi a1, a1, BASE_SAVE_AREA_SIZE

	rfi XCHAL_NUM_INTLEVELS

/*
 * void xtensa_switch(void *new, void **old_return);
 *
 * Context switches into the prevoiusly-saved "new" handle, placing
 * the saved "old" handle into the address provided by old_return.
 */
.global xtensa_switch
.align 4
xtensa_switch:
	entry a1, 16
	SPILL_ALL_WINDOWS
	addi a1, a1, -BASE_SAVE_AREA_SIZE

	/* Stash our A0/2/3 and the shift/loop registers into the base
	 * save area so they get restored as they are now.  A2/A3
	 * don't actually get used post-restore, but they need to be
	 * stashed across the xtensa_save_high_regs call and this is a
	 * convenient place.
	 */
	s32i a0, a1, BSA_A0_OFF
	s32i a2, a1, BSA_A2_OFF
	s32i a3, a1, BSA_A3_OFF
	ODD_REG_SAVE

	/* Stash our PS register contents and a "restore" PC. */
	rsr.PS a0
	s32i a0, a1, BSA_PS_OFF
	movi a0, _switch_restore_pc
	s32i a0, a1, BSA_PC_OFF

	/* Now the high registers */
	call0 xtensa_save_high_regs

#ifdef CONFIG_KERNEL_COHERENCE
	/* Flush the stack.  The top of stack was stored for us in
	 * EXCSAVE3 (FIXME: shouldn't be hardcoded!) by arch_cohere_stacks().
	 */
	rsr.EXCSAVE3 a0
	mov a3, a1
flushloop:
	dhwb a3, 0
	addi a3, a3, XCHAL_DCACHE_LINESIZE
	blt a3, a0, flushloop
#endif

	/* Restore the A3 argument we spilled earlier (via the base
	 * save pointer pushed at the bottom of the stack) and set the
	 * stack to the "new" context out of the A2 spill slot.
	 */
	l32i a2, a1, 0
	l32i a3, a2, BSA_A3_OFF
	s32i a1, a3, 0

	/* Switch stack pointer and restore.  The jump to
	 * _restore_context does not return as such, but we arrange
	 * for the restored "next" address to be immediately after for
	 * sanity.
	 */
	l32i a1, a2, BSA_A2_OFF

#ifdef CONFIG_INSTRUMENT_THREAD_SWITCHING
	call4 z_thread_mark_switched_in
#endif
	j _restore_context
_switch_restore_pc:
	retw

/* Define our entry handler to load the struct kernel_t from the
 * MISC0 special register, and to find the nest and irq_stack values
 * at the precomputed offsets.
 */
.align 4
_handle_excint:
	EXCINT_HANDLER CONFIG_XTENSA_KERNEL_CPU_PTR_SR, ___cpu_t_nested_OFFSET, ___cpu_t_irq_stack_OFFSET

/* Define the actual vectors for the hardware-defined levels with
 * DEF_EXCINT.  These load a C handler address and jump to our handler
 * above.
 */

DEF_EXCINT 1, _handle_excint, xtensa_excint1_c

#if XCHAL_NMILEVEL >= 2
DEF_EXCINT 2, _handle_excint, xtensa_int2_c
#endif

#if XCHAL_NMILEVEL >= 3
DEF_EXCINT 3, _handle_excint, xtensa_int3_c
#endif

#if XCHAL_NMILEVEL >= 4
DEF_EXCINT 4, _handle_excint, xtensa_int4_c
#endif

#if XCHAL_NMILEVEL >= 5
DEF_EXCINT 5, _handle_excint, xtensa_int5_c
#endif

#if XCHAL_NMILEVEL >= 6
DEF_EXCINT 6, _handle_excint, xtensa_int6_c
#endif

#if XCHAL_NMILEVEL >= 7
DEF_EXCINT 7, _handle_excint, xtensa_int7_c
#endif

/* The user exception vector is defined here, as we need to handle
 * MOVSP exceptions in assembly (the result has to be to unspill the
 * caller function of the code that took the exception, and that can't
 * be done in C).  A prototype exists which mucks with the stack frame
 * from the C handler instead, but that would add a LARGE overhead to
 * some alloca() calls (those whent he caller has been spilled) just
 * to save these five cycles during other exceptions and L1
 * interrupts.  Maybe revisit at some point, with better benchmarking.
 * Note that _xt_alloca_exc is Xtensa-authored code which expects A0
 * to have been saved to EXCSAVE1, which is an unfortunate ABI given
 * that Zephyr code otherwise does not use the EXCSAVE registers.
 */
.pushsection .UserExceptionVector.text, "ax"
.global _Level1RealVector
_Level1RealVector:
	wsr.excsave1 a0
	rsr.exccause a0
	bnei a0, EXCCAUSE_ALLOCA, _not_alloca
	j _xt_alloca_exc
_not_alloca:
	rsr.excsave1 a0
	j _Level1Vector
.popsection

/* In theory you can have levels up to 15, but known hardware only uses 7. */
#if XCHAL_NMILEVEL > 7
#error More interrupts than expected.
#endif

/* We don't actually use "kernel mode" currently.  Populate the vector
 * out of simple caution in case app code clears the UM bit by mistake.
 */
.pushsection .KernelExceptionVector.text, "ax"
.global _KernelExceptionVector
_KernelExceptionVector:
	j _Level1Vector
.popsection

#ifdef XCHAL_DOUBLEEXC_VECTOR_VADDR
.pushsection .DoubleExceptionVector.text, "ax"
.global _DoubleExceptionVector
_DoubleExceptionVector:
#if XCHAL_HAVE_DEBUG
/* Signals an unhandled double exception */
1:	break	1, 4
#else
1:
#endif
	j	1b
.popsection
#endif
