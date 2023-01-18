/*
 * Copyright (c) 2023 Advanced Micro Devices, Inc. (AMD)
 * Copyright (c) 2023 Alp Sayin <alpsayin@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <offsets_short.h>
#include <microblaze/microblaze_regs.h>
#include <microblaze/microblaze_asm.h>

/* import */
.extern _asm_stack_failed

/* export */
.global _interrupt_handler

/* The context is oversized to allow functions called from the ISR to write
 * back into the caller stack. The size is defined as __z_arch_esf_t_SIZEOF in offsets.c
 */

/* Offsets from the stack pointer at which saved registers are placed.
 * These are generated using the definitions in offsets.c
 */

	.text
	.balign 4

_interrupt_handler:
	/* Make room for the context on the stack. */
	STACK_ALLOC(__z_arch_esf_t_SIZEOF)
	ASSERT_GT_ZERO(r1, _asm_stack_failed)

	PUSH_CONTEXT_TO_STACK(r31)
	PUSH_CONTEXT_TO_STACK(r30)
	PUSH_CONTEXT_TO_STACK(r29)
	PUSH_CONTEXT_TO_STACK(r28)
	PUSH_CONTEXT_TO_STACK(r27)
	PUSH_CONTEXT_TO_STACK(r26)
	PUSH_CONTEXT_TO_STACK(r25)
	PUSH_CONTEXT_TO_STACK(r24)
	PUSH_CONTEXT_TO_STACK(r23)
	PUSH_CONTEXT_TO_STACK(r22)
	PUSH_CONTEXT_TO_STACK(r21)
	PUSH_CONTEXT_TO_STACK(r20)
	PUSH_CONTEXT_TO_STACK(r19)
	PUSH_CONTEXT_TO_STACK(r18)
	PUSH_CONTEXT_TO_STACK(r17)
	PUSH_CONTEXT_TO_STACK(r16)
	/* Stack the return address from user sub-routines */
	PUSH_CONTEXT_TO_STACK(r15)
	/* Stack the return address from interrupt */
	PUSH_CONTEXT_TO_STACK(r14)
	PUSH_CONTEXT_TO_STACK(r13)
	PUSH_CONTEXT_TO_STACK(r12)
	PUSH_CONTEXT_TO_STACK(r11)
	PUSH_CONTEXT_TO_STACK(r10)
	PUSH_CONTEXT_TO_STACK(r9)
	PUSH_CONTEXT_TO_STACK(r8)
	PUSH_CONTEXT_TO_STACK(r7)
	PUSH_CONTEXT_TO_STACK(r6)
	PUSH_CONTEXT_TO_STACK(r5)
	PUSH_CONTEXT_TO_STACK(r4)
	PUSH_CONTEXT_TO_STACK(r3)
	PUSH_CONTEXT_TO_STACK(r2)

	/* Stack MSR using r11(temp1) */
	mfs TEMP_DATA_REG, rmsr
	STORE_TO_STACK(TEMP_DATA_REG, ESF_OFFSET(msr))

	#if defined(CONFIG_USE_HARDWARE_FLOAT_INSTR)
		/* Stack FSR using TEMP_DATA_REG(temp1) */
		mfs TEMP_DATA_REG, rfsr
		STORE_TO_STACK(TEMP_DATA_REG, ESF_OFFSET(fsr))
	#endif

	/* Get a reference to _kernel in r11 (temp1) -> r11 = &_kernel */
	SET_REG(KERNEL_REF_REG, _kernel)
	/* Get a reference to current thread in r12 (temp2): r12 = *(&kernel+offsetof(current)) */
	LOAD_CURRENT_THREAD(CURRENT_THREAD_REG)
	/* Save the stack pointer to current thread */
	STORE_TO_CURRENT_THREAD(r1, _thread_offset_to_r1)
	/* Write 1 to key to indicate that IRQs are currently unlocked */
	SET_REG(TEMP_DATA_REG, 1)
	STORE_TO_CURRENT_THREAD(TEMP_DATA_REG, _thread_offset_to_key)
	/* Write 1 to preempted to indicate this thread yielded from ISR */
	STORE_TO_CURRENT_THREAD(TEMP_DATA_REG, _thread_offset_to_preempted)

	/* Switch to the ISR stack: r1 = *(&_kernel+offsetof(irq_stack)) */
	SWITCH_TO_IRQ_STACK(r1)
	/* r1 (sp) now points to cpu dedicated irq stack */

	/* Assert ISR stack is sane */
	ASSERT_GT_ZERO(r1, _asm_stack_failed)

