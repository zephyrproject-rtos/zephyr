/* swap_macros.h - helper macros for context switch */

/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SWAP_MACROS__H_
#define _SWAP_MACROS__H_

#include <kernel_structs.h>
#include <offsets_short.h>
#include <toolchain.h>
#include <arch/cpu.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _ASMLANGUAGE

/* entering this macro, current is in r2 */
.macro _save_callee_saved_regs

	sub_s sp, sp, ___callee_saved_stack_t_SIZEOF

	/* save regs on stack */
	st_s r13, [sp, ___callee_saved_stack_t_r13_OFFSET]
	st_s r14, [sp, ___callee_saved_stack_t_r14_OFFSET]
	st_s r15, [sp, ___callee_saved_stack_t_r15_OFFSET]
	st r16, [sp, ___callee_saved_stack_t_r16_OFFSET]
	st r17, [sp, ___callee_saved_stack_t_r17_OFFSET]
	st r18, [sp, ___callee_saved_stack_t_r18_OFFSET]
	st r19, [sp, ___callee_saved_stack_t_r19_OFFSET]
	st r20, [sp, ___callee_saved_stack_t_r20_OFFSET]
	st r21, [sp, ___callee_saved_stack_t_r21_OFFSET]
	st r22, [sp, ___callee_saved_stack_t_r22_OFFSET]
	st r23, [sp, ___callee_saved_stack_t_r23_OFFSET]
	st r24, [sp, ___callee_saved_stack_t_r24_OFFSET]
	st r25, [sp, ___callee_saved_stack_t_r25_OFFSET]
	st r26, [sp, ___callee_saved_stack_t_r26_OFFSET]
	st fp,  [sp, ___callee_saved_stack_t_fp_OFFSET]
	st r30, [sp, ___callee_saved_stack_t_r30_OFFSET]

	/* save stack pointer in struct tcs */
	st sp, [r2, _thread_offset_to_sp]
.endm

/* entering this macro, current is in r2 */
.macro _load_callee_saved_regs
	/* restore stack pointer from struct tcs */
	ld sp, [r2, _thread_offset_to_sp]

	ld_s r13, [sp, ___callee_saved_stack_t_r13_OFFSET]
	ld_s r14, [sp, ___callee_saved_stack_t_r14_OFFSET]
	ld_s r15, [sp, ___callee_saved_stack_t_r15_OFFSET]
	ld r16, [sp, ___callee_saved_stack_t_r16_OFFSET]
	ld r17, [sp, ___callee_saved_stack_t_r17_OFFSET]
	ld r18, [sp, ___callee_saved_stack_t_r18_OFFSET]
	ld r19, [sp, ___callee_saved_stack_t_r19_OFFSET]
	ld r20, [sp, ___callee_saved_stack_t_r20_OFFSET]
	ld r21, [sp, ___callee_saved_stack_t_r21_OFFSET]
	ld r22, [sp, ___callee_saved_stack_t_r22_OFFSET]
	ld r23, [sp, ___callee_saved_stack_t_r23_OFFSET]
	ld r24, [sp, ___callee_saved_stack_t_r24_OFFSET]
	ld r25, [sp, ___callee_saved_stack_t_r25_OFFSET]
	ld r26, [sp, ___callee_saved_stack_t_r26_OFFSET]
	ld fp,  [sp, ___callee_saved_stack_t_fp_OFFSET]
	ld r30, [sp, ___callee_saved_stack_t_r30_OFFSET]

	add_s sp, sp, ___callee_saved_stack_t_SIZEOF

.endm

.macro _discard_callee_saved_regs
	add_s sp, sp, ___callee_saved_stack_t_SIZEOF
.endm

/*
 * Must be called with interrupts locked or in P0.
 * Upon exit, sp will be pointing to the stack frame.
 */
.macro _create_irq_stack_frame

	sub_s sp, sp, ___isf_t_SIZEOF

	st blink, [sp, ___isf_t_blink_OFFSET]

	/* store these right away so we can use them if needed */

	st_s r13, [sp, ___isf_t_r13_OFFSET]
	st_s r12, [sp, ___isf_t_r12_OFFSET]
	st   r11, [sp, ___isf_t_r11_OFFSET]
	st   r10, [sp, ___isf_t_r10_OFFSET]
	st   r9,  [sp, ___isf_t_r9_OFFSET]
	st   r8,  [sp, ___isf_t_r8_OFFSET]
	st   r7,  [sp, ___isf_t_r7_OFFSET]
	st   r6,  [sp, ___isf_t_r6_OFFSET]
	st   r5,  [sp, ___isf_t_r5_OFFSET]
	st   r4,  [sp, ___isf_t_r4_OFFSET]
	st_s r3,  [sp, ___isf_t_r3_OFFSET]
	st_s r2,  [sp, ___isf_t_r2_OFFSET]
	st_s r1,  [sp, ___isf_t_r1_OFFSET]
	st_s r0,  [sp, ___isf_t_r0_OFFSET]

	mov r0, lp_count
	st_s r0, [sp, ___isf_t_lp_count_OFFSET]
	lr r1, [_ARC_V2_LP_START]
	lr r0, [_ARC_V2_LP_END]
	st_s r1, [sp, ___isf_t_lp_start_OFFSET]
	st_s r0, [sp, ___isf_t_lp_end_OFFSET]

.endm

/*
 * Must be called with interrupts locked or in P0.
 * sp must be pointing the to stack frame.
 */
.macro _pop_irq_stack_frame

	ld blink, [sp, ___isf_t_blink_OFFSET]

	ld_s r0, [sp, ___isf_t_lp_count_OFFSET]
	mov lp_count, r0
	ld_s r1, [sp, ___isf_t_lp_start_OFFSET]
	ld_s r0, [sp, ___isf_t_lp_end_OFFSET]
	sr r1, [_ARC_V2_LP_START]
	sr r0, [_ARC_V2_LP_END]

	ld_s r13, [sp, ___isf_t_r13_OFFSET]
	ld_s r12, [sp, ___isf_t_r12_OFFSET]
	ld   r11, [sp, ___isf_t_r11_OFFSET]
	ld   r10, [sp, ___isf_t_r10_OFFSET]
	ld   r9,  [sp, ___isf_t_r9_OFFSET]
	ld   r8,  [sp, ___isf_t_r8_OFFSET]
	ld   r7,  [sp, ___isf_t_r7_OFFSET]
	ld   r6,  [sp, ___isf_t_r6_OFFSET]
	ld   r5,  [sp, ___isf_t_r5_OFFSET]
	ld   r4,  [sp, ___isf_t_r4_OFFSET]
	ld_s r3,  [sp, ___isf_t_r3_OFFSET]
	ld_s r2,  [sp, ___isf_t_r2_OFFSET]
	ld_s r1,  [sp, ___isf_t_r1_OFFSET]
	ld_s r0,  [sp, ___isf_t_r0_OFFSET]

	/*
	 * All gprs have been reloaded, the only one that is still usable is
	 * ilink.
	 *
	 * The pc and status32 values will still be on the stack. We cannot
	 * pop them yet because the callers of _pop_irq_stack_frame must reload
	 * status32 differently depending on the execution context they are
	 * running in (_Swap(), firq or exception).
	 */
	add_s sp, sp, ___isf_t_SIZEOF

.endm

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /*  _SWAP_MACROS__H_ */
