/* swap.S - thread context switching */

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
This module implements the routines necessary for thread context switching
on ARCv2 CPUs.

See isr_wrapper.S for details.
*/

#define _ASMLANGUAGE

#include <nano_private.h>
#include <offsets.h>
#include <toolchain.h>
#include <arch/cpu.h>
#include <v2/irq.h>
#include "swap_macros.h"

GTEXT(_Swap)

GDATA(_nanokernel)

/*******************************************************************************
*
* _Swap - initiate a cooperative context switch
*
* The _Swap() routine is invoked by various nanokernel services to effect
* a cooperative context context switch.  Prior to invoking _Swap(), the caller
* disables interrupts via nanoCpuIntLock() and the return 'key' is passed as a
* parameter to _Swap(). The key is in fact the value stored in the register
* operand of a CLRI instruction.
*
* It stores the intlock key parameter into current->intlock_key.

* Given that _Swap() is called to effect a cooperative context context switch,
* the caller-saved integer registers are saved on the stack by the function
* call preamble to _Swap(). This creates a custom stack frame that will be
* popped when returning from _Swap(), but is not suitable for handling a return
* from an exception. Thus, the fact that the thread is pending because of a
* cooperative call to _Swap() has to be recorded via the _CAUSE_COOP code in
* the relinquish_cause of the context's tCCS. The _IrqExit()/_FirqExit() code
* will take care of doing the right thing to restore the thread status.
*
* When _Swap() is invoked, we know the decision to perform a context switch or
* not has already been taken and a context switch must happen.
*
* RETURNS: may contain a return value setup by a call to fiberRtnValueSet()
*
* C function prototype:
*
* unsigned int _Swap (unsigned int key);
*
*/

SECTION_FUNC(TEXT, _Swap)

	/* interrupts are locked, interrupt key is in r0 */

	mov r1, _nanokernel
	ld r2, [r1, __tNANO_current_OFFSET]

	/* save intlock key */
	st r0, [r2, __tCCS_intlock_key_OFFSET]
	st _CAUSE_COOP, [r2, __tCCS_relinquish_cause_OFFSET]

	/*
	 * Save status32 and blink on the stack before the callee-saved registers.
	 * This is the same layout as the start of an IRQ stack frame.
	 */
	lr r3, [_ARC_V2_STATUS32]
	push_s r3
	push_s blink

	_save_callee_saved_regs

	ld r2, [r1, __tNANO_fiber_OFFSET]
	breq.nd r2, 0, _swap_to_the_task

.balign 4
_swap_to_a_fiber:

	ld r3, [r2, __tCCS_link_OFFSET]
	b.d _finish_swapping_to_context    /* always execute delay slot */
	st r3, [r1, __tNANO_fiber_OFFSET] /* delay slot */

.balign 4
_swap_to_the_task:

	ld r2, [r1, __tNANO_task_OFFSET]

	/* fall through */

.balign 4
_finish_swapping_to_context:


	/* entering here, r2 contains the new current context */
#if 0
/* don't save flags in tNANO: slower, error-prone, and might not even give
 * a speed boost where it's supposed to */
	ld r3, [r2, __tCCS_flags_OFFSET]
	st r3, [r1, __tNANO_flags_OFFSET]
#endif

	/* XXX - can be moved to delay slot of _CAUSE_RIRQ ? */
	st r2, [r1, __tNANO_current_OFFSET]

	_load_callee_saved_regs

	ld r3, [r2, __tCCS_relinquish_cause_OFFSET]

	breq.nd r3, _CAUSE_RIRQ, _swap_return_from_rirq
	nop
	breq.nd r3, _CAUSE_FIRQ, _swap_return_from_firq
	nop

	/* fall through to _swap_return_from_coop */

.balign 4
_swap_return_from_coop:

	ld r1, [r2, __tCCS_intlock_key_OFFSET]
	st  0, [r2, __tCCS_intlock_key_OFFSET]
	ld r0, [r2, __tCCS_return_value_OFFSET]

	/*
	 * Adjust the stack here in case we go to _return_from_exc: this allows
	 * keeping handling both coop and irq cases in _return_from_exc without
	 * adding extra logic.
	 */
	add_s sp, sp, 8
	lr ilink, [_ARC_V2_STATUS32]
	bbit1 ilink, _ARC_V2_STATUS32_AE_BIT, _return_from_exc
	sub_s sp, sp, 8

	pop_s blink /* pc into blink */
	pop_s r3    /* status32 into r3 */
	kflag r3    /* write status32 */

	j_s.d [blink] /* always execute delay slot */
	seti r1       /* delay slot */


.balign 4
_swap_return_from_rirq:
_swap_return_from_firq:

	_pop_irq_stack_frame

	lr ilink, [_ARC_V2_STATUS32]
	bbit1 ilink, _ARC_V2_STATUS32_AE_BIT, _return_from_exc

	ld ilink, [sp, -4] /* status32 into ilink */
	and ilink, ilink, 0x7ffffffe // keep interrupts disabled until seti
	kflag ilink

	ld ilink, [sp, -8] /* pc into ilink */

	j.d [ilink]
	seti (_ARC_V2_DEF_IRQ_LEVEL | (1 << 4))

.balign 4
_return_from_exc:
	/* put the return address to eret */
	ld ilink, [sp, -8] /* pc into ilink */
	sr ilink, [_ARC_V2_ERET]

	/* put status32 into estatus */
	ld ilink, [sp, -4] /* status32 into ilink */
	sr ilink, [_ARC_V2_ERSTATUS]
	rtie
