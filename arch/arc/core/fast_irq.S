/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file
 * @brief Handling of transitions to-and-from fast IRQs (FIRQ)
 *
 * This module implements the code for handling entry to and exit from Fast IRQs.
 *
 * See isr_wrapper.S for details.
 */

#define _ASMLANGUAGE

#include <kernel_structs.h>
#include <offsets_short.h>
#include <toolchain.h>
#include <arch/cpu.h>
#include "swap_macros.h"

GTEXT(_firq_enter)
GTEXT(_firq_exit)
GTEXT(_firq_stack_setup)

#if CONFIG_RGF_NUM_BANKS != 1
GDATA(_firq_stack)
GTEXT(_is_next_thread_current)

SECTION_VAR(NOINIT, _firq_stack)
	.space CONFIG_FIRQ_STACK_SIZE
#else
GDATA(saved_r0)
#endif

.macro _firq_return
#if CONFIG_RGF_NUM_BANKS == 1
	b _firq_no_reschedule
#else
	rtie
#endif
.endm

/**
 *
 * @brief Work to be done before handing control to a FIRQ ISR
 *
 * The processor switches to a second register bank so registers from the
 * current bank do not have to be preserved yet. The only issue is the LP_START/
 * LP_COUNT/LP_END registers, which are not banked. These can be saved
 * in available callee saved registers.
 *
 * If all FIRQ ISRs are programmed such that there are no use of the LP
 * registers (ie. no LPcc instruction), and CONFIG_ARC_STACK_CHECKING is
 * not set, then the kernel can be configured to not save and restore them.
 *
 * When entering a FIRQ, interrupts might as well be locked: the processor is
 * running at its highest priority, and cannot be interrupted by any other
 * interrupt. An exception, however, can be taken.
 *
 * Assumption by _isr_demux: r3 is untouched by _firq_enter.
 *
 * @return N/A
 */

SECTION_FUNC(TEXT, _firq_enter)

/*
 * ATTENTION:
 * If CONFIG_RGF_NUM_BANKS>1, firq uses a 2nd register bank so GPRs do
 * not need to be saved.
 * If CONFIG_RGF_NUM_BANKS==1, firq must use the stack to save registers.
 * This has already been done by _isr_enter.
 */

#ifdef CONFIG_ARC_STACK_CHECKING
	/* disable stack checking */
	lr r2, [_ARC_V2_STATUS32]
	bclr r2, r2, _ARC_V2_STATUS32_SC_BIT
	kflag r2
#endif

#if CONFIG_RGF_NUM_BANKS != 1
#ifndef CONFIG_FIRQ_NO_LPCC
	/*
	 * Save LP_START/LP_COUNT/LP_END because called handler might use.
	 * Save these in callee saved registers to avoid using memory.
	 * These will be saved by the compiler if it needs to spill them.
	 */
	mov r23,lp_count
	lr r24, [_ARC_V2_LP_START]
	lr r25, [_ARC_V2_LP_END]
#endif
#endif

	j @_isr_demux

/**
 *
 * @brief Work to be done exiting a FIRQ
 *
 * @return N/A
 */

SECTION_FUNC(TEXT, _firq_exit)

#if CONFIG_RGF_NUM_BANKS != 1
#ifndef CONFIG_FIRQ_NO_LPCC
	/* restore lp_count, lp_start, lp_end from r23-r25 */
	mov lp_count,r23
	sr r24, [_ARC_V2_LP_START]
	sr r25, [_ARC_V2_LP_END]
#endif
#endif

	mov_s r1, _kernel
	ld_s r2, [r1, _kernel_offset_to_current]

#if CONFIG_NUM_IRQ_PRIO_LEVELS > 1
	/* check if we're a nested interrupt: if so, let the interrupted
	 * interrupt handle the reschedule */

	lr r3, [_ARC_V2_AUX_IRQ_ACT]

	/* the OS on ARCv2 always runs in kernel mode, so assume bit31 [U] in
	 * AUX_IRQ_ACT is always 0: if the contents of AUX_IRQ_ACT is not 1, it
	 * means that another bit is set so an interrupt was interrupted.
	 */

	breq r3, 1, _firq_check_for_swap

	_firq_return
#endif

.balign 4
_firq_check_for_swap:
	/* coop thread ? do not schedule */
	ld_s r0, [r2, _thread_offset_to_prio]
	brlt r0, 0, _firq_no_reschedule

	/* scheduler locked ? do not schedule */
	ld_s r0, [r2, _thread_offset_to_sched_locked]
	brgt r0, 0, _firq_no_reschedule

	/* check if the current thread needs to be rescheduled */
	push_s r2
	push_s r1
	push_s blink
	jl _is_next_thread_current
	pop_s blink
	pop_s r1
	pop_s r2
#if CONFIG_RGF_NUM_BANKS != 1
#ifndef CONFIG_FIRQ_NO_LPCC
	/*
	 * restore lp_count, lp_start, lp_end from r23-r25 in case
	 * _is_next_thread_current() routine used them
	 */
	mov lp_count,r23
	sr r24, [_ARC_V2_LP_START]
	sr r25, [_ARC_V2_LP_END]
#endif
#endif
	breq r0, 0, _firq_reschedule
	/* fall to no rescheduling */

.balign 4
_firq_no_reschedule:
	/*
	 * Keeping this code block close to those that use it allows using brxx
	 * instruction instead of a pair of cmp and bxx
	 */
