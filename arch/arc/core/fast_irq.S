/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Handling of transitions to-and-from fast IRQs (FIRQ)
 *
 * This module implements the code for handling entry to and exit from Fast IRQs.
 *
 * See isr_wrapper.S for details.
 */

#include <kernel_structs.h>
#include <offsets_short.h>
#include <toolchain.h>
#include <arch/cpu.h>
#include <swap_macros.h>

GTEXT(_firq_enter)
GTEXT(_firq_exit)

GDATA(exc_nest_count)
#if CONFIG_RGF_NUM_BANKS == 1
GDATA(saved_r0)
#else
GDATA(saved_sp)
#endif

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
 * This has already been done by _isr_wrapper.
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

	ld r1, [exc_nest_count]
	add r0, r1, 1
	st r0, [exc_nest_count]
	cmp r1, 0

	bgt.d  firq_nest
	mov r0, sp

	mov r1, _kernel
	ld sp, [r1, _kernel_offset_to_irq_stack]
#if CONFIG_RGF_NUM_BANKS != 1
	b firq_nest_1
firq_nest:
	mov r1, ilink
	lr r0, [_ARC_V2_STATUS32]
	and r0, r0, ~_ARC_V2_STATUS32_RB(7)
	kflag r0

	st sp, [saved_sp]

	lr ilink, [_ARC_V2_STATUS32]
	or ilink, ilink, _ARC_V2_STATUS32_RB(1)
	kflag ilink
	mov r0, sp
	ld sp, [saved_sp]
	mov ilink, r1
firq_nest_1:
#else
firq_nest:
#endif
	push_s r0
	j @_isr_demux



/**
 *
 * @brief Work to be done exiting a FIRQ
 *
 * @return N/A
 */

SECTION_FUNC(TEXT, _firq_exit)

	pop sp

#if CONFIG_RGF_NUM_BANKS != 1
#ifndef CONFIG_FIRQ_NO_LPCC
	/* restore lp_count, lp_start, lp_end from r23-r25 */
	mov lp_count,r23
	sr r24, [_ARC_V2_LP_START]
	sr r25, [_ARC_V2_LP_END]
#endif
#endif
	/* check if we're a nested interrupt: if so, let the interrupted
	 * interrupt handle the reschedule */
	mov	r1, exc_nest_count
	ld	r0, [r1]
	sub	r0, r0, 1
	cmp	r0, 0
	bne.d	_firq_no_reschedule
	st	r0, [r1]

#ifdef CONFIG_PREEMPT_ENABLED

	mov_s r1, _kernel
	ld_s r2, [r1, _kernel_offset_to_current]

	/*
	 * Non-preemptible thread ? Do not schedule (see explanation of
	 * preempt field in kernel_struct.h).
	 */
	ldh_s r0, [r2, _thread_offset_to_preempt]
	brhs r0, _NON_PREEMPT_THRESHOLD, _firq_no_reschedule

	/* Check if the current thread (in r2) is the cached thread */
	ld_s r0, [r1, _kernel_offset_to_ready_q_cache]
	brne r0, r2, _firq_reschedule

	/* fall to no rescheduling */

#endif /* CONFIG_PREEMPT_ENABLED */

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
#ifdef CONFIG_CODE_DENSITY
	pop_s r0
	sr r0, [_ARC_V2_EI_BASE]
	pop_s r0
	sr r0, [_ARC_V2_LDI_BASE]
	pop_s r0
	sr r0, [_ARC_V2_JLI_BASE]
#endif
	ld r0,[saved_r0]
	add sp,sp,8 /* don't need ilink & status32_po from stack */
#endif
	rtie

#ifdef CONFIG_PREEMPT_ENABLED

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

	ld_s r2, [r1, _kernel_offset_to_ready_q_cache]
	st_s r2, [r1, _kernel_offset_to_current]

#ifdef CONFIG_ARC_STACK_CHECKING
	/* Use stack top and base registers from restored context */
	ld r3, [r2, _thread_offset_to_stack_base]
	sr r3, [_ARC_V2_KSTACK_BASE]
	ld r3, [r2, _thread_offset_to_stack_top]
	sr r3, [_ARC_V2_KSTACK_TOP]
#endif
	/*
	 * _load_callee_saved_regs expects incoming thread in r2.
	 * _load_callee_saved_regs restores the stack pointer.
	 */
	_load_callee_saved_regs

#ifdef CONFIG_MPU_STACK_GUARD
	push_s r2
	mov r0, r2
	bl configure_mpu_stack_guard
	pop_s r2
#endif

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

#endif /* CONFIG_PREEMPT_ENABLED */
