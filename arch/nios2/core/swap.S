/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <toolchain.h>
#include <linker/sections.h>
#include <offsets_short.h>

/* exports */
GTEXT(arch_swap)
GTEXT(z_thread_entry_wrapper)

/* imports */
GTEXT(_k_neg_eagain)

/* unsigned int arch_swap(unsigned int key)
 *
 * Always called with interrupts locked
 */
SECTION_FUNC(exception.other, arch_swap)

#if defined(CONFIG_INSTRUMENT_THREAD_SWITCHING)
	/* Need to preserve r4 as it has the function argument. */
	addi sp, sp, -12
	stw ra, 8(sp)
	stw fp, 4(sp)
	stw r4, 0(sp)

	call z_thread_mark_switched_out

	ldw r4, 0(sp)
	ldw fp, 4(sp)
	ldw ra, 8(sp)
	addi sp, sp, 12
#endif

	/* Get a reference to _kernel in r10 */
	movhi r10, %hi(_kernel)
	ori   r10, r10, %lo(_kernel)

	/* Get the pointer to kernel->current */
	ldw  r11, _kernel_offset_to_current(r10)

	/* Store all the callee saved registers. We either got here via
	 * an exception or from a cooperative invocation of arch_swap() from C
	 * domain, so all the caller-saved registers have already been
	 * saved by the exception asm or the calling C code already.
	 */
	stw r16, _thread_offset_to_r16(r11)
	stw r17, _thread_offset_to_r17(r11)
	stw r18, _thread_offset_to_r18(r11)
	stw r19, _thread_offset_to_r19(r11)
	stw r20, _thread_offset_to_r20(r11)
	stw r21, _thread_offset_to_r21(r11)
	stw r22, _thread_offset_to_r22(r11)
	stw r23, _thread_offset_to_r23(r11)
	stw r28, _thread_offset_to_r28(r11)
	stw ra,  _thread_offset_to_ra(r11)
	stw sp,  _thread_offset_to_sp(r11)

	/* r4 has the 'key' argument which is the result of irq_lock()
	 * before this was called
	 */
	stw r4, _thread_offset_to_key(r11)

	/* Populate default return value */
	movhi r5, %hi(_k_neg_eagain)
	ori   r5, r5, %lo(_k_neg_eagain)
	ldw   r4, (r5)
	stw   r4, _thread_offset_to_retval(r11)

	/* get cached thread to run */
	ldw   r2, _kernel_offset_to_ready_q_cache(r10)

	/* At this point r2 points to the next thread to be swapped in */

	/* the thread to be swapped in is now the current thread */
	stw   r2, _kernel_offset_to_current(r10)

	/* Restore callee-saved registers and switch to the incoming
	 * thread's stack
	 */
	ldw r16, _thread_offset_to_r16(r2)
	ldw r17, _thread_offset_to_r17(r2)
	ldw r18, _thread_offset_to_r18(r2)
	ldw r19, _thread_offset_to_r19(r2)
	ldw r20, _thread_offset_to_r20(r2)
	ldw r21, _thread_offset_to_r21(r2)
	ldw r22, _thread_offset_to_r22(r2)
	ldw r23, _thread_offset_to_r23(r2)
	ldw r28, _thread_offset_to_r28(r2)
	ldw ra,  _thread_offset_to_ra(r2)
	ldw sp,  _thread_offset_to_sp(r2)

	/* We need to irq_unlock(current->coopReg.key);
	 * key was supplied as argument to arch_swap(). Fetch it.
	 */
	ldw r3, _thread_offset_to_key(r2)

	/*
	 * Load return value into r2 (return value register). -EAGAIN unless
	 * someone previously called arch_thread_return_value_set(). Do this
	 * before we potentially unlock interrupts.
	 */
	ldw r2, _thread_offset_to_retval(r2)

	/* Now do  irq_unlock(current->coopReg.key) */
#if (ALT_CPU_NUM_OF_SHADOW_REG_SETS > 0) || \
		(defined ALT_CPU_EIC_PRESENT) || \
		(defined ALT_CPU_MMU_PRESENT) || \
		(defined ALT_CPU_MPU_PRESENT)
	andi r3, r3, NIOS2_STATUS_PIE_MSK
	beq r3, zero, no_unlock
	rdctl r3, status
	ori r3, r3, NIOS2_STATUS_PIE_MSK
	wrctl status, r3

no_unlock:
#else
	wrctl status, r3
#endif

#if defined(CONFIG_INSTRUMENT_THREAD_SWITCHING)
	/* Also need to preserve r2, r3 as return values */
	addi sp, sp, -20
	stw ra, 16(sp)
	stw fp, 12(sp)
	stw r4, 8(sp)
	stw r3, 4(sp)
	stw r2, 0(sp)

	call z_thread_mark_switched_in

	ldw r2, 0(sp)
	ldw r3, 4(sp)
	ldw r4, 8(sp)
	ldw fp, 12(sp)
	ldw ra, 16(sp)
	addi sp, sp, 20
#endif
	ret


/* void z_thread_entry_wrapper(void)
 */
SECTION_FUNC(TEXT, z_thread_entry_wrapper)
	/* This all corresponds to struct init_stack_frame defined in
	 * thread.c. We need to take this stuff off the stack and put
	 * it in the appropriate registers
	 */

	/* Can't return from here, just put NULL in ra */
	movi ra, 0

	/* Calling convention has first 4 arguments in registers r4-r7. */
	ldw  r4, 0(sp)
	ldw  r5, 4(sp)
	ldw  r6, 8(sp)
	ldw  r7, 12(sp)

	/* pop all the stuff that we just loaded into registers */
	addi sp, sp, 16

	call z_thread_entry

