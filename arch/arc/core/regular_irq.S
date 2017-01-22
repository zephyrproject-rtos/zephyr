/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Handling of transitions to-and-from regular IRQs (RIRQ)
 *
 * This module implements the code for handling entry to and exit from regular
 * IRQs.
 *
 * See isr_wrapper.S for details.
 */

#include <kernel_structs.h>
#include <offsets_short.h>
#include <toolchain.h>
#include <arch/cpu.h>
#include <swap_macros.h>

GTEXT(_rirq_enter)
GTEXT(_rirq_exit)
GTEXT(_rirq_common_interrupt_swap)

#if 0 /* TODO: when FIRQ is not present, all would be regular */
#define NUM_REGULAR_IRQ_PRIO_LEVELS CONFIG_NUM_IRQ_PRIO_LEVELS
#else
#define NUM_REGULAR_IRQ_PRIO_LEVELS (CONFIG_NUM_IRQ_PRIO_LEVELS-1)
#endif
/* note: the above define assumes that prio 0 IRQ is for FIRQ, and
 * that all others are regular interrupts.
 * TODO: Revist this if FIRQ becomes configurable.
 */

#if NUM_REGULAR_IRQ_PRIO_LEVELS > 1
#error "nested regular interrupts are not supported."
/*
 * Nesting of Regularing interrupts is not yet supported.
 * Set CONFIG_NUM_IRQ_PRIO_LEVELS to 2 even if SOC supports more.
 */
#endif


/**
 *
 * @brief Work to be done before handing control to an IRQ ISR
 *
 * The processor pushes automatically all registers that need to be saved.
 * However, since the processor always runs at kernel privilege there is no
 * automatic switch to the IRQ stack: this must be done in software.
 *
 * Assumption by _isr_demux: r3 is untouched by _rirq_enter.
 *
 * @return N/A
 */

SECTION_FUNC(TEXT, _rirq_enter)

	mov r1, _kernel
#ifdef CONFIG_ARC_STACK_CHECKING
	/* disable stack checking */
	lr r2, [_ARC_V2_STATUS32]
	bclr r2, r2, _ARC_V2_STATUS32_SC_BIT
	kflag r2
#endif
	ld_s r2, [r1, _kernel_offset_to_current]
#if NUM_REGULAR_IRQ_PRIO_LEVELS == 1
	st sp, [r2, _thread_offset_to_sp]
	ld sp, [r1, _kernel_offset_to_irq_stack]
#else
#error regular irq nesting is not implemented
#endif
	j _isr_demux


/**
 *
 * @brief Work to be done exiting an IRQ
 *
 * @return N/A
 */

SECTION_FUNC(TEXT, _rirq_exit)

#ifdef CONFIG_PREEMPT_ENABLED

	mov r1, _kernel
	ld_s r2, [r1, _kernel_offset_to_current]

	/*
	 * Lock interrupts to ensure kernel queues do not change from this
	 * point on until return from interrupt.
	 */

	clri

#if NUM_REGULAR_IRQ_PRIO_LEVELS > 1
	/* check if we're a nested interrupt: if so, let the interrupted interrupt
	 * handle the reschedule */

	lr r3, [_ARC_V2_AUX_IRQ_ACT]
	ffs r0, r3

	asl r0, 1, r0

	/* the OS on ARCv2 always runs in kernel mode, so assume bit31 [U] in
	 * AUX_IRQ_ACT is always 0: if the contents of AUX_IRQ_ACT is greater
	 * than FFS(AUX_IRQ_ACT), it means that another bit is set so an
	 * interrupt was interrupted.
	 */

	cmp r0, r3
	brgt _rirq_return_from_rirq
#endif

	/*
	 * Non-preemptible thread ? Do not schedule (see explanation of
	 * preempt field in kernel_struct.h).
	 */
	ldh_s r0, [r2, _thread_offset_to_preempt]
	mov r3, _NON_PREEMPT_THRESHOLD
	brhs.d r0, r3, _rirq_no_reschedule

	/*
	 * Both (a)reschedule and (b)non-reschedule cases need to load the
	 * current thread's stack, but don't have to use it until the decision
	 * is taken: load the delay slots with the 'load stack pointer'
	 * instruction.
	 *
	 * a) needs to load it to save outgoing context.
	 * b) needs to load it to restore the interrupted context.
	 */

	ld sp, [r2, _thread_offset_to_sp]

	/* check if the current thread needs to be rescheduled */
	ld_s r0, [r1, _kernel_offset_to_ready_q_cache]
	breq r0, r2, _rirq_no_reschedule

	/* cached thread to run is in r0, fall through */

.balign 4
_rirq_reschedule:

	/* _save_callee_saved_regs expects outgoing thread in r2 */
	_save_callee_saved_regs

	st _CAUSE_RIRQ, [r2, _thread_offset_to_relinquish_cause]

	/* incoming thread is in r0: it becomes the new 'current' */
	mov r2, r0
	st_s r2, [r1, _kernel_offset_to_current]

.balign 4
_rirq_common_interrupt_swap:
	/* r2 contains pointer to new thread */

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

	breq r3, _CAUSE_RIRQ, _rirq_return_from_rirq
	nop
	breq r3, _CAUSE_FIRQ, _rirq_return_from_firq
	nop

	/* fall through */

.balign 4
_rirq_return_from_coop:

	/* status32 and pc (blink) are already on the stack in the right order */

	/* update status32.ie (explanation in firq_exit:_firq_return_from_coop) */
	ld_s r0, [sp, 4]
	ld_s r3, [r2, _thread_offset_to_intlock_key]
	st  0, [r2, _thread_offset_to_intlock_key]
	cmp r3, 0
	or.ne r0, r0, _ARC_V2_STATUS32_IE
	st_s r0, [sp, 4]

	/* carve fake stack */

	/*
	 * a) status32/pc are already on the stack
	 * b) a real value will be pushed in r0
	 */
	sub sp, sp, (___isf_t_SIZEOF - 12)

	/* push return value on stack */
	ld_s r0, [r2, _thread_offset_to_return_value]
	push_s r0

	/*
	 * r13 is part of both the callee and caller-saved register sets because
	 * the processor is only able to save registers in pair in the regular
	 * IRQ prologue. r13 thus has to be set to its correct value in the IRQ
	 * stack frame.
	 */
	st_s r13, [sp, ___isf_t_r13_OFFSET]

	/* stack now has the IRQ stack frame layout, pointing to r0 */

	/* fall through to rtie instruction */

.balign 4
_rirq_return_from_firq:
_rirq_return_from_rirq:

	/* rtie will pop the rest from the stack */

	/* fall through to rtie instruction */

#endif /* CONFIG_PREEMPT_ENABLED */

.balign 4
_rirq_no_reschedule:

	rtie
