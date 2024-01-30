/*
 * Copyright (c) 2022, Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <xtensa_asm2_s.h>
#include <offsets.h>
#include <offsets_short.h>
#include <zephyr/syscall.h>
#include <zsr.h>

/**
 *  syscall number     arg1, arg2, arg3, arg4, arg5, arg6
 *  --------------     ----------------------------------
 *  a2                 a6,   a3,   a4,   a5,   a8,   a9
 *
 **/
.pushsection .text.xtensa_do_syscall, "ax"
.global	xtensa_do_syscall
.align	4
xtensa_do_syscall:
	rsr a0, ZSR_CPU
	l32i a0, a0, ___cpu_t_current_OFFSET
	l32i a0, a0, _thread_offset_to_psp

	addi a0, a0, -___xtensa_irq_bsa_t_SIZEOF

	s32i a1, a0, ___xtensa_irq_bsa_t_scratch_OFFSET
	s32i a2, a0, ___xtensa_irq_bsa_t_a2_OFFSET
	s32i a3, a0, ___xtensa_irq_bsa_t_a3_OFFSET
	rsr a2, ZSR_A0SAVE
	s32i a2, a0, ___xtensa_irq_bsa_t_a0_OFFSET
	rsr.ps a2
	movi a3, ~PS_OWB_MASK
	and a2, a2, a3
	s32i a2, a0, ___xtensa_irq_bsa_t_ps_OFFSET
	rsr.epc1 a2
	s32i a2, a0, ___xtensa_irq_bsa_t_pc_OFFSET

#if XCHAL_HAVE_NMI
	movi a2, PS_WOE|PS_INTLEVEL(XCHAL_NMILEVEL)
#elif XCHAL_HAVE_INTERRUPTS
	movi a2, PS_WOE|PS_INTLEVEL(XCHAL_NUM_INTLEVELS)
#else
#error Xtensa core with no interrupt support is used
#endif
	rsr.ps a3
	or a3, a3, a2
	movi a2, ~(PS_EXCM | PS_RING_MASK)
	and a3, a3, a2
	wsr.ps a3
	rsync
	l32i a2, a0, ___xtensa_irq_bsa_t_a2_OFFSET
	l32i a3, a0, ___xtensa_irq_bsa_t_a3_OFFSET
	SPILL_ALL_WINDOWS

	rsr a0, ZSR_CPU
	l32i a0, a0, ___cpu_t_current_OFFSET
	l32i a0, a0, _thread_offset_to_psp
	addi a0, a0, -___xtensa_irq_bsa_t_SIZEOF

	mov a1, a0

	l32i a3, a1, ___xtensa_irq_bsa_t_pc_OFFSET
#if XCHAL_HAVE_LOOPS
	/* If the syscall instruction was the last instruction in the body of
	 * a zero-overhead loop, and the loop will execute again, decrement
	 * the loop count and resume execution at the head of the loop.
	 */
	rsr.lend a2
	addi a3, a3, 3
	bne a2, a3, end_loop
	rsr.lcount a2
	beqz a2, end_loop
	addi a2, a2, -1
	wsr.lcount a2
	rsr.lbeg a3
end_loop:
#else
	/* EPC1 (and now a3) contains the address that invoked syscall.
	 * We need to increment it to execute the next instruction when
	 * we return. The instruction size is 3 bytes, so lets just add it.
	 */
	addi a3, a3, 3
#endif
	s32i a3, a1, ___xtensa_irq_bsa_t_pc_OFFSET
	ODD_REG_SAVE

#if defined(CONFIG_XTENSA_HIFI_SHARING)
	call0 _xtensa_hifi_save
#endif

	call0 xtensa_save_high_regs

	l32i a2, a1, 0
	l32i a2, a2, ___xtensa_irq_bsa_t_a2_OFFSET
	movi a0, K_SYSCALL_LIMIT
	bgeu a2, a0, _bad_syscall

_id_ok:
	/* Find the function handler for the given syscall id. */
	movi a3, _k_syscall_table
	slli a2, a2, 2
	add a2, a2, a3
	l32i a2, a2, 0

	/* Clear up the threadptr because it is used
	 * to check if a thread is running on user mode. Since
	 * we are in a interruption we don't want the system
	 * thinking it is possibly running in user mode.
	 */
#ifdef CONFIG_THREAD_LOCAL_STORAGE
	movi a0, is_user_mode@tpoff
	rur.THREADPTR a3
	add a0, a3, a0

	movi a3, 0
	s32i a3, a0, 0
#else
	movi a0, 0
	wur.THREADPTR a0
#endif

	/* Set syscall parameters by moving them into place before we do
	 * a call4 for the syscall function itself.
	 * arg1 = a6
	 * arg2 = a3 (clobbered above, so we need to reload it)
	 * arg3 = a4
	 * arg4 = a5
	 * arg5 = a8
	 * arg6 = a9
	 */
	mov a10, a8
	mov a11, a9
	mov a8, a4
	mov a9, a5

	/* Stack frame pointer is the 7th argument to z_mrsh_*()
	 * as ssf, and must be put on stack to be consumed.
	 */
	mov a3, a1
	addi a1, a1, -4
	s32i a3, a1, 0

	l32i a3, a1, 4
	l32i a7, a3, ___xtensa_irq_bsa_t_a3_OFFSET


	/* Since we are unmasking EXCM, we need to set RING bits to kernel
	 * mode, otherwise we won't be able to run the exception handler in C.
	 */
	movi a0, PS_WOE|PS_CALLINC(0)|PS_UM|PS_INTLEVEL(0)
	wsr.ps a0
	rsync

	callx4 a2

	/* Going back before stack frame pointer on stack to
	 * actual the stack frame. So restoration of registers
	 * can be done properly when finishing syscalls.
	 */
	addi a1, a1, 4

	/* copy return value. Lets put it in the top of stack
	 * because registers will be clobbered in
         * xtensa_restore_high_regs
	 */
	l32i a3, a1, 0
	s32i a6, a3, ___xtensa_irq_bsa_t_a2_OFFSET

	j _syscall_returned

