/*
 * Copyright (c) 2021 Antony Pavlov <antonynpavlov@gmail.com>
 *
 * based on arch/riscv/core/isr.S and arch/nios2/core/exception.S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/kernel_structs.h>
#include <offsets_short.h>
#include <zephyr/arch/cpu.h>

#include <mips/regdef.h>
#include <mips/mipsregs.h>

#define ESF_O(FIELD)	__struct_arch_esf_##FIELD##_OFFSET
#define THREAD_O(FIELD)	_thread_offset_to_##FIELD

/* Convenience macros for loading/storing register states. */

#define DO_CALLEE_SAVED(op, reg) \
	op s0, THREAD_O(s0)(reg)		;\
	op s1, THREAD_O(s1)(reg)		;\
	op s2, THREAD_O(s2)(reg)		;\
	op s3, THREAD_O(s3)(reg)		;\
	op s4, THREAD_O(s4)(reg)		;\
	op s5, THREAD_O(s5)(reg)		;\
	op s6, THREAD_O(s6)(reg)		;\
	op s7, THREAD_O(s7)(reg)		;\
	op s8, THREAD_O(s8)(reg)		;

#define STORE_CALLEE_SAVED(reg) \
	DO_CALLEE_SAVED(OP_STOREREG, reg)

#define LOAD_CALLEE_SAVED(reg) \
	DO_CALLEE_SAVED(OP_LOADREG, reg)

#define DO_CALLER_SAVED(op) \
	op ra, ESF_O(ra)(sp)		;\
	op gp, ESF_O(gp)(sp)		;\
	op AT, ESF_O(at)(sp)		;\
	op t0, ESF_O(t0)(sp)		;\
	op t1, ESF_O(t1)(sp)		;\
	op t2, ESF_O(t2)(sp)		;\
	op t3, ESF_O(t3)(sp)		;\
	op t4, ESF_O(t4)(sp)		;\
	op t5, ESF_O(t5)(sp)		;\
	op t6, ESF_O(t6)(sp)		;\
	op t7, ESF_O(t7)(sp)		;\
	op t8, ESF_O(t8)(sp)		;\
	op t9, ESF_O(t9)(sp)		;\
	op a0, ESF_O(a0)(sp)		;\
	op a1, ESF_O(a1)(sp)		;\
	op a2, ESF_O(a2)(sp)		;\
	op a3, ESF_O(a3)(sp)		;\
	op v0, ESF_O(v0)(sp)		;\
	op v1, ESF_O(v1)(sp)		;

#define STORE_CALLER_SAVED() \
	addi sp, sp, -__struct_arch_esf_SIZEOF	;\
	DO_CALLER_SAVED(OP_STOREREG)		;

#define LOAD_CALLER_SAVED() \
	DO_CALLER_SAVED(OP_LOADREG)		;\
	addi sp, sp, __struct_arch_esf_SIZEOF	;

/* imports */
GTEXT(z_mips_fault)

GTEXT(_k_neg_eagain)
GTEXT(z_thread_mark_switched_in)
GTEXT(z_thread_mark_switched_out)

/* exports */
GTEXT(__isr_vec)

SECTION_FUNC(exception.entry, __isr_vec)
	la k0, _mips_interrupt
	jr k0

SECTION_FUNC(exception.other, _mips_interrupt)
	.set noat
	/*
	 * Save caller-saved registers on current thread stack.
	 */
	STORE_CALLER_SAVED()

	/* save CP0 registers */
	mfhi t0
	mflo t1
	OP_STOREREG t0, ESF_O(hi)(sp)
	OP_STOREREG t1, ESF_O(lo)(sp)
	mfc0 t0, CP0_EPC
	OP_STOREREG t0, ESF_O(epc)(sp)
	mfc0 t1, CP0_BADVADDR
	OP_STOREREG t1, ESF_O(badvaddr)(sp)
	mfc0 t0, CP0_STATUS
	OP_STOREREG t0, ESF_O(status)(sp)
	mfc0 t1, CP0_CAUSE
	OP_STOREREG t1, ESF_O(cause)(sp)

	/*
	 * Check if exception is the result of an interrupt or not.
	 */
	li k0, CAUSE_EXP_MASK
	and k1, k0, t1
	srl k1, k1, CAUSE_EXP_SHIFT

	/* ExcCode == 8 (SYSCALL) ? */
	li k0, 8
	beq k0, k1, is_kernel_syscall

	/* a0 = ((cause & status) & CAUSE_IP_MASK) >> CAUSE_IP_SHIFT */
	and t1, t1, t0
	li a0, CAUSE_IP_MASK
	and a0, a0, t1
	srl a0, a0, CAUSE_IP_SHIFT

	/* ExcCode == 0 (INTERRUPT) ? if not, go to unhandled */
	bnez k1, unhandled

	/* cause IP_MASK != 0 ? */
	bnez a0, is_interrupt

unhandled:
	move a0, sp
	jal z_mips_fault
	eret

