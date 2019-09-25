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
#ifdef CONFIG_ARC_SECURE_FIRMWARE
	lr r2, [_ARC_V2_SEC_STAT]
	bclr r2, r2, _ARC_V2_SEC_STAT_SSC_BIT
	sflag r2
#else
	/* disable stack checking */
	lr r2, [_ARC_V2_STATUS32]
	bclr r2, r2, _ARC_V2_STATUS32_SC_BIT
	kflag r2
#endif
#endif

#if CONFIG_RGF_NUM_BANKS != 1
	/*
	 * Save LP_START/LP_COUNT/LP_END because called handler might use.
	 * Save these in callee saved registers to avoid using memory.
	 * These will be saved by the compiler if it needs to spill them.
	 */
	mov r23,lp_count
	lr r24, [_ARC_V2_LP_START]
	lr r25, [_ARC_V2_LP_END]
#endif

	/* check whether irq stack is used */
	_check_and_inc_int_nest_counter r0, r1

	bne.d firq_nest
	mov_s r0, sp

	_get_curr_cpu_irq_stack sp
#if CONFIG_RGF_NUM_BANKS != 1
	b firq_nest_1
firq_nest:
	/*
	 * because firq and rirq share the same interrupt stack,
	 * switch back to original register bank to get correct sp.
	 * to get better firq latency, an approach is to prepare
	 * separate interrupt stack for firq and do not do thread
	 * switch in firq.
	 */
	lr r1, [_ARC_V2_STATUS32]
	and r1, r1, ~_ARC_V2_STATUS32_RB(7)
	kflag r1

	/* here use _ARC_V2_USER_SP and ilink to exchange sp
	 * save original value of _ARC_V2_USER_SP and ilink into
	 * the stack of interrupted context first, then restore them later
	 */
	push ilink
	PUSHAX ilink, _ARC_V2_USER_SP

	/* sp here is the sp of interrupted context */
	sr sp, [_ARC_V2_USER_SP]
	/* here, bank 0 sp must go back to the value before push and
	 * PUSHAX as we will switch to bank1, the pop and POPAX later will
	 * change bank1's sp, not bank0's sp
	 */
	add sp, sp, 8

	/* switch back to banked reg, only ilink can be used */
	lr ilink, [_ARC_V2_STATUS32]
	or ilink, ilink, _ARC_V2_STATUS32_RB(1)
	kflag ilink
	lr sp, [_ARC_V2_USER_SP]

	POPAX ilink, _ARC_V2_USER_SP
	pop ilink
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

#if CONFIG_RGF_NUM_BANKS != 1
	/* restore lp_count, lp_start, lp_end from r23-r25 */
	mov lp_count,r23
	sr r24, [_ARC_V2_LP_START]
	sr r25, [_ARC_V2_LP_END]
#endif
	_dec_int_nest_counter r0, r1

	_check_nest_int_by_irq_act r0, r1

	jne _firq_no_reschedule

#ifdef CONFIG_STACK_SENTINEL
	bl z_check_stack_sentinel
#endif

#ifdef CONFIG_PREEMPT_ENABLED

#ifdef CONFIG_SMP
	bl z_arch_smp_switch_in_isr
	/* r0 points to new thread, r1 points to old thread */
	brne r0, 0, _firq_reschedule
#else
	mov_s r1, _kernel
	ld_s r2, [r1, _kernel_offset_to_current]

	/* Check if the current thread (in r2) is the cached thread */
	ld_s r0, [r1, _kernel_offset_to_ready_q_cache]
	brne r0, r2, _firq_reschedule
#endif
	/* fall to no rescheduling */

#endif /* CONFIG_PREEMPT_ENABLED */

.balign 4
_firq_no_reschedule:
	pop sp

	/*
	 * Keeping this code block close to those that use it allows using brxx
	 * instruction instead of a pair of cmp and bxx
	 */
#if CONFIG_RGF_NUM_BANKS == 1
	_pop_irq_stack_frame
#endif
	rtie

#ifdef CONFIG_PREEMPT_ENABLED

.balign 4
_firq_reschedule:
	pop sp

#if CONFIG_RGF_NUM_BANKS != 1
#ifdef CONFIG_SMP
/*
 * save r0, r1 in irq stack for a while, as they will be changed by register
 * bank switch
 */
	_get_curr_cpu_irq_stack r2
	st r0, [r2, -4]
	st r1, [r2, -8]
#endif
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
#ifdef CONFIG_SMP
/*
 * load r0, r1 from irq stack
 */
	_get_curr_cpu_irq_stack r2
	ld r0, [r2, -4]
	ld r1, [r2, -8]
#endif
#endif

#ifdef CONFIG_SMP
	mov_s r2, r1
#else
	mov_s r1, _kernel
	ld_s r2, [r1, _kernel_offset_to_current]
#endif
	_save_callee_saved_regs

	st _CAUSE_FIRQ, [r2, _thread_offset_to_relinquish_cause]

#ifdef CONFIG_SMP
	mov_s r2, r0
#else
	ld_s r2, [r1, _kernel_offset_to_ready_q_cache]
	st_s r2, [r1, _kernel_offset_to_current]
#endif

#ifdef CONFIG_ARC_STACK_CHECKING
	_load_stack_check_regs
#endif
	/*
	 * _load_callee_saved_regs expects incoming thread in r2.
	 * _load_callee_saved_regs restores the stack pointer.
	 */
	_load_callee_saved_regs

#if defined(CONFIG_MPU_STACK_GUARD) || defined(CONFIG_USERSPACE)
	push_s r2
	mov_s r0, r2
	bl configure_mpu_thread
	pop_s r2
#endif

#if defined(CONFIG_USERSPACE)
/*
 * see comments in regular_irq.S
 */
	lr r0, [_ARC_V2_AUX_IRQ_ACT]
	bclr r0, r0, 31
	sr r0, [_ARC_V2_AUX_IRQ_ACT]
#endif
	ld r3, [r2, _thread_offset_to_relinquish_cause]

	breq r3, _CAUSE_RIRQ, _firq_return_from_rirq
	nop_s
	breq r3, _CAUSE_FIRQ, _firq_return_from_firq
	nop_s

	/* fall through */

.balign 4
_firq_return_from_coop:
	/* pc into ilink */
	pop_s r0
	mov_s ilink, r0

	pop_s r0 /* status32 into r0 */
	sr r0, [_ARC_V2_STATUS32_P0]

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