#if CONFIG_RGF_NUM_BANKS == 1
	add sp,sp,4 /* don't need r0 from stack */
	pop_s r1
	pop_s r2
	pop_s r3
	pop r4
	pop r5
	pop r6
	pop r7
	pop r8
	pop r9
	pop r10
	pop r11
	pop_s r12
	pop_s r13
	pop_s blink
	pop_s r0
	sr r0, [_ARC_V2_LP_END]
	pop_s r0
	sr r0, [_ARC_V2_LP_START]
	pop_s r0
	mov lp_count,r0
	ld r0,[saved_r0]
	add sp,sp,8 /* don't need ilink & status32_po from stack */
#endif
	rtie

.balign 4
_firq_reschedule:

#if CONFIG_RGF_NUM_BANKS != 1
	/*
	 * We know there is no interrupted interrupt of lower priority at this
	 * point, so when switching back to register bank 0, it will contain the
	 * registers from the interrupted thread.
	 */

	/* chose register bank #0 */
	lr r0, [_ARC_V2_STATUS32]
	and r0, r0, ~_ARC_V2_STATUS32_RB(7)
	kflag r0

	/* we're back on the outgoing thread's stack */
	_create_irq_stack_frame

	/*
	 * In a FIRQ, STATUS32 of the outgoing thread is in STATUS32_P0 and the
	 * PC in ILINK: save them in status32/pc respectively.
	 */

	lr r0, [_ARC_V2_STATUS32_P0]
	st_s r0, [sp, ___isf_t_status32_OFFSET]

	st ilink, [sp, ___isf_t_pc_OFFSET] /* ilink into pc */
#endif

	mov_s r1, _kernel
	ld_s r2, [r1, _kernel_offset_to_current]

	_save_callee_saved_regs

	st _CAUSE_FIRQ, [r2, _thread_offset_to_relinquish_cause]

	/*
	 * Save needed registers to callee saved ones. It is faster than
	 * pushing them to stack. It is possible to do since program has
	 * just saved them and the calling routine will save them in turn
	 * if it uses them.
	 */
	mov_s r13, blink
	mov_s r14, r1
	jl _get_next_ready_thread
	mov_s blink, r13
	mov_s r1, r14
	mov_s r2, r0
	st_s r2, [r1, _kernel_offset_to_current]

#ifdef CONFIG_ARC_STACK_CHECKING
	/* Use stack top and down registers from restored context */
	add r3, r2, _K_THREAD_NO_FLOAT_SIZEOF
	sr r3, [_ARC_V2_KSTACK_TOP]
	ld_s r3, [r2, _thread_offset_to_stack_top]
	sr r3, [_ARC_V2_KSTACK_BASE]
#endif
	/*
	 * _load_callee_saved_regs expects incoming thread in r2.
	 * _load_callee_saved_regs restores the stack pointer.
	 */
	_load_callee_saved_regs

	ld_s r3, [r2, _thread_offset_to_relinquish_cause]

	breq r3, _CAUSE_RIRQ, _firq_return_from_rirq
	nop
	breq r3, _CAUSE_FIRQ, _firq_return_from_firq
	nop

	/* fall through */

.balign 4
_firq_return_from_coop:

	ld_s r3, [r2, _thread_offset_to_intlock_key]
	st  0, [r2, _thread_offset_to_intlock_key]

	/* pc into ilink */
	pop_s r0
	mov ilink, r0

	pop_s r0 /* status32 into r0 */
	/*
	 * There are only two interrupt lock states: locked and unlocked. When
	 * entering _Swap(), they are always locked, so the IE bit is unset in
	 * status32. If the incoming thread had them locked recursively, it
	 * means that the IE bit should stay unset. The only time the bit
	 * has to change is if they were not locked recursively.
	 */
	and.f r3, r3, (1 << 4)
	or.nz r0, r0, _ARC_V2_STATUS32_IE
	sr r0, [_ARC_V2_STATUS32_P0]

	ld_s r0, [r2, _thread_offset_to_return_value]
	rtie

.balign 4
_firq_return_from_rirq:
_firq_return_from_firq:

	_pop_irq_stack_frame

	ld ilink, [sp, -4] /* status32 into ilink */
	sr ilink, [_ARC_V2_STATUS32_P0]
	ld ilink, [sp, -8] /* pc into ilink */

	/* LP registers are already restored, just switch back to bank 0 */
	rtie


/**
 *
 * @brief Install the FIRQ stack in register bank 1 if CONFIG_RGF_NUM_BANK!=1
 *
 * @return N/A
 */

SECTION_FUNC(TEXT, _firq_stack_setup)

#if CONFIG_RGF_NUM_BANKS != 1
	lr r0, [_ARC_V2_STATUS32]
	and r0, r0, ~_ARC_V2_STATUS32_RB(7)
	or r0, r0, _ARC_V2_STATUS32_RB(1)
	kflag r0

	mov sp, _firq_stack
	add sp, sp, CONFIG_FIRQ_STACK_SIZE

	/*
	 * We have to reload r0 here, because it is bank1 r0 which contains
	 * garbage, not bank0 r0 containing the previous value of status32.
	 */
	lr r0, [_ARC_V2_STATUS32]
	and r0, r0, ~_ARC_V2_STATUS32_RB(7)
	kflag r0
#endif

	j_s [blink]
