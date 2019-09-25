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

/*

===========================================================
 RETURN FROM INTERRUPT TO COOPERATIVE THREAD
===========================================================

That's a special case because:
 1. We return from IRQ handler to a cooperative thread
 2. During IRQ handling context switch did happen
 3. Returning to a thread which previously gave control
    to another thread because of:
     - Calling k_sleep()
     - Explicitly yielding
     - Bumping into locked sync primitive etc

What (3) means is before passing control to another thread our thread
in question:
 a. Stashed all precious caller-saved registers on its stack
 b. Pushed return address to the top of the stack as well

That's how thread's stack looks like right before jumping to another thread:
----------------------------->8---------------------------------
PRE-CONTEXT-SWITCH STACK

  lower_addr, let's say: 0x1000

         --------------------------------------
  SP ->  | Return address; PC (Program Counter), in fact value taken from
         | BLINK register in z_arch_switch()
         --------------------------------------
         | STATUS32 value, we explicitly save it here for later usage, read-on
         --------------------------------------
         | Caller-saved registers: some of R0-R12
         --------------------------------------
         |...
         |...

  higher_addr, let's say: 0x2000
----------------------------->8---------------------------------

When context gets switched the kernel saves callee-saved registers in the
thread's stack right on top of pre-switch contents so that's what we have:
----------------------------->8---------------------------------
POST-CONTEXT-SWITCH STACK

  lower_addr, let's say: 0x1000

         --------------------------------------
SP ->    | Callee-saved registers: see struct _callee_saved_stack{}
         |  |- R13
         |  |- R14
         |  | ...
         |  \- FP
         |   ...
         --------------------------------------
         | Return address; PC (Program Counter)
         --------------------------------------
         | STATUS32 value
         --------------------------------------
         | Caller-saved registers: some of R0-R12
         --------------------------------------
         |...
         |...

  higher_addr, let's say: 0x2000
----------------------------->8---------------------------------

So how do we return in such a complex scenario.

First we restore callee-saved regs with help of _load_callee_saved_regs().
Now we're back to PRE-CONTEXT-SWITCH STACK (see above).

Logically our next step is to load return address from the top of the stack
and jump to that address to continue execution of the desired thread, but
we're still in interrupt handling mode and the only way to return to normal
execution mode is to execute "rtie" instruction. And here we need to deal
with peculiarities of return from IRQ on ARCv2 cores.

Instead of simple jump to a return address stored in the tip of thread's stack
(with subsequent interrupt enable) ARCv2 core additionally automatically
restores some registers from stack. Most important ones are
PC ("Program Counter") which holds address of the next instruction to execute
and STATUS32 which holds imortant flags including global interrupt enable,
zero, carry etc.

To make things worse depending on ARC core configuration and run-time setup
of certain features different set of registers will be restored.

Typically those same registers are automatically saved on stack on entry to
an interrupt, but remember we're returning to the thread which was
not interrupted by interrupt and so on its stack there're no automatically
saved registers, still inevitably on RTIE execution register restoration
will happen. So if we do nothing special we'll end-up with that:
----------------------------->8---------------------------------
  lower_addr, let's say: 0x1000

         --------------------------------------
    #    | Return address; PC (Program Counter)
    |    --------------------------------------
    |    | STATUS32 value
    |    --------------------------------------
    |
 sizeof(_irq_stack_frame)
    |
    |    | Caller-saved registers: R0-R12
    V    --------------------------------------
         |...
  SP ->  | < Some data on thread's stack>
         |...

  higher_addr, let's say: 0x2000
----------------------------->8---------------------------------

I.e. we'll go much deeper down the stack over needed return address, read
some value from unexpected location in stack and will try to jump there.
Nobody knows were we end-up then.

To work-around that problem we need to mimic existance of IRQ stack frame
of which we really only need return address obviously to return where we
need to. For that we just shift SP so that it points sizeof(_irq_stack_frame)
above like that:
----------------------------->8---------------------------------
  lower_addr, let's say: 0x1000

  SP ->  |
    A    | < Some unrelated data >
    |    |
    |
 sizeof(_irq_stack_frame)
    |
    |    --------------------------------------
    |    | Return address; PC (Program Counter)
    |    --------------------------------------
    #    | STATUS32 value
         --------------------------------------
         | Caller-saved registers: R0-R12
         --------------------------------------
         |...
         | < Some data on thread's stack>
         |...

  higher_addr, let's say: 0x2000
----------------------------->8---------------------------------

Indeed R0-R13 "restored" from IRQ stack frame will contain garbage but
it makes no difference because we're returning to execution of code as if
we're returning from yet another function call and so we will restore
all needed registers from the stack.

One other important remark here is R13.

CPU hardware automatically save/restore registers in pairs and since we
wanted to save/restore R12 in IRQ stack frame as a caller-saved register we
just happen to do that for R13 as well. But given compiler treats it as
a callee-saved register we save/restore it separately in _callee_saved_stack
structure. And when we restore callee-saved registers from stack we among
other registers recover R13. But later on return from IRQ with RTIE
instruction, R13 will be "restored" again from fake IRQ stack frame and
if we don't copy correct R13 value to fake IRQ stack frame R13 value
will be corrupted.

*/

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
	clri

	/* check whether irq stack is used */
	_check_and_inc_int_nest_counter r0, r1

	bne.d rirq_nest
	mov_s r0, sp

	_get_curr_cpu_irq_stack sp