on_irq_stack:
	/* Enter C interrupt handling code.
	 * Execute any pending interrupts.
	 */
	CALL(_enter_irq, \
	DELAY_SLOT(nop))

	/* Interrupt handler finished and the interrupt should be serviced
	 * now, the appropriate bits in ipending should be cleared
	 */

#ifdef CONFIG_PREEMPT_ENABLED

	/* Get a reference to _kernel again in r11 (temp1) = &_kernel */
	SET_REG(KERNEL_REF_REG, _kernel)
	/* Get a reference to current thread in r12 (temp2) = *(&kernel+offsetof(current)) */
	LOAD_CURRENT_THREAD(CURRENT_THREAD_REG)
	/* Get a reference to ready_q.cache in r4 (retval1) = *(&_kernel+offsetof(ready_q.cache)) */
	LOAD_NEXT_THREAD(NEXT_THREAD_REG)
	/* Check to see if a scheduling decision is necessary
	 * by comparing current vs kernel.ready_q.cache threads */
	cmpu CURRENT_THREAD_REG, NEXT_THREAD_REG, CURRENT_THREAD_REG // r12 should be 0 if r4==r12
	/* Delay slot instruction has no effect if they're equal */
	JUMP_IF_ZERO(CURRENT_THREAD_REG, _isr_restore_and_exit, \
	DELAY_SLOT(COPY_REG(CURRENT_THREAD_REG, NEXT_THREAD_REG)))

	/* A context reschedule is required */
#if defined(CONFIG_INSTRUMENT_THREAD_SWITCHING)
	CALL(z_thread_mark_switched_out, \
	DELAY_SLOT(nop))
#endif
	/* Get a reference to _kernel again in r11 (temp1) = &_kernel */
	SET_REG(KERNEL_REF_REG, _kernel)
	/* Get a reference to ready_q.cache in r4 (retval1) = *(&_kernel+offsetof(ready_q.cache)) */
	LOAD_NEXT_THREAD(NEXT_THREAD_REG)
	/* Scheduler seems to run whenever a task is about to yield either
	 * because of voluntary yield or blocking. So it runs in "spare time".
	 * Definitely not supposed to be in ISR so we're OK but just in case
	 */
	WRITE_TO_KERNEL_CURRENT(NEXT_THREAD_REG)
	/* _kernel.current is now equal to _kernel.ready_q.cache */
#if defined(CONFIG_INSTRUMENT_THREAD_SWITCHING)
	CALL(z_thread_mark_switched_in, \
	DELAY_SLOT(nop))
#endif
#endif /* ifdef CONFIG_PREEMPT_ENABLED */

_isr_restore_and_exit:
	/* Get a reference to _kernel again in r11 (temp1) = &_kernel */
	SET_REG(KERNEL_REF_REG, _kernel)
	/* Get a reference to current thread in r12 (temp2) = *(&kernel+offsetof(current)) */
	LOAD_CURRENT_THREAD(CURRENT_THREAD_REG)
	/* Grab the stack pointer from "new" current thread */
	LOAD_FROM_CURRENT_THREAD(r1, _thread_offset_to_r1)
	LOAD_FROM_CURRENT_THREAD(TEMP_DATA_REG, _thread_offset_to_preempted)
	/* skip set retval if preempted == 1 */
	JUMP_IF_NONZERO(TEMP_DATA_REG, _isr_skip_set_retval, \
	DELAY_SLOT(POP_CONTEXT_FROM_STACK(r3)))

	/* Load return value into r3 (returnval1).
	 * -EAGAIN unless someone previously called arch_thread_return_value_set()
	 */
	LOAD_FROM_CURRENT_THREAD(r3, _thread_offset_to_retval)

