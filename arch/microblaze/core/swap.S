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

/* exports */
.global arch_swap

/* imports */
.extern _k_neg_eagain
.extern _asm_stack_failed


/* unsigned int arch_swap(unsigned int key)
 *
 * Always called with interrupts locked.
 * Must always be called with a delayed branch instruction!
 */
SECTION_FUNC(exception.other, arch_swap)

	/* Make room for the context on the stack. */
	STACK_ALLOC(__z_arch_esf_t_SIZEOF)
	ASSERT_GT_ZERO(r1, _asm_stack_failed)

	/* Populate default return value */
	LOAD_REG_FROM_ADDR(r3, _k_neg_eagain)

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
	/* The interrupts will always save the "next instruction to execute" to r14.
	 * This isn't the same as r15 which is a link to the calling instruction.
	 * Because there's a chance we may return from an ISR, we store r15+8 to r14,
	 * so that r14 always has the next instruction to execute. And we make both
	 * the swap and isr routines return to r14. Therefore, arch_swap should never
	 * be called via an undelayed branch instruction!
	 */
	addik r14, r15, 8
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

	#if defined(CONFIG_MICROBLAZE_USE_HARDWARE_FLOAT_INSTR)
		/* Stack FSR using TEMP_DATA_REG(temp1) */
		mfs TEMP_DATA_REG, rfsr
		STORE_TO_STACK(TEMP_DATA_REG, ESF_OFFSET(fsr))
	#endif

#if defined(CONFIG_INSTRUMENT_THREAD_SWITCHING)
	CALL(z_thread_mark_switched_out, \
	DELAY_SLOT(nop))
	/* Need to preserve r3-retval as we're about to store it */
	POP_CONTEXT_FROM_STACK(r3)
	/* Need to preserve r5-arg1 as it has the function argument. */
	POP_CONTEXT_FROM_STACK(r5)
#endif

	/* Get a reference to _kernel again in r11 (temp1) -> r11 = &_kernel */
	SET_REG(KERNEL_REF_REG, _kernel)
	/* Get a reference to current thread in r12 (temp2): r12 = *(&kernel+offsetof(current)) */
	LOAD_CURRENT_THREAD(CURRENT_THREAD_REG)
	/* Save the stack pointer to current thread */
	STORE_TO_CURRENT_THREAD(r1, _thread_offset_to_r1)
	/* r5 has the 'key' argument which is the result of irq_lock() before this was called */
	STORE_TO_CURRENT_THREAD(r5, _thread_offset_to_key)
	/* Write 0 to preempted to indicate this thread yielded from arch_swap */
	STORE_TO_CURRENT_THREAD(r0, _thread_offset_to_preempted)
	/* Also store the default retval to thread */
	STORE_TO_CURRENT_THREAD(r3, _thread_offset_to_retval)

	/* Get a reference to ready_q.cache in r4 (retval1) *(&_kernel+offsetof(ready_q.cache)) */
	LOAD_NEXT_THREAD(NEXT_THREAD_REG)
	/* The thread to be swapped in is now the current thread */
	WRITE_TO_KERNEL_CURRENT(NEXT_THREAD_REG)
	/* Grab the stack pointer from "new" current thread */
	LOAD_FROM_NEXT_THREAD(r1, _thread_offset_to_r1)

#if defined(CONFIG_INSTRUMENT_THREAD_SWITCHING)
	/* Preserve r4 */
	STACK_ALLOC(4)
	STORE_TO_STACK(NEXT_THREAD_REG, 0)

	CALL(z_thread_mark_switched_in, \
	DELAY_SLOT(nop))

	LOAD_FROM_STACK(NEXT_THREAD_REG, 0)
	STACK_FREE(4)
#endif
	/* r1 (sp) now points to new thread's stack */

_arch_swap_restore_and_exit:

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
	/* r4 is next thread reg and will be return value for arch_swap; restoring it differently */
	POP_CONTEXT_FROM_STACK(r3)
	POP_CONTEXT_FROM_STACK(r2)

	/* BEGIN restore carry bit */
	LOAD_FROM_STACK(TEMP_DATA_REG, ESF_OFFSET(msr))
	MASK_BITS(TEMP_DATA_REG, MSR_C_MASK)
	beqid TEMP_DATA_REG, _swap_clear_carry
	mfs TEMP_DATA_REG, rmsr

_swap_set_carry:
	SET_BITS(TEMP_DATA_REG, MSR_C_MASK)
	braid _swap_restore_carry
	nop

_swap_clear_carry:
	CLEAR_BITS(TEMP_DATA_REG, MSR_C_MASK)

_swap_restore_carry:
	mts rmsr, TEMP_DATA_REG
	/* END restore carry bit */

	#if defined(CONFIG_MICROBLAZE_USE_HARDWARE_FLOAT_INSTR)
		/* Reload the FSR from the stack. */
		LOAD_FROM_STACK(TEMP_DATA_REG, ESF_OFFSET(fsr))
		mts rfsr, TEMP_DATA_REG
	#endif

	LOAD_FROM_NEXT_THREAD(TEMP_DATA_REG, _thread_offset_to_preempted)
	bneid TEMP_DATA_REG, _arch_swap_check_key // skip set retval if preempted == 1
	LOAD_FROM_NEXT_THREAD(TEMP_DATA_REG, _thread_offset_to_key)
	/* Load return value into r3 (returnval1).
	 * -EAGAIN unless someone previously called arch_thread_return_value_set().
	 * Do this before we potentially unlock interrupts. */
	LOAD_FROM_NEXT_THREAD(r3, _thread_offset_to_retval)

_arch_swap_check_key:
	/* Temp data reg has the "key" */
	beqid TEMP_DATA_REG, _arch_swap_exit // skip unlock if key == 0
	/* Restore r4 before we potentially unlock interrupts. */
	POP_CONTEXT_FROM_STACK(r4)

_arch_swap_unlock_irq:
	POP_CONTEXT_FROM_STACK(TEMP_DATA_REG)
	STACK_FREE(__z_arch_esf_t_SIZEOF)
/* BEGIN microblaze_enable_interrupts() */
#if CONFIG_MICROBLAZE_USE_MSR_INSTR == 1
	/* r10 was being used as a temporary. Now restore its true value from the stack. */
	rtsd r14, 0
	msrset  r0, MSR_IE_MASK
#else /*CONFIG_MICROBLAZE_USE_MSR_INSTR == 1*/
	/* r17-exception return address register should be OK to use here
	 * Because if we somehow manage to get an exception here,
	 * we probably dont plan to come back...
	 * Most likely exception causes:
	 * 1. r14 contains a poor address
	 * 2. We tried to write an invalid value to MSR
	 */
	mfs	r17, rmsr
	ori	r17, r17, MSR_IE_MASK
	rtsd r14, 0
	mts	rmsr, r17
#endif
/* END microblaze_enable_interrupts() */

_arch_swap_exit:
	/* r10 was being used as a temporary. Now restore its true value from the stack. */
	POP_CONTEXT_FROM_STACK(TEMP_DATA_REG)
	rtsd r14, 0
	STACK_FREE(__z_arch_esf_t_SIZEOF)
