/* swap_macros.h - helper macros for context switch */

/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _SWAP_MACROS__H_
#define _SWAP_MACROS__H_

#include <nano_private.h>
#include <offsets.h>
#include <toolchain.h>
#include <arch/cpu.h>

#ifdef _ASMLANGUAGE

/* entering this macro, current is in r2 */
.macro _save_callee_saved_regs

	sub_s sp, sp, __tCalleeSaved_SIZEOF

	/* save regs on stack */
	st r13, [sp, __tCalleeSaved_r13_OFFSET]
	st r14, [sp, __tCalleeSaved_r14_OFFSET]
	st r15, [sp, __tCalleeSaved_r15_OFFSET]
	st r16, [sp, __tCalleeSaved_r16_OFFSET]
	st r17, [sp, __tCalleeSaved_r17_OFFSET]
	st r18, [sp, __tCalleeSaved_r18_OFFSET]
	st r19, [sp, __tCalleeSaved_r19_OFFSET]
	st r20, [sp, __tCalleeSaved_r20_OFFSET]
	st r21, [sp, __tCalleeSaved_r21_OFFSET]
	st r22, [sp, __tCalleeSaved_r22_OFFSET]
	st r23, [sp, __tCalleeSaved_r23_OFFSET]
	st r24, [sp, __tCalleeSaved_r24_OFFSET]
	st r25, [sp, __tCalleeSaved_r25_OFFSET]
	st r26, [sp, __tCalleeSaved_r26_OFFSET]
	st fp,  [sp, __tCalleeSaved_fp_OFFSET]
	st r30, [sp, __tCalleeSaved_r30_OFFSET]

	/* save stack pointer in tCCS */
	st sp, [r2, __tCCS_preempReg_OFFSET + __tPreempt_sp_OFFSET]
.endm

/* entering this macro, current is in r2 */
.macro _load_callee_saved_regs
	/* restore stack pointer from tCCS */
	ld sp, [r2, __tCCS_preempReg_OFFSET + __tPreempt_sp_OFFSET]

	ld r13, [sp, __tCalleeSaved_r13_OFFSET]
	ld r14, [sp, __tCalleeSaved_r14_OFFSET]
	ld r15, [sp, __tCalleeSaved_r15_OFFSET]
	ld r16, [sp, __tCalleeSaved_r16_OFFSET]
	ld r17, [sp, __tCalleeSaved_r17_OFFSET]
	ld r18, [sp, __tCalleeSaved_r18_OFFSET]
	ld r19, [sp, __tCalleeSaved_r19_OFFSET]
	ld r20, [sp, __tCalleeSaved_r20_OFFSET]
	ld r21, [sp, __tCalleeSaved_r21_OFFSET]
	ld r22, [sp, __tCalleeSaved_r22_OFFSET]
	ld r23, [sp, __tCalleeSaved_r23_OFFSET]
	ld r24, [sp, __tCalleeSaved_r24_OFFSET]
	ld r25, [sp, __tCalleeSaved_r25_OFFSET]
	ld r26, [sp, __tCalleeSaved_r26_OFFSET]
	ld fp,  [sp, __tCalleeSaved_fp_OFFSET]
	ld r30, [sp, __tCalleeSaved_r30_OFFSET]

	add_s sp, sp, __tCalleeSaved_SIZEOF

.endm

/*
 * Must be called with interrupts locked or in P0.
 * Upon exit, sp will be pointing to the stack frame.
 */
.macro _create_irq_stack_frame

	sub_s sp, sp, __tISF_SIZEOF

	st blink, [sp, __tISF_blink_OFFSET]

	/* store these right away so we can use them if needed */

	st_s r13, [sp, __tISF_r13_OFFSET]
	st_s r12, [sp, __tISF_r12_OFFSET]
	st   r11, [sp, __tISF_r11_OFFSET]
	st   r10, [sp, __tISF_r10_OFFSET]
	st   r9,  [sp, __tISF_r9_OFFSET]
	st   r8,  [sp, __tISF_r8_OFFSET]
	st   r7,  [sp, __tISF_r7_OFFSET]
	st   r6,  [sp, __tISF_r6_OFFSET]
	st   r5,  [sp, __tISF_r5_OFFSET]
	st   r4,  [sp, __tISF_r4_OFFSET]
	st_s r3,  [sp, __tISF_r3_OFFSET]
	st_s r2,  [sp, __tISF_r2_OFFSET]
	st_s r1,  [sp, __tISF_r1_OFFSET]
	st_s r0,  [sp, __tISF_r0_OFFSET]

	mov r0, lp_count
	st_s r0, [sp, __tISF_lp_count_OFFSET]
	lr r0, [_ARC_V2_LP_START]
	st_s r0, [sp, __tISF_lp_start_OFFSET]
	lr r0, [_ARC_V2_LP_END]
	st_s r0, [sp, __tISF_lp_end_OFFSET]

.endm

/*
 * Must be called with interrupts locked or in P0.
 * sp must be pointing the to stack frame.
 */
.macro _pop_irq_stack_frame

	ld blink, [sp, __tISF_blink_OFFSET]

	ld_s r0, [sp, __tISF_lp_count_OFFSET]
	mov lp_count, r0
	ld_s r0, [sp, __tISF_lp_start_OFFSET]
	sr r0, [_ARC_V2_LP_START]
	ld_s r0, [sp, __tISF_lp_end_OFFSET]
	sr r0, [_ARC_V2_LP_END]

	ld_s r13, [sp, __tISF_r13_OFFSET]
	ld_s r12, [sp, __tISF_r12_OFFSET]
	ld   r11, [sp, __tISF_r11_OFFSET]
	ld   r10, [sp, __tISF_r10_OFFSET]
	ld   r9,  [sp, __tISF_r9_OFFSET]
	ld   r8,  [sp, __tISF_r8_OFFSET]
	ld   r7,  [sp, __tISF_r7_OFFSET]
	ld   r6,  [sp, __tISF_r6_OFFSET]
	ld   r5,  [sp, __tISF_r5_OFFSET]
	ld   r4,  [sp, __tISF_r4_OFFSET]
	ld_s r3,  [sp, __tISF_r3_OFFSET]
	ld_s r2,  [sp, __tISF_r2_OFFSET]
	ld_s r1,  [sp, __tISF_r1_OFFSET]
	ld_s r0,  [sp, __tISF_r0_OFFSET]

	/*
	 * All gprs have been reloaded, the only one that is still usable is
	 * ilink.
	 *
	 * The pc and status32 values will still be on the stack. We cannot
	 * pop them yet because the callers of _pop_irq_stack_frame must reload
	 * status32 differently depending on the context they are running in
	 * (_Swap(), firq or exception).
	 */
	add_s sp, sp, __tISF_SIZEOF

.endm

#endif /* _ASMLANGUAGE */
#endif /*  _SWAP_MACROS__H_ */
