/*
 * Copyright (c) 2017, Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <xtensa-asm2-s.h>

/*
 * spill_reg_windows
 *
 * Globally visible symbol to do register spills.  Useful for unit
 * testing, or maybe as part of a debug/watchdog/error handler.  Not a
 * C function, call this via CALL0 (so you probably have to save off
 * A0, but no other registers need to be spilled).  On return, all
 * registers not part of the current function will be spilled to
 * memory.
 */
.global spill_reg_windows
.align 4
spill_reg_windows:
	SPILL_ALL_WINDOWS
	ret

/* Takes two arguments, a function pointer in A2 and a count in A3.
 * Decrements the count, and if non-zero calls itself
 * recursively.  Otherwise calls the function.
 */
.align 4
_one_quad:
	entry a1, 16
	addi a3, a3, -1
	beqz a3, _call_fn
	mov a6, a2
	mov a7, a3
	call4 _one_quad
	retw
_call_fn:
	callx4 a2
	retw

/* Takes a function pointer as its single argument (in A2 as per ABI)
 * and invokes it having "filled" the register window with CALL4
 * frames.
 */
.global fill_window
.align 4
fill_window:
	entry a1, 16
	mov a6, a2
	movi a7, 16
	call4 _one_quad
	retw

/* The operation of the specific tests is to put some known values
 * into a particular subset of high registers.  Doing this will cause
 * the window exception to spill wrapped-around frames to make space,
 * which should be detected by the save code and cause it to write
 * only the specific registers needed.
 */

.global test_highreg_0
.align 4
test_highreg_0:
	entry a1, 16
	j _test_highreg_end

.global test_highreg_4
.align 4
test_highreg_4:
	entry a1, 16
	movi a4, 4
	movi a5, 5
	movi a6, 6
	movi a7, 7
	j _test_highreg_end

.global test_highreg_8
.align 4
test_highreg_8:
	entry a1, 16
	movi a4, 4
	movi a5, 5
	movi a6, 6
	movi a7, 7
	movi a8, 8
	movi a9, 9
	movi a10, 10
	movi a11, 11
	j _test_highreg_end

.global test_highreg_12
.align 4
test_highreg_12:
	entry a1, 16
	movi a4, 4
	movi a5, 5
	movi a6, 6
	movi a7, 7
	movi a8, 8
	movi a9, 9
	movi a10, 10
	movi a11, 11
	movi a12, 12
	movi a13, 13
	movi a14, 14
	movi a15, 15
	j _test_highreg_end

/* Loads a pointer into A1 to serve as a "save stack" that can be
 * inspected by the caller, does the save, then restores and returns,
 * placing the output stack pointer "test_highreg_handle" for
 * inspection.
 */
.align 4
_test_highreg_end:
	movi a2, _test_highreg_a0_save
	s32i a0, a2, 0
	movi a2, _test_highreg_sp_save
	s32i a1, a2, 0

	/* Do it once just to make sure the restore code works */
	call0 xtensa_save_high_regs
	movi a2, 22
	movi a3, 33
	call0 xtensa_restore_high_regs

	movi a2, test_highreg_sp_top
	l32i a1, a2, 0
	call0 xtensa_save_high_regs
	movi a2, test_highreg_handle
	s32i a1, a2, 0
	movi a2, _test_highreg_sp_save
	l32i a1, a2, 0
	movi a2, _test_highreg_a0_save
	l32i a0, a2, 0
	retw

.global testfw
.align 4
testfw:
	entry a1, 16
	movi a2, testfw_wb
	rsr.WINDOWBASE a3
	s32i a3, a2, 0
	movi a2, testfw_ws
	rsr.WINDOWSTART a3
	s32i a3, a2, 0
	retw


/* Does a "jump" to a symbol named "rfi_jump_c" using RFI. */
.global rfi_jump
.align 4
rfi_jump:
#if 1
	movi a2, rfi_jump_c
	wsr.EPC6 a2
	rsr.PS a2
	wsr.EPS6 a2
	rsync
	rfi 6
#else
	movi a2, rfi_jump_c
	jx a2
#endif

.global do_xstack_call
.align 4
do_xstack_call:
	entry a1, 16

	mov a3, a2  /* a3 == "new sp" (this function's 1st argument) */
	movi a2, xstack_top /* a2 == cross-stack callee/handler */

	/* Fake a save frame, CROSS_STACK_CALL just wants the old SP
	 * from it, we don't need to fill it.  Only one available
	 * register, so it uses the bottom slot of the "fake BSA" as
	 * scratch.
	 */
	addi a1, a1, -BASE_SAVE_AREA_SIZE
	s32i a1, a1, 0
	addi a1, a1, -4
	l32i a1, a1, 4
	s32i a1, a1, 0

	CROSS_STACK_CALL

	/* Restore the stack */
	l32i a1, a1, 0
	addi a1, a1, BASE_SAVE_AREA_SIZE

	retw

/* Define our exception handler.  Offsets written to assume:
 *     struct { int nest; void *stack_top; }
 */
.align 4
_handle_excint:
	EXCINT_HANDLER MISC0, 0, 4

/* And a single vector at level 5 to point to it and call our C
 * handler.  There is a timer on most cores (qemu and LX6/ESP-32 at
 * least) that can be used for unit testing.
 */
DEF_EXCINT 5, _handle_excint, handle_int5_c