rirq_nest:
	push_s r0

	seti
	j _isr_demux


/**
 *
 * @brief Work to be done exiting an IRQ
 *
 * @return N/A
 */

SECTION_FUNC(TEXT, _rirq_exit)
	clri

	pop sp

	_dec_int_nest_counter r0, r1

	_check_nest_int_by_irq_act r0, r1

	jne _rirq_no_reschedule

#ifdef CONFIG_STACK_SENTINEL
	bl z_check_stack_sentinel
#endif

#ifdef CONFIG_PREEMPT_ENABLED

#ifdef CONFIG_SMP
	bl z_arch_smp_switch_in_isr
	/* r0 points to new thread, r1 points to old thread */
	cmp_s r0, 0
	beq _rirq_no_reschedule
	mov_s r2, r1
#else
	mov_s r1, _kernel
	ld_s r2, [r1, _kernel_offset_to_current]

	/*
	 * Both (a)reschedule and (b)non-reschedule cases need to load the
	 * current thread's stack, but don't have to use it until the decision
	 * is taken: load the delay slots with the 'load stack pointer'
	 * instruction.
	 *
	 * a) needs to load it to save outgoing context.
	 * b) needs to load it to restore the interrupted context.
	 */

	/* check if the current thread needs to be rescheduled */
	ld_s r0, [r1, _kernel_offset_to_ready_q_cache]
	cmp_s r0, r2
	beq _rirq_no_reschedule

	/* cached thread to run is in r0, fall through */
#endif
.balign 4
_rirq_reschedule:

#ifdef CONFIG_ARC_SECURE_FIRMWARE
	/* here need to remember SEC_STAT.IRM bit */
	lr r3, [_ARC_V2_SEC_STAT]
	push_s r3
#endif
	/* _save_callee_saved_regs expects outgoing thread in r2 */
	_save_callee_saved_regs

	st _CAUSE_RIRQ, [r2, _thread_offset_to_relinquish_cause]

#ifdef CONFIG_SMP
	mov_s r2, r0
#else
	/* incoming thread is in r0: it becomes the new 'current' */
	mov_s r2, r0
	st_s r2, [r1, _kernel_offset_to_current]
#endif

.balign 4
_rirq_common_interrupt_swap:
	/* r2 contains pointer to new thread */

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
 * when USERSPACE is enabled, according to ARCv2 ISA, SP will be switched
 * if interrupt comes out in user mode, and will be recorded in bit 31
 * (U bit) of IRQ_ACT. when interrupt exits, SP will be switched back
 * according to U bit.
 *
 * For the case that context switches in interrupt, the target sp must be
 * thread's kernel stack, no need to do hardware sp switch. so, U bit should
 * be cleared.
 */
	lr r0, [_ARC_V2_AUX_IRQ_ACT]
	bclr r0, r0, 31
	sr r0, [_ARC_V2_AUX_IRQ_ACT]
#endif

	ld r3, [r2, _thread_offset_to_relinquish_cause]

	breq r3, _CAUSE_RIRQ, _rirq_return_from_rirq
	nop_s
	breq r3, _CAUSE_FIRQ, _rirq_return_from_firq
	nop_s

	/* fall through */

.balign 4
_rirq_return_from_coop:

#ifdef CONFIG_ARC_SECURE_FIRMWARE
	/* must return to secure mode, so set IRM bit to 1 */
	lr r0, [_ARC_V2_SEC_STAT]
	bset r0, r0, _ARC_V2_SEC_STAT_IRM_BIT
	sflag r0
#endif

	/*
	 * See verbose explanation of
	 * RETURN FROM INTERRUPT TO COOPERATIVE THREAD above
	 */

	/* carve fake stack */
	sub sp, sp, ___isf_t_pc_OFFSET


	/* reset zero-overhead loops */
	st 0, [sp, ___isf_t_lp_end_OFFSET]

	/*
	 * r13 is part of both the callee and caller-saved register sets because
	 * the processor is only able to save registers in pair in the regular
	 * IRQ prologue. r13 thus has to be set to its correct value in the IRQ
	 * stack frame.
	 */
	st_s r13, [sp, ___isf_t_r13_OFFSET]

	/* stack now has the IRQ stack frame layout, pointing to sp */
	/* rtie will pop the rest from the stack */
	rtie

#endif /* CONFIG_PREEMPT_ENABLED */

.balign 4
_rirq_return_from_firq:
_rirq_return_from_rirq:
#ifdef CONFIG_ARC_SECURE_FIRMWARE
	/* here need to recover SEC_STAT.IRM bit */
	pop_s r3
	sflag r3
#endif
_rirq_no_reschedule:

	rtie
