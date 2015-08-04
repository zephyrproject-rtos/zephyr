/* isr_wrapper.S - wrapper around ISRs with logic for context switching */

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

/*
DESCRIPTION

Wrapper installed in vector table for handling dynamic interrupts that accept
a parameter.
 */

#define _ASMLANGUAGE

#include <offsets.h>
#include <toolchain.h>
#include <sections.h>
#include <sw_isr_table.h>
#include <nano_private.h>
#include <arch/cpu.h>

GTEXT(_isr_enter)
GTEXT(_isr_demux)

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
          [jli_base]
          [ldi_base]
          [ei_base]
           lp_count
           lp_start
            lp_end
            blink
             r13
             ...
    sp ->    r0

  low address

  [registers not taken into account in the current implementation]

The context switch code adopts this standard so that it is easier to follow:

  - r1 contains _nanokernel ASAP and is not overwritten over the lifespan of
    the functions.
  - r2 contains _nanokernel.current ASAP, and the incoming thread when we
    transition from outgoing context to incoming context

Not loading _nanokernel into r0 allows loading _nanokernel without stomping on
the parameter in r0 in _Swap().


ARCv2 processors have two kinds of interrupts: fast (FIRQ) and regular. The
official documentation calls the regular interrupts 'IRQs', but the internals
of the kernel call them 'RIRQs' to differentiate from the 'irq' subsystem,
which is the interrupt API/layer of abstraction.

FIRQs can be used to allow ISRs to run without having to save any context,
since they work with a second register bank. However, they can be somewhat more
limited than RIRQs since the register bank does not copy every possible
register that is needed to implement all available instructions: an example is
that the 'loop' registers (lp_count, lp_end, lp_start) are not present in the
second bank. The kernel thus takes upon itself to save these extra registers,
if the FIRQ is made known to the kernel. It is possible for a FIRQ to operate
outside of the kernel, but care must be taken to only use instructions that
only use the banked registers. RIRQs must always use the kernel's interrupt
entry and exit mechanisms.

The kernel is able to handle transitions to and from FIRQ, RIRQ and threads
(fibers/task). The contexts are saved 'lazily': the minimum amount of work is
done upfront, and the rest is done when needed:

o RIRQ

  All needed regisers to run C code in the ISR are saved automatically
  on the outgoing context's stack: loop, status32, pc, and the caller-
  saved GPRs. That stack frame layout is pre-determined. If returning
  to a fiber, the stack is popped and no registers have to be saved by
  the kernel. If a context switch is required, the callee-saved GPRs
  are then saved in the context control structure (CCS).

o FIRQ

  First, a FIRQ can be interrupting a lower-priority RIRQ: if this is the case,
  the FIRQ does not take a scheduling decision and leaves it the RIRQ to
  handle. This limits the amount of code that has to run at interrupt-level.

  GPRs are banked, loop registers are saved in a global structure upon
  interrupt entry. If returning to a fiber, loop registers are poppped and the
  CPU switches back to bank 0 for the GPRs. If a context switch is
  needed, at this point only are all the registers saved. First, a
  stack frame with the same layout as the automatic RIRQ one is created
  and then the callee-saved GPRs are saved in the CCS. status32_p0 and
  ilink are saved in this case, not status32 and pc.

  To create the stack frame, the FIRQ handling code must first go back to using
  bank0 of registers, since that is where the registers containing the exiting
  thread are saved. Care must be taken not to touch any register before saving
  them: the only one usable at that point is the stack pointer.

o coop

  When a coop context switch is done, the callee-saved registers are
  saved in the CCS. The other GPRs do not need to be saved, since the
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

  The processor is back to using bank0, not bank1 anymore, because it had to
  save the outgoing context from bank0, and now has to load the incoming one
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
	lr r0, [_ARC_V2_AUX_IRQ_ACT]
	ffs r0, r0
	cmp r0, 0

	mov.z r3, _firq_exit
	mov.z r2, _firq_enter
	mov.nz r3, _rirq_exit
	mov.nz r2, _rirq_enter

	j_s.nd [r2]

/* when getting here, r3 contains the interrupt exit stub to call */
SECTION_FUNC(TEXT, _isr_demux)
	push_s r3
	lr r0, [_ARC_V2_ICAUSE]
	sub r0, r0, 16

	mov r1, _sw_isr_table
	add3 r0, r1, r0   /* table entries are 8-bytes wide */

	ld r1, [r0, 4] /* ISR into r1 */
	jl_s.d [r1]
	ld_s r0, [r0] /* delay slot: ISR parameter into r0  */

	/* back from ISR, jump to exit stub */
	pop_s r3
	j_s.nd [r3]
	nop
