/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <offsets_short.h>

/* exports */
GTEXT(_exception)

/* import */
GTEXT(z_nios2_fault)
GTEXT(arch_swap)
#ifdef CONFIG_IRQ_OFFLOAD
GTEXT(z_irq_do_offload)
GTEXT(_offload_routine)
#endif

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
	subi sp, sp, __struct_arch_esf_SIZEOF

	/* Preserve all caller-saved registers onto the thread's stack */
	stw ra,  __struct_arch_esf_ra_OFFSET(sp)
	stw r1,  __struct_arch_esf_r1_OFFSET(sp)
	stw r2,  __struct_arch_esf_r2_OFFSET(sp)
	stw r3,  __struct_arch_esf_r3_OFFSET(sp)
	stw r4,  __struct_arch_esf_r4_OFFSET(sp)
	stw r5,  __struct_arch_esf_r5_OFFSET(sp)
	stw r6,  __struct_arch_esf_r6_OFFSET(sp)
	stw r7,  __struct_arch_esf_r7_OFFSET(sp)
	stw r8,  __struct_arch_esf_r8_OFFSET(sp)
	stw r9,  __struct_arch_esf_r9_OFFSET(sp)
	stw r10, __struct_arch_esf_r10_OFFSET(sp)
	stw r11, __struct_arch_esf_r11_OFFSET(sp)
	stw r12, __struct_arch_esf_r12_OFFSET(sp)
	stw r13, __struct_arch_esf_r13_OFFSET(sp)
	stw r14, __struct_arch_esf_r14_OFFSET(sp)
	stw r15, __struct_arch_esf_r15_OFFSET(sp)

	/* Store value of estatus control register */
	rdctl et, estatus
	stw   et, __struct_arch_esf_estatus_OFFSET(sp)

	/* ea-4 is the address of the instruction when the exception happened,
	 * put this in the stack frame as well
	 */
	addi  r15, ea, -4
	stw   r15, __struct_arch_esf_instr_OFFSET(sp)

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

is_interrupt:
	/* If we get here, this is an interrupt */

	/* Grab a reference to _kernel in r10 so we can determine the
	 * current irq stack pointer
	 */
	movhi r10, %hi(_kernel)
	ori r10, r10, %lo(_kernel)

	/* Stash a copy of thread's sp in r12 so that we can put it on the IRQ
	 * stack
	 */
	mov r12, sp

	/* Switch to interrupt stack */
	ldw sp, _kernel_offset_to_irq_stack(r10)

	/* Store thread stack pointer onto IRQ stack */
	addi sp, sp, -4
	stw r12, 0(sp)

on_irq_stack:

	/* Enter C interrupt handling code. Value of ipending will be the
	 * function parameter since we put it in r4
	 */
	call _enter_irq

	/* Interrupt handler finished and the interrupt should be serviced
	 * now, the appropriate bits in ipending should be cleared */

	/* Get a reference to _kernel again in r10 */
	movhi r10, %hi(_kernel)
	ori   r10, r10, %lo(_kernel)

#ifdef CONFIG_PREEMPT_ENABLED
	ldw   r11, _kernel_offset_to_current(r10)
	/* Determine whether the exception of the ISR requires context
	 * switch
	 */

	/* Call into the kernel to see if a scheduling decision is necessary */
	ldw  r2, _kernel_offset_to_ready_q_cache(r10)
	beq  r2, r11, no_reschedule

	/*
	 * A context reschedule is required: keep the volatile registers of
	 * the interrupted thread on the context's stack.  Utilize
	 * the existing arch_swap() primitive to save the remaining
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

	call arch_swap
	jmpi _exception_exit
#else
	jmpi no_reschedule
#endif /* CONFIG_PREEMPT_ENABLED */

not_interrupt:

	/* Since this wasn't an interrupt we're not going to restart the
	 * faulting instruction.
	 *
	 * We earlier put ea - 4 in the stack frame, replace it with just ea
	 */
	stw ea, __struct_arch_esf_instr_OFFSET(sp)

#ifdef CONFIG_IRQ_OFFLOAD
	/* Check the contents of _offload_routine. If non-NULL, jump into
	 * the interrupt code anyway.
	 */
	movhi r10, %hi(_offload_routine)
	ori   r10, r10, %lo(_offload_routine)
	ldw   r11, (r10)
	bne   r11, zero, is_interrupt
#endif

_exception_enter_fault:
	/* If we get here, the exception wasn't in interrupt or an
	 * invocation of irq_offload(). Let z_nios2_fault() handle it in
	 * C domain
	 */

	mov  r4, sp
	call z_nios2_fault
	jmpi _exception_exit

no_reschedule:

	/* We put the thread stack pointer on top of the IRQ stack before
	 * we switched stacks. Restore it to go back to thread stack
	 */
	ldw sp, 0(sp)

	/* Fall through */

_exception_exit:
	/* We are on the thread stack. Restore all saved registers
	 * and return to the interrupted context */

	/* Return address from the exception */
	ldw ea, __struct_arch_esf_instr_OFFSET(sp)

	/* Restore estatus
	 * XXX is this right??? */
	ldw   r5, __struct_arch_esf_estatus_OFFSET(sp)
	wrctl estatus, r5

	/* Restore caller-saved registers */
	ldw ra,  __struct_arch_esf_ra_OFFSET(sp)
	ldw r1,  __struct_arch_esf_r1_OFFSET(sp)
	ldw r2,  __struct_arch_esf_r2_OFFSET(sp)
	ldw r3,  __struct_arch_esf_r3_OFFSET(sp)
	ldw r4,  __struct_arch_esf_r4_OFFSET(sp)
	ldw r5,  __struct_arch_esf_r5_OFFSET(sp)
	ldw r6,  __struct_arch_esf_r6_OFFSET(sp)
	ldw r7,  __struct_arch_esf_r7_OFFSET(sp)
	ldw r8,  __struct_arch_esf_r8_OFFSET(sp)
	ldw r9,  __struct_arch_esf_r9_OFFSET(sp)
	ldw r10, __struct_arch_esf_r10_OFFSET(sp)
	ldw r11, __struct_arch_esf_r11_OFFSET(sp)
	ldw r12, __struct_arch_esf_r12_OFFSET(sp)
	ldw r13, __struct_arch_esf_r13_OFFSET(sp)
	ldw r14, __struct_arch_esf_r14_OFFSET(sp)
	ldw r15, __struct_arch_esf_r15_OFFSET(sp)

	/* Put the stack pointer back where it was when we entered
	 * exception state
	 */
	addi sp, sp, __struct_arch_esf_SIZEOF

	/* All done, copy estatus into status and transfer to ea */
	eret