_isr_skip_set_retval:

	POP_CONTEXT_FROM_STACK(r31)
	POP_CONTEXT_FROM_STACK(r30)
	POP_CONTEXT_FROM_STACK(r29)
	POP_CONTEXT_FROM_STACK(r28)
	POP_CONTEXT_FROM_STACK(r27)
	POP_CONTEXT_FROM_STACK(r26)
	POP_CONTEXT_FROM_STACK(r25)
	POP_CONTEXT_FROM_STACK(r24)
	POP_CONTEXT_FROM_STACK(r23)
	POP_CONTEXT_FROM_STACK(r22)
	POP_CONTEXT_FROM_STACK(r21)
	POP_CONTEXT_FROM_STACK(r20)
	POP_CONTEXT_FROM_STACK(r19)
	POP_CONTEXT_FROM_STACK(r18)
	POP_CONTEXT_FROM_STACK(r17)
	POP_CONTEXT_FROM_STACK(r16)
	POP_CONTEXT_FROM_STACK(r15)
	POP_CONTEXT_FROM_STACK(r14)
	POP_CONTEXT_FROM_STACK(r13)
	POP_CONTEXT_FROM_STACK(r12)
	POP_CONTEXT_FROM_STACK(r11)
	POP_CONTEXT_FROM_STACK(r10)
	POP_CONTEXT_FROM_STACK(r9)
	POP_CONTEXT_FROM_STACK(r8)
	POP_CONTEXT_FROM_STACK(r7)
	POP_CONTEXT_FROM_STACK(r6)
	POP_CONTEXT_FROM_STACK(r5)
	POP_CONTEXT_FROM_STACK(r4)
	/* r3-retval is restored or set previously */
	POP_CONTEXT_FROM_STACK(r2)

	/* BEGIN restore carry bit */
	LOAD_FROM_STACK(TEMP_DATA_REG, ESF_OFFSET(msr))
	MASK_BITS(TEMP_DATA_REG, MSR_C_MASK)
	JUMP_IF_ZERO(TEMP_DATA_REG, _isr_clear_carry, \
	DELAY_SLOT(mfs TEMP_DATA_REG, rmsr))

_isr_set_carry:
	SET_BITS(TEMP_DATA_REG, MSR_C_MASK)
	JUMP(_isr_restore_carry, \
	DELAY_SLOT(nop))

_isr_clear_carry:
	CLEAR_BITS(TEMP_DATA_REG, MSR_C_MASK)

_isr_restore_carry:
	mts rmsr, TEMP_DATA_REG
	/* END restore carry bit */

	#if defined(CONFIG_USE_HARDWARE_FLOAT_INSTR)
		/* Reload the FSR from the stack. */
		LOAD_FROM_STACK(TEMP_DATA_REG, ESF_OFFSET(fsr))
		mts rfsr, TEMP_DATA_REG
	#endif

	/* r10, r14 was being used as a temporary. Now restore its true value from the stack. */
	POP_CONTEXT_FROM_STACK(TEMP_DATA_REG)


_isr_interrupt_exit:
	/* Return using rtid so interrupts are re-enabled as this function is
	 * exited. Use r14(hw enforced return from interrupt address)
	 */
	rtid r14, 0
	/* Put the stack pointer back where it was when we entered
	 * exception state i.e. remove the stack frame.
	 */
	STACK_FREE(__z_arch_esf_t_SIZEOF)
