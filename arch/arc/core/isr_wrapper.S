/*
 * Copyright (c) 2014-2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Wrapper around ISRs with logic for context switching
 *
 *
 * Wrapper installed in vector table for handling dynamic interrupts that accept
 * a parameter.
 */

#include <offsets_short.h>
#include <toolchain.h>
#include <sections.h>
#include <sw_isr_table.h>
#include <kernel_structs.h>
#include <arch/cpu.h>

GTEXT(_isr_enter)
GTEXT(_isr_demux)

#if CONFIG_RGF_NUM_BANKS == 1
GDATA(saved_r0)

	.balign 4
SECTION_VAR(BSS, saved_r0)
        .word 0
#endif

#if defined(CONFIG_SYS_POWER_MANAGEMENT)
GTEXT(_sys_power_save_idle_exit)
#endif

/*
The symbols in this file are not real functions, and neither are
_rirq_enter/_firq_enter: they are jump points.

The flow is the following:

ISR -> _isr_enter -- + -> _rirq_enter -> _isr_demux -> ISR -> _rirq_exit
                     |
                     + -> _firq_enter -> _isr_demux -> ISR -> _firq_exit

Context switch explanation:

The context switch code is spread in these files:

  isr_wrapper.s, swap.s, swap_macros.s, fast_irq.s, regular_irq.s

IRQ stack frame layout:

  high address

           status32
              pc
           lp_count
           lp_start
            lp_end
            blink
             r13
             ...
    sp ->    r0

  low address

Registers not taken into account in the current implementation.
          jli_base
          ldi_base
          ei_base
          accl
          acch

The context switch code adopts this standard so that it is easier to follow:

  - r1 contains _kernel ASAP and is not overwritten over the lifespan of
    the functions.
  - r2 contains _kernel.current ASAP, and the incoming thread when we
    transition from outgoing thread to incoming thread

Not loading _kernel into r0 allows loading _kernel without stomping on
the parameter in r0 in _Swap().


ARCv2 processors have two kinds of interrupts: fast (FIRQ) and regular. The
official documentation calls the regular interrupts 'IRQs', but the internals
of the kernel call them 'RIRQs' to differentiate from the 'irq' subsystem,
which is the interrupt API/layer of abstraction.

For FIRQ, there are two cases, depending upon the value of
CONFIG_RGF_NUM_BANKS.

CONFIG_RGF_NUM_BANKS==1 case:
Scratch registers are pushed onto the current stack just as they are with
RIRQ. See the above frame layout. Unlike RIRQ, the status32_p0 and ilink
registers are where status32 and the program counter are located, so these
need to be pushed.

CONFIG_RGF_NUM_BANKS!=1 case:
The FIRQ handler has its own register bank for general purpose registers,
and thus it doesn't have to save them on a stack. The 'loop' registers
(lp_count, lp_end, lp_start), however, are not present in the
second bank. The handler saves these special registers in unused callee saved
registers (to avoid stack accesses). It is possible to register a FIRQ
handler that operates outside of the kernel, but care must be taken to only
use instructions that only use the banked registers.

The kernel is able to handle transitions to and from FIRQ, RIRQ and threads
(fibers/task). The contexts are saved 'lazily': the minimum amount of work is
done upfront, and the rest is done when needed:

o RIRQ

  All needed regisers to run C code in the ISR are saved automatically
  on the outgoing thread's stack: loop, status32, pc, and the caller-
  saved GPRs. That stack frame layout is pre-determined. If returning
  to a fiber, the stack is popped and no registers have to be saved by
  the kernel. If a context switch is required, the callee-saved GPRs
  are then saved in the thread control structure (TCS).

o FIRQ

  First, a FIRQ can be interrupting a lower-priority RIRQ: if this is the case,
  the FIRQ does not take a scheduling decision and leaves it the RIRQ to
  handle. This limits the amount of code that has to run at interrupt-level.

  CONFIG_RGF_NUM_BANKS==1 case:
  Registers are saved on the stack frame just as they are for RIRQ.
  Context switch can happen just as it does in the RIRQ case, however,
  if the FIRQ interrupted a RIRQ, the FIRQ will return from interrupt and
  let the RIRQ do the context switch.  At entry, one register is needed in order
  to have code to save other registers. r0 is saved first in a global called
  saved_r0.

  CONFIG_RGF_NUM_BANKS!=1 case:
  During early initialization, the sp in the 2nd register bank is made to
  refer to _firq_stack. This allows for the FIRQ handler to use its own stack.
  GPRs are banked, loop registers are saved in unused callee saved regs upon
  interrupt entry. If returning to a fiber, loop registers are restored and the
  CPU switches back to bank 0 for the GPRs. If a context switch is
  needed, at this point only are all the registers saved. First, a
  stack frame with the same layout as the automatic RIRQ one is created
  and then the callee-saved GPRs are saved in the TCS. status32_p0 and
  ilink are saved in this case, not status32 and pc.
  To create the stack frame, the FIRQ handling code must first go back to using
  bank0 of registers, since that is where the registers containing the exiting
  thread are saved. Care must be taken not to touch any register before saving
  them: the only one usable at that point is the stack pointer.

o coop

  When a coop context switch is done, the callee-saved registers are
  saved in the TCS. The other GPRs do not need to be saved, since the
  compiler has already placed them on the stack.

For restoring the contexts, there are six cases. In all cases, the
callee-saved registers of the incoming thread have to be restored. Then, there
are specifics for each case:

From coop:

  o to coop

    Restore interrupt lock level and do a normal function call return.

  o to any irq

    The incoming interrupted thread has an IRQ stack frame containing the
    caller-saved registers that has to be popped. status32 has to be restored,
    then we jump to the interrupted instruction.

From FIRQ:

  When CONFIG_RGF_NUM_BANKS==1, context switch is done as it is for RIRQ.
  When CONFIG_RGF_NUM_BANKS!=1, the processor is put back to using bank0,
  not bank1 anymore, because it had to save the outgoing context from bank0,
  and now has to load the incoming one
  into bank0.

  o to coop

    The address of the returning instruction from _Swap() is loaded in ilink and
    the saved status32 in status32_p0, taking care to adjust the interrupt lock
    state desired in status32_p0. The return value is put in r0.

  o to any irq

    The IRQ has saved the caller-saved registers in a stack frame, which must be
    popped, and statu32 and pc loaded in status32_p0 and ilink.

From RIRQ:

  o to coop

    The interrupt return mechanism in the processor expects a stack frame, but
    the outgoing context did not create one. A fake one is created here, with
    only the relevant values filled in: pc, status32 and the return value in r0.

    There is a discrepancy between the ABI from the ARCv2 docs, including the
    way the processor pushes GPRs in pairs in the IRQ stack frame, and the ABI
    GCC uses. r13 should be a callee-saved register, but GCC treats it as
    caller-saved. This means that the processor pushes it in the stack frame
    along with r12, but the compiler does not save it before entering a
    function. So, it is saved as part of the callee-saved registers, and
    restored there, but the processor restores it _a second time_ when popping
    the IRQ stack frame. Thus, the correct value must also be put in the fake
    stack frame when returning to a thread that context switched out
    cooperatively.

  o to any irq

    Both types of IRQs already have an IRQ stack frame: simply return from
    interrupt.
 */

