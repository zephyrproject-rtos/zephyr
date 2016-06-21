/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define _ASMLANGUAGE
#include <arch/nios2/asm.h>
#include <nano_private.h>

/* exports */
GTEXT(_exception)
GTEXT(_exception_exit)
GTEXT(_exception_enter_fault)

/* import */
GTEXT(_Fault)
GTEXT(_Swap)
GTEXT(_exception_try_muldiv)

/* Allows use of r1/at register, otherwise reserved for assembler use */
.set noat

/* Placed into special 'exception' section so that the linker can put this code
 * at ALT_CPU_EXCEPTION_ADDR defined in system.h
 *
 * This is the common entry point for processor exceptions and interrupts from
 * the Internal Interrupt Controller (IIC).
 *
 * If the External (EIC) controller is in use, then we will never get here on
 * behalf of an interrupt, instead the EIC driver will have set up a vector
 * table and the processor will jump directly into the appropriate table
 * entry.
 */
SECTION_FUNC(exception.entry, _exception)
	/* Reserve thread stack space for saving context */
	addi sp, sp, -76

	/* Preserve all caller-saved registers onto the thread's stack */
	stw ra,   0(sp)
	/* Gap here for muldiv handler to store zero register */
	stw r1,   8(sp)
	stw r2,  12(sp)
	stw r3,  16(sp)
	stw r4,  20(sp)
	stw r5,  24(sp)
	stw r6,  28(sp)
	stw r7,  32(sp)
	stw r8,  36(sp)
	stw r9,  40(sp)
	stw r10, 44(sp)
	stw r11, 48(sp)
	stw r12, 52(sp)
	stw r13, 56(sp)
	stw r14, 60(sp)
	stw r15, 64(sp)

	/* Store value of estatus control register */
	rdctl et, estatus
	stw   et, 68(sp)

	/* ea-4 is the address of the instruction when the exception happened,
	 * put this in the stack frame as well
	 */
	addi  r15, ea, -4
	stw   r15, 72(sp)

	/* Figure out whether we are here because of an interrupt or an
	 * exception. If an interrupt, switch stacks and enter IRQ handling
	 * code. If an exception, remain on current stack and enter exception
	 * handing code. From the CPU manual, ipending must be nonzero and
	 * estatis.PIE must be enabled for this to be considered an interrupt.
	 *
	 * Stick ipending in r4 since it will be an arg for _enter_irq
	 */
	rdctl r4, ipending
	beq   r4, zero, not_interrupt
	/* We stashed estatus in et earlier */
	andi  r15, et, 1
	beq   r15, zero, not_interrupt

	/* If we get here, this is an interrupt */

	/* Grab a reference to _nanokernel in r10 so we can determine the
	 * current irq stack pointer
	 */
	movhi r10, %hi(_nanokernel)
	ori r10, r10, %lo(_nanokernel)

	/* Stash a copy of thread's sp in r12 so that we can put it on the IRQ
	 * stack
	 */
	mov r12, sp

	/* Switch to interrupt stack */
	ldw sp, __tNANO_irq_sp_OFFSET(r10)

	/* Store thread stack pointer onto IRQ stack */
	addi sp, sp, -4
	stw r12, 0(sp)

BRANCH_LABEL(on_irq_stack)

#if 0 /* TODO enable interrupt handling code */
	/* Enter C interrupt handling code. Value of ipending will be the
	 * function parameter since we put it in r4
	 */
	call _enter_irq
#endif

	/* Interrupt handler finished and the interrupt should be serviced
	 * now, the appropriate bits in ipending should be cleared */

	/* Get a reference to _nanokernel again in r10 */
	movhi r10, %hi(_nanokernel)
	ori   r10, r10, %lo(_nanokernel)

	/* Determine whether the execution of the ISR requires a context
	 * switch.  If the interrupted thread is PREEMPTIBLE (a task) and
	 * _nanokernel.fiber is non-NULL, a _Swap() needs to occur.
	 */

	/* Check (_nanokernel.current->flags & PREEMPTIBLE), if not
	 * goto no_reschedule
	 */
	ldw  r11, __tNANO_current_OFFSET(r10)
	ldw  r12, __tTCS_flags_OFFSET(r11)
	movi r13, PREEMPTIBLE
	and  r12, r13, r12
	beq  r12, zero, no_reschedule

	/* Check _nanokernel.fiber != NULL, if NULL goto no_reschedule */
	ldw  r11, __tNANO_fiber_OFFSET(r10)
	beq  r11, zero, no_reschedule

	/*
	 * A context reschedule is required: keep the volatile registers of
	 * the interrupted thread on the context's stack.  Utilize
	 * the existing _Swap() primitive to save the remaining
	 * thread's registers (including floating point) and perform
	 * a switch to the new thread.
	 */

	/* We put the thread stack pointer on top of the IRQ stack before
	 * we switched stacks. Restore it to go back to thread stack
	 */
	ldw sp, 0(sp)

	/* Argument to Swap() is estatus since that's the state of the
	 * status register before the exception happened. When coming
	 * out of the context switch we need this info to restore
	 * IRQ lock state. We put this value in et earlier.
	 */
	mov r4, et

	call _Swap
	jmpi _exception_exit

BRANCH_LABEL(not_interrupt)

#if 0 /* TODO enable multiply / divide exception handing */

	/* Since this wasn't an interrupt we're not going to restart the
	 * faulting instruction. If it's an unimplemented math instruction,
	 * the muldiv code will handle it, else we just give up and _Fault.
	 *
	 * We earlier put ea - 4 in the stack frame, replace it with just ea
	 */
	stw ea, 72(sp)

	/* Could be an unimplemented instruction we have to emulate.
	 * Smaller Nios II cores don't have multiply or divide instructions.
	 * This code comes back to either _exception_enter_fault or
	 * _exception_exit
	 */
	jmpi _exception_try_muldiv
#endif

SECTION_FUNC(exception.entry, _exception_enter_fault)
	/* If we get here, the exception wasn't in interrupt or an
	 * unimplemented math instruction. Let _Fault() handle it in
	 * C domain
	 */

	ldw  r4, 0(sp)
	call _Fault
	jmpi _exception_exit

BRANCH_LABEL(no_reschedule)

	/* We put the thread stack pointer on top of the IRQ stack before
	 * we switched stacks. Restore it to go back to thread stack
	 */
	ldw sp, 0(sp)

	/* Fall through */

SECTION_FUNC(exception.entry, _exception_exit)
	/* We are on the thread stack. Restore all saved registers
	 * and return to the interrupted context */

	/* Return address from the exception */
	ldw ea, 72(sp)

	/* Restore estatus
	 * XXX is this right??? */
	ldw   r5, 68(sp)
	wrctl estatus, r5

	/* Restore caller-saved registers */
	ldw ra,   0(sp)
	ldw r1,   8(sp)
	ldw r2,  12(sp)
	ldw r3,  16(sp)
	ldw r4,  20(sp)
	ldw r5,  24(sp)
	ldw r6,  28(sp)
	ldw r7,  32(sp)
	ldw r8,  36(sp)
	ldw r9,  40(sp)
	ldw r10, 44(sp)
	ldw r11, 48(sp)
	ldw r12, 52(sp)
	ldw r13, 56(sp)
	ldw r14, 60(sp)
	ldw r15, 64(sp)

	/* Put the stack pointer back where it was when we entered
	 * exception state
	 */
	addi sp, sp, 76

	/* All done, copy estatus into status and transfer to ea */
	eret
