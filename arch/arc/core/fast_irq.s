/* fast_irq.s - handling of transitions to-and-from fast IRQs (FIRQ) */

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
This module implements the code for handling entry to and exit from Fast IRQs.

See isr_wrapper.s for details.
*/

#define _ASMLANGUAGE

#include <nanok.h>
#include <offsets.h>
#include <toolchain.h>
#include <nanokernel/cpu.h>
#include "swap_macros.h"

GTEXT(_firq_enter)
GTEXT(_firq_exit)
GTEXT(_firq_stack_setup)
GDATA(_firq_stack)

SECTION_VAR(NOINIT, _firq_stack)
	.space CONFIG_FIRQ_STACK_SIZE

/*******************************************************************************
*
* _firq_enter - work to be done before handing control to a FIRQ ISR
*
* The processor switches to a second register bank so registers from the
* current bank do not have to be preserved yet. The only issue is the LP_START/
* LP_COUNT/LP_END registers, which are not banked.
*
* If all FIRQ ISRs are programmed such that there are no use of the LP
* registers (ie. no LPcc instruction), then the kernel can be configured to
* remove the use of _firq_enter().
*
* When entering a FIRQ, interrupts might as well be locked: the processor is
* running at its highest priority, and cannot be preempted by anything.
*
* Assumption by _isr_demux: r3 is untouched by _firq_enter.
*
* RETURNS: N/A
*/

SECTION_FUNC(TEXT, _firq_enter)

#ifndef CONFIG_FIRQ_NO_LPCC
	/*
	 * Unlike the rest of context switching code, r2 is loaded with something
	 * else than 'current' in this routine: this is to preserve r3 so that it
	 * does not have to be fetched again in _isr_demux.
	 */

	/* save LP_START/LP_COUNT/LP_END variables */
	mov_s r1, _NanoKernel

	/* cannot store lp_count directly to memory */
	mov r2, lp_count
	st lp_count, [r1, __tNANO_firq_regs_OFFSET + __tFirqRegs_lp_count_OFFSET]

	lr r2, [_ARC_V2_LP_START]
	st r2, [r1, __tNANO_firq_regs_OFFSET + __tFirqRegs_lp_start_OFFSET]
	lr r2, [_ARC_V2_LP_END]
	st r2, [r1, __tNANO_firq_regs_OFFSET + __tFirqRegs_lp_end_OFFSET]
#endif

	j @_isr_demux

/*******************************************************************************
*
* _firq_exit - work to be done exiting a FIRQ
*
* RETURNS: N/A
*/

SECTION_FUNC(TEXT, _firq_exit)

	mov_s r1, _NanoKernel
	ld_s r2, [r1, __tNANO_current_OFFSET]

#ifndef CONFIG_FIRQ_NO_LPCC

	/* assumption: r1 contains _NanoKernel, r2 contains the current thread */

	/* restore LP_START/LP_COUNT/LP_END variables */

	/* cannot load lp_count from memory */
	ld r3, [r1, __tNANO_firq_regs_OFFSET + __tFirqRegs_lp_count_OFFSET]
	mov lp_count, r3

	ld r3, [r1, __tNANO_firq_regs_OFFSET + __tFirqRegs_lp_start_OFFSET]
	sr r3, [_ARC_V2_LP_START]
	ld r3, [r1, __tNANO_firq_regs_OFFSET + __tFirqRegs_lp_end_OFFSET]
	sr r3, [_ARC_V2_LP_END]

	/* exiting here: r1/r2 unchanged, r0/r3 destroyed */
#endif

#if CONFIG_NUM_IRQ_PRIO_LEVELS > 1
	/* check if we're a nested interrupt: if so, let the interrupted interrupt
	 * handle the reschedule */

	lr r3, [_ARC_V2_AUX_IRQ_ACT]

	/* Viper on ARCv2 always runs in kernel mode, so assume bit31 [U] in
	 * AUX_IRQ_ACT is always 0: if the contents of AUX_IRQ_ACT is not 1, it
	 * means that another bit is set so an interrupt was interrupted.
	 */

	breq.nd r3, 1, _check_if_current_is_the_task

	rtie

#endif

.balign 4
_check_if_current_is_the_task:

	ld r0, [r2, __tCCS_flags_OFFSET]
	and.f r0, r0, PREEMPTIBLE
	bnz.nd _check_if_a_fiber_is_ready
	rtie

.balign 4
_check_if_a_fiber_is_ready:
	ld r0, [r1, __tNANO_fiber_OFFSET] /* incoming fiber in r0 */
	brne.nd r0, 0, _firq_reschedule
	rtie

.balign 4
_firq_reschedule:

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
	st_s r0, [sp, __tISF_status32_OFFSET]

	st ilink, [sp, __tISF_pc_OFFSET] /* ilink into pc */

	mov_s r1, _NanoKernel
	ld r2, [r1, __tNANO_current_OFFSET]

	_save_callee_saved_regs

	st _CAUSE_FIRQ, [r2, __tCCS_relinquish_cause_OFFSET]

	ld r2, [r1, __tNANO_fiber_OFFSET]

	st r2, [r1, __tNANO_current_OFFSET]
	ld r3, [r2, __tCCS_link_OFFSET]
	st r3, [r1, __tNANO_fiber_OFFSET]

	/*
	 * _load_callee_saved_regs expects incoming thread in r2.
	 * _load_callee_saved_regs restores the stack pointer.
	 */
	_load_callee_saved_regs

	ld r3, [r2, __tCCS_relinquish_cause_OFFSET]

	breq.nd r3, _CAUSE_RIRQ, _firq_return_from_rirq
	nop
	breq.nd r3, _CAUSE_FIRQ, _firq_return_from_firq
	nop

	/* fall through */

.balign 4
_firq_return_from_coop:

	ld r3, [r2, __tCCS_intlock_key_OFFSET]
	st  0, [r2, __tCCS_intlock_key_OFFSET]

	/* pc into ilink */
	pop_s r0
	mov ilink, r0

	pop_s r0 /* status32 into r0 */
	/*
	 * There are only two interrupt lock states: locked and unlocked. When
	 * entering _Swap(), they are always locked, so the IE bit is unset in
	 * status32. If the incoming thread had them locked recursively, it means
	 * that the IE bit should stay unset. The only time the bit has to change
	 * is if they were not locked recursively.
	 */
	and.f r3, r3, (1 << 4)
	or.nz r0, r0, _ARC_V2_STATUS32_IE
	sr r0, [_ARC_V2_STATUS32_P0]

	ld r0, [r2, __tCCS_return_value_OFFSET]
	rtie

.balign 4
_firq_return_from_rirq:
_firq_return_from_firq:

	_pop_irq_stack_frame

	ld ilink, [sp, -4] /* status32 into ilink */
	sr ilink, [_ARC_V2_STATUS32_P0]
	ld ilink, [sp, -8] /* pc into ilink */

	/* fall through to rtie instruction */

.balign 4
_firq_no_reschedule:

	/* LP registers are already restored, just switch back to bank 0 */
	rtie

/*******************************************************************************
*
* _firq_stack_setup - install the FIRQ stack in register bank 1
*
* RETURNS: N/A
*/

SECTION_FUNC(TEXT, _firq_stack_setup)

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

	j_s.nd [blink]
