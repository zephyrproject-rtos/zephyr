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
#include <linker/sections.h>
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
	j _isr_demux



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

	jne _firq_no_switch

	/* sp is struct k_thread **old of z_arc_switch_in_isr
	 * which is a wrapper of z_get_next_switch_handle.
	 * r0 contains the 1st thread in ready queue. if
	 * it equals _current(r2) ,then do swap, or no swap.
	 */
	_get_next_switch_handle

	cmp r0, r2
	bne _firq_switch

	/* fall to no switch */

.align 4
_firq_no_switch:
	/* restore interrupted context' sp */
	pop sp
	/*
	 * Keeping this code block close to those that use it allows using brxx
	 * instruction instead of a pair of cmp and bxx
	 */
#if CONFIG_RGF_NUM_BANKS == 1
	_pop_irq_stack_frame
#endif
	rtie

.align 4
_firq_switch:
	/* restore interrupted context' sp */
	pop sp

#if CONFIG_RGF_NUM_BANKS != 1
/*
 * save r0, r2 in irq stack for a while, as they will be changed by register
 * bank switch
 */
	_get_curr_cpu_irq_stack r1
	st r0, [r1, -4]
	st r2, [r1, -8]

	/*
	 * We know there is no interrupted interrupt of lower priority at this
	 * point, so when switching back to register bank 0, it will contain the
	 * registers from the interrupted thread.
	 */
#if defined(CONFIG_USERSPACE)
/* when USERSPACE is configured, here need to consider the case where firq comes
 * out in user mode, according to ARCv2 ISA and nsim, the following micro ops
 * will be executed:
 *	sp<-reg bank1'sp
 *      switch between sp and _ARC_V2_USER_SP
 * then:
 *      sp is the sp of kernel stack of interrupted thread
 *      _ARC_V2_USER_SP is reg bank1'sp
 *      the sp of user stack of interrupted thread is reg bank0'sp
 * if firq comes out in kernel mode, the following micro ops will be executed:
 *      sp<-reg bank'sp
 * so, sw needs to do necessary handling to set up the correct sp
 */
	lr r0, [_ARC_V2_AUX_IRQ_ACT]
	bbit0 r0, 31, _firq_from_kernel
	aex sp, [_ARC_V2_USER_SP]
	lr r0, [_ARC_V2_STATUS32]
	and r0, r0, ~_ARC_V2_STATUS32_RB(7)
	kflag r0
	aex sp, [_ARC_V2_USER_SP]
	b _firq_create_irq_stack_frame
_firq_from_kernel:
#endif
	/* chose register bank #0 */
	lr r0, [_ARC_V2_STATUS32]
	and r0, r0, ~_ARC_V2_STATUS32_RB(7)
	kflag r0

_firq_create_irq_stack_frame:
	/* we're back on the outgoing thread's stack */
	_create_irq_stack_frame

	/*
	 * In a FIRQ, STATUS32 of the outgoing thread is in STATUS32_P0 and the
	 * PC in ILINK: save them in status32/pc respectively.
	 */

	lr r0, [_ARC_V2_STATUS32_P0]
	st_s r0, [sp, ___isf_t_status32_OFFSET]

	st ilink, [sp, ___isf_t_pc_OFFSET] /* ilink into pc */
/*
 * load r0, r2 from irq stack
 */
	_get_curr_cpu_irq_stack r1
	ld r0, [r1, -4]
	ld r2, [r1, -8]
#endif
	/* r2 is old thread */
	_irq_store_old_thread_callee_regs

	st _CAUSE_FIRQ, [r2, _thread_offset_to_relinquish_cause]

	/* mov new thread (r0) to r2 */

	mov r2, r0
	_load_new_thread_callee_regs

	breq r3, _CAUSE_RIRQ, _firq_switch_from_rirq
	nop_s
	breq r3, _CAUSE_FIRQ, _firq_switch_from_firq
	nop_s

	/* fall through */

.align 4
_firq_switch_from_coop:

	_set_misc_regs_irq_switch_from_coop

	/* pc into ilink */
	pop_s r0
	mov ilink, r0

	pop_s r0 /* status32 into r0 */
	sr r0, [_ARC_V2_STATUS32_P0]

#ifdef CONFIG_INSTRUMENT_THREAD_SWITCHING
	push_s blink

	bl z_thread_mark_switched_in

	pop_s blink
#endif
	rtie

.align 4
_firq_switch_from_rirq:
_firq_switch_from_firq:

	_set_misc_regs_irq_switch_from_irq

	_pop_irq_stack_frame

	ld ilink, [sp, -4] /* status32 into ilink */
	sr ilink, [_ARC_V2_STATUS32_P0]
	ld ilink, [sp, -8] /* pc into ilink */

#ifdef CONFIG_INSTRUMENT_THREAD_SWITCHING
	push_s blink

	bl z_thread_mark_switched_in

	pop_s blink
#endif
	/* LP registers are already restored, just switch back to bank 0 */
	rtie