_syscall_returned:
	call0 xtensa_restore_high_regs

	l32i a3, a1, ___xtensa_irq_bsa_t_sar_OFFSET
	wsr a3, SAR
#if XCHAL_HAVE_LOOPS
	l32i a3, a1, ___xtensa_irq_bsa_t_lbeg_OFFSET
	wsr a3, LBEG
	l32i a3, a1, ___xtensa_irq_bsa_t_lend_OFFSET
	wsr a3, LEND
	l32i a3, a1, ___xtensa_irq_bsa_t_lcount_OFFSET
	wsr a3, LCOUNT
#endif
#if XCHAL_HAVE_S32C1I
	l32i a3, a1, ___xtensa_irq_bsa_t_scompare1_OFFSET
	wsr a3, SCOMPARE1
#endif

#ifdef CONFIG_THREAD_LOCAL_STORAGE
	l32i a3, a1, ___xtensa_irq_bsa_t_threadptr_OFFSET
	movi a0, is_user_mode@tpoff
	add a0, a3, a0
	movi a3, 1
	s32i a3, a0, 0
#else
	rsr a3, ZSR_CPU
	l32i a3, a3, ___cpu_t_current_OFFSET
	wur.THREADPTR a3
#endif

	l32i a3, a1, ___xtensa_irq_bsa_t_ps_OFFSET
	wsr.ps a3

	l32i a3, a1, ___xtensa_irq_bsa_t_pc_OFFSET
	wsr.epc1 a3

	l32i a0, a1, ___xtensa_irq_bsa_t_a0_OFFSET
	l32i a2, a1, ___xtensa_irq_bsa_t_a2_OFFSET
	l32i a3, a1, ___xtensa_irq_bsa_t_a3_OFFSET

	l32i a1, a1, ___xtensa_irq_bsa_t_scratch_OFFSET
	rsync

	rfe

_bad_syscall:
	movi a2, K_SYSCALL_BAD
	j _id_ok

.popsection

/* FUNC_NORETURN void xtensa_userspace_enter(k_thread_entry_t user_entry,
 *					   void *p1, void *p2, void *p3,
 *					   uint32_t stack_end,
 *					   uint32_t stack_start)
 *
 * A one-way trip to userspace.
 */
.global xtensa_userspace_enter
.type xtensa_userspace_enter, @function
.align 4
xtensa_userspace_enter:
	/* Call entry to set a bit in the windowstart and
	 * do the rotation, but we are going to set our own
	 * stack.
	 */
	entry a1, 16

	/* We have to switch to kernel stack before spill kernel data and
	 * erase user stack to avoid leak from previous context.
	 */
	mov a1, a7 /* stack start (low address) */
	addi a1, a1, -16

	SPILL_ALL_WINDOWS

	rsr a0, ZSR_CPU
	l32i a0, a0, ___cpu_t_current_OFFSET

	addi a1, a1, -28
	s32i a0, a1, 24
	s32i a2, a1, 20
	s32i a3, a1, 16
	s32i a4, a1, 12
	s32i a5, a1, 8
	s32i a6, a1, 4
	s32i a7, a1, 0

	l32i a6, a1, 24
	call4 xtensa_user_stack_perms

	l32i a6, a1, 24
	call4 xtensa_swap_update_page_tables

#ifdef CONFIG_THREAD_LOCAL_STORAGE
	rur.threadptr a3
	movi a0, is_user_mode@tpoff
	add a0, a3, a0
	movi a3, 1
	s32i a3, a0, 0
#else
	rsr a3, ZSR_CPU
	l32i a3, a3, ___cpu_t_current_OFFSET
	wur.THREADPTR a3
#endif

	/* Set now z_thread_entry parameters, we are simulating a call4
	 * call, so parameters start at a6, a7, ...
	 */
	l32i a6, a1, 20
	l32i a7, a1, 16
	l32i a8, a1, 12
	l32i a9, a1, 8

	/* stash user stack */
	l32i a0, a1, 4

	/* Go back to user stack */
	mov a1, a0

	movi a0, z_thread_entry
	wsr.epc2 a0

	/* Configuring PS register.
	 * We have to set callinc as well, since the called
	 * function will do "entry"
	 */
	movi a0, PS_WOE|PS_CALLINC(1)|PS_UM|PS_RING(2)
	wsr a0, EPS2

	movi a0, 0

	rfi 2

/*
 * size_t arch_user_string_nlen(const char *s, size_t maxsize, int *err_arg)
 */
.global arch_user_string_nlen
.type arch_user_string_nlen, @function
.align 4
arch_user_string_nlen:
	entry a1, 32

	/* error value, set to -1. */
	movi a5, -1
	s32i a5, a4, 0

	/* length count */
	xor a5, a5, a5

	/* This code might page fault */
strlen_loop:
.global xtensa_user_string_nlen_fault_start
xtensa_user_string_nlen_fault_start:
	l8ui a6, a2, 0 /* Current char */

.global xtensa_user_string_nlen_fault_end
xtensa_user_string_nlen_fault_end:
	beqz a6, strlen_done
	addi a5, a5, 1
	addi a2, a2, 1
	beq a5, a3, strlen_done
	j strlen_loop

strlen_done:
	/* Set return value */
	mov a2, a5

	/* Set error value to 0 since we succeeded */
	movi a5, 0x0
	s32i a5, a4, 0

.global xtensa_user_string_nlen_fixup
xtensa_user_string_nlen_fixup:
	retw
