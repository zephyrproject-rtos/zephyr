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
#include <linker/sections.h>
#include <arch/cpu.h>
#include <swap_macros.h>

GTEXT(_rirq_enter)
GTEXT(_rirq_exit)
GTEXT(_rirq_newthread_switch)

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
         | BLINK register in arch_switch()
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

/* the ISR will be handled in separate interrupt stack,
 * so stack checking must be diabled, or exception will
 * be caused
 */
	_disable_stack_checking r2
	clri

	/* check whether irq stack is used, if
	 * not switch to isr stack
	 */
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

	jne _rirq_no_switch

	/* sp is struct k_thread **old of z_arc_switch_in_isr
	 * which is a wrapper of z_get_next_switch_handle.
	 * r0 contains the 1st thread in ready queue. if
	 * it equals _current(r2) ,then do swap, or no swap.
	 */
	_get_next_switch_handle

	cmp r0, r2
	beq _rirq_no_switch

#ifdef CONFIG_ARC_SECURE_FIRMWARE
	/* here need to remember SEC_STAT.IRM bit */
	lr r3, [_ARC_V2_SEC_STAT]
	push_s r3
#endif

	/* r2 is old thread */
	_irq_store_old_thread_callee_regs

	st _CAUSE_RIRQ, [r2, _thread_offset_to_relinquish_cause]

	/* mov new thread (r0) to r2 */
	mov r2, r0

/* _rirq_newthread_switch required by exception handling */
.align 4
_rirq_newthread_switch:

	_load_new_thread_callee_regs

	breq r3, _CAUSE_RIRQ, _rirq_switch_from_rirq
	nop_s
	breq r3, _CAUSE_FIRQ, _rirq_switch_from_firq
	nop_s

	/* fall through */

.align 4
_rirq_switch_from_coop:

	/* for a cooperative switch, it's not in irq, so
	 * need to set some regs for irq return
	 */
	_set_misc_regs_irq_switch_from_coop

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

#ifdef CONFIG_INSTRUMENT_THREAD_SWITCHING
	push_s blink

	bl z_thread_mark_switched_in

	pop_s blink
#endif
	/* stack now has the IRQ stack frame layout, pointing to sp */
	/* rtie will pop the rest from the stack */
	rtie

.align 4
_rirq_switch_from_firq:
_rirq_switch_from_rirq:

	_set_misc_regs_irq_switch_from_irq

#ifdef CONFIG_INSTRUMENT_THREAD_SWITCHING
	push_s blink

	bl z_thread_mark_switched_in

	pop_s blink
#endif
_rirq_no_switch:
	rtie