SECTION_FUNC(TEXT, _isr_enter)
#if CONFIG_RGF_NUM_BANKS == 1
	st r0,[saved_r0]
#endif
	lr r0, [_ARC_V2_AUX_IRQ_ACT]
	ffs r0, r0
	cmp r0, 0
#if CONFIG_RGF_NUM_BANKS == 1
	bnz rirq_path
	/* 1-register bank FIRQ handling must save registers on stack */
	lr r0,[_ARC_V2_STATUS32_P0]
	push_s r0
	mov r0,ilink
	push_s r0
	mov r0,lp_count
	push_s r0
	lr r0, [_ARC_V2_LP_START]
	push_s r0
	lr r0, [_ARC_V2_LP_END]
	push_s r0
	push_s blink
	push_s r13
	push_s r12
	push r11
	push r10
	push r9
	push r8
	push r7
	push r6
	push r5
	push r4
	push_s r3
	push_s r2
	push_s r1
	ld r0,[saved_r0]
	push_s r0
	mov r3, _firq_exit
	mov r2, _firq_enter
	j_s [r2]
rirq_path:
	mov r3, _rirq_exit
	mov r2, _rirq_enter
	j_s [r2]
#else
	mov.z r3, _firq_exit
	mov.z r2, _firq_enter
	mov.nz r3, _rirq_exit
	mov.nz r2, _rirq_enter
	j_s [r2]
#endif

#ifdef CONFIG_KERNEL_EVENT_LOGGER_SLEEP
GTEXT(_sys_k_event_logger_exit_sleep)

.macro log_sleep_k_event
	clri r0 /* do not interrupt event logger operations */
	push_s r0
	push_s blink
	jl _sys_k_event_logger_exit_sleep
	pop_s blink
	pop_s r0
	seti r0
.endm
#else
	#define log_sleep_k_event
#endif
#if defined(CONFIG_KERNEL_EVENT_LOGGER_INTERRUPT)
GTEXT(_sys_k_event_logger_interrupt)

.macro log_interrupt_k_event
	clri r0 /* do not interrupt event logger operations */
	push_s r0
	push_s blink
	jl _sys_k_event_logger_interrupt
	pop_s blink
	pop_s r0
	seti r0
.endm
#else
	#define log_interrupt_k_event
#endif

#if defined(CONFIG_SYS_POWER_MANAGEMENT)
.macro exit_tickless_idle
	clri r0 /* do not interrupt exiting tickless idle operations */
	push_s r1
	push_s r0
	mov_s r1, _kernel
	ld_s r0, [r1, _kernel_offset_to_idle] /* requested idle duration */
	breq r0, 0, _skip_sys_power_save_idle_exit

	st 0, [r1, _kernel_offset_to_idle] /* zero idle duration */
	push_s blink
	jl _sys_power_save_idle_exit
	pop_s blink

_skip_sys_power_save_idle_exit:
	pop_s r0
	pop_s r1
	seti r0
.endm
#else
	#define exit_tickless_idle
#endif

/* when getting here, r3 contains the interrupt exit stub to call */
SECTION_FUNC(TEXT, _isr_demux)
	push_s r3

	/* cannot be done before this point because we must be able to run C */
	/* r0 is available to be stomped here, and exit_tickless_idle uses it */
	exit_tickless_idle
	log_interrupt_k_event
	log_sleep_k_event

	lr r0, [_ARC_V2_ICAUSE]
	sub r0, r0, 16

	mov r1, _sw_isr_table
	add3 r0, r1, r0   /* table entries are 8-bytes wide */

	ld_s r1, [r0, 4] /* ISR into r1 */
	jl_s.d [r1]
	ld_s r0, [r0] /* delay slot: ISR parameter into r0  */

	/* back from ISR, jump to exit stub */
	pop_s r3
	j_s [r3]
	nop