is_kernel_syscall:
	/*
	 * A syscall is the result of an syscall instruction, in which case the
	 * EPC will contain the address of the syscall instruction.
	 * Increment saved EPC by 4 to prevent triggering the same syscall
	 * again upon exiting the ISR.
	 */
	OP_LOADREG k0, ESF_O(epc)(sp)
	addi k0, k0, 4
	OP_STOREREG k0, ESF_O(epc)(sp)

#ifdef CONFIG_IRQ_OFFLOAD
	/*
	 * Determine if the system call is the result of an IRQ offloading.
	 * Done by checking if _offload_routine is not pointing to NULL.
	 * If NULL, jump to reschedule to perform a context-switch, otherwise,
	 * jump to is_interrupt to handle the IRQ offload.
	 */
	la t0, _offload_routine
	OP_LOADREG t1, 0(t0)
	/*
	 * Put 0 into a0: call z_mips_enter_irq() with ipending==0
	 * to prevent spurious interrupt.
	 */
	move a0, zero
	bnez t1, is_interrupt
#endif /* CONFIG_IRQ_OFFLOAD */

	/*
	 * Go to reschedule to handle context-switch
	 */
	j reschedule

is_interrupt:
	/*
	 * Save current thread stack pointer and switch
	 * stack pointer to interrupt stack.
	 */

	/* Save thread stack pointer to temp register k0 */
	move k0, sp

	/* Switch to interrupt stack */
	la k1, _kernel
	OP_LOADREG sp, _kernel_offset_to_irq_stack(k1)

	/*
	 * Save thread stack pointer on interrupt stack
	 */
	addi sp, sp, -16
	OP_STOREREG k0, 0(sp)

on_irq_stack:
	/*
	 * Enter C interrupt handling code. Value of ipending will be the
	 * function parameter since we put it in a0
	 */
	jal z_mips_enter_irq

on_thread_stack:
	/* Restore thread stack pointer */
	OP_LOADREG sp, 0(sp)

#ifdef CONFIG_PREEMPT_ENABLED
	/*
	 * Check if we need to perform a reschedule
	 */

	/* Get pointer to _kernel.current */
	OP_LOADREG t2, _kernel_offset_to_current(k1)

	/*
	 * Check if next thread to schedule is current thread.
	 * If yes do not perform a reschedule
	 */
	OP_LOADREG t3, _kernel_offset_to_ready_q_cache(k1)
	beq t3, t2, no_reschedule
#else
	j no_reschedule
#endif /* CONFIG_PREEMPT_ENABLED */

reschedule:
#ifdef CONFIG_INSTRUMENT_THREAD_SWITCHING
	jal z_thread_mark_switched_out
#endif
	/*
	 * Check if the current thread is the same as the thread on the ready Q. If
	 * so, do not reschedule.
	 * Note:
	 *   Sometimes this code is execute back-to-back before the target thread
	 *   has a chance to run. If this happens, the current thread and the
	 *   target thread will be the same.
	 */
	la t0, _kernel
	OP_LOADREG t2, _kernel_offset_to_current(t0)
	OP_LOADREG t3, _kernel_offset_to_ready_q_cache(t0)
	beq t2, t3, no_reschedule

	/* Get reference to _kernel */
	la t0, _kernel

	/* Get pointer to _kernel.current */
	OP_LOADREG t1, _kernel_offset_to_current(t0)

	/*
	 * Save callee-saved registers of current kernel thread
	 * prior to handle context-switching
	 */
	STORE_CALLEE_SAVED(t1)

skip_callee_saved_reg:

	/*
	 * Save stack pointer of current thread and set the default return value
	 * of z_swap to _k_neg_eagain for the thread.
	 */
	OP_STOREREG sp, _thread_offset_to_sp(t1)
	la t2, _k_neg_eagain
	lw t3, 0(t2)
	sw t3, _thread_offset_to_swap_return_value(t1)

	/* Get next thread to schedule. */
	OP_LOADREG t1, _kernel_offset_to_ready_q_cache(t0)

	/*
	 * Set _kernel.current to new thread loaded in t1
	 */
	OP_STOREREG t1, _kernel_offset_to_current(t0)

	/* Switch to new thread stack */
	OP_LOADREG sp, _thread_offset_to_sp(t1)

	/* Restore callee-saved registers of new thread */
	LOAD_CALLEE_SAVED(t1)

#ifdef CONFIG_INSTRUMENT_THREAD_SWITCHING
	jal z_thread_mark_switched_in
#endif

	/* fallthrough */

no_reschedule:
	/* restore CP0 */
	OP_LOADREG t1, ESF_O(hi)(sp)
	OP_LOADREG t2, ESF_O(lo)(sp)
	mthi t1
	mtlo t2

	OP_LOADREG k0, ESF_O(epc)(sp)
	mtc0 k0, CP0_EPC
	OP_LOADREG k1, ESF_O(status)(sp)
	mtc0 k1, CP0_STATUS
	ehb

	/* Restore caller-saved registers from thread stack */
	LOAD_CALLER_SAVED()

	/* exit ISR */
	eret
