/*
 * Copyright (c) 2016 Jean-Paul Etienne <fractalclone@gmail.com>
 * Copyright (c) 2018 Foundries.io Ltd
 * Copyright (c) 2020 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <toolchain.h>
#include <linker/sections.h>
#include <offsets_short.h>
#include <arch/cpu.h>
#include <sys/util.h>
#include <kernel.h>
#include <syscall.h>
#include <arch/riscv/csr.h>
#include <arch/riscv/syscall.h>
#include "asm_macros.inc"

/* Convenience macros for loading/storing register states. */

#define DO_FP_CALLER_SAVED(op, reg) \
	op ft0, __z_arch_esf_t_ft0_OFFSET(reg)	 ;\
	op ft1, __z_arch_esf_t_ft1_OFFSET(reg)	 ;\
	op ft2, __z_arch_esf_t_ft2_OFFSET(reg)	 ;\
	op ft3, __z_arch_esf_t_ft3_OFFSET(reg)	 ;\
	op ft4, __z_arch_esf_t_ft4_OFFSET(reg)	 ;\
	op ft5, __z_arch_esf_t_ft5_OFFSET(reg)	 ;\
	op ft6, __z_arch_esf_t_ft6_OFFSET(reg)	 ;\
	op ft7, __z_arch_esf_t_ft7_OFFSET(reg)	 ;\
	op ft8, __z_arch_esf_t_ft8_OFFSET(reg)	 ;\
	op ft9, __z_arch_esf_t_ft9_OFFSET(reg)	 ;\
	op ft10, __z_arch_esf_t_ft10_OFFSET(reg) ;\
	op ft11, __z_arch_esf_t_ft11_OFFSET(reg) ;\
	op fa0, __z_arch_esf_t_fa0_OFFSET(reg)	 ;\
	op fa1, __z_arch_esf_t_fa1_OFFSET(reg)	 ;\
	op fa2, __z_arch_esf_t_fa2_OFFSET(reg)	 ;\
	op fa3, __z_arch_esf_t_fa3_OFFSET(reg)	 ;\
	op fa4, __z_arch_esf_t_fa4_OFFSET(reg)	 ;\
	op fa5, __z_arch_esf_t_fa5_OFFSET(reg)	 ;\
	op fa6, __z_arch_esf_t_fa6_OFFSET(reg)	 ;\
	op fa7, __z_arch_esf_t_fa7_OFFSET(reg)	 ;

#define DO_FP_CALLEE_SAVED(op, reg) \
	op fs0, _thread_offset_to_fs0(reg)   ;\
	op fs1, _thread_offset_to_fs1(reg)   ;\
	op fs2, _thread_offset_to_fs2(reg)   ;\
	op fs3, _thread_offset_to_fs3(reg)   ;\
	op fs4, _thread_offset_to_fs4(reg)   ;\
	op fs5, _thread_offset_to_fs5(reg)   ;\
	op fs6, _thread_offset_to_fs6(reg)   ;\
	op fs7, _thread_offset_to_fs7(reg)   ;\
	op fs8, _thread_offset_to_fs8(reg)   ;\
	op fs9, _thread_offset_to_fs9(reg)   ;\
	op fs10, _thread_offset_to_fs10(reg) ;\
	op fs11, _thread_offset_to_fs11(reg) ;

#define DO_CALLEE_SAVED(op, reg) \
	op s0, _thread_offset_to_tp(reg)		;\
	op s0, _thread_offset_to_s0(reg)		;\
	op s1, _thread_offset_to_s1(reg)		;\
	op s2, _thread_offset_to_s2(reg)		;\
	op s3, _thread_offset_to_s3(reg)		;\
	op s4, _thread_offset_to_s4(reg)		;\
	op s5, _thread_offset_to_s5(reg)		;\
	op s6, _thread_offset_to_s6(reg)		;\
	op s7, _thread_offset_to_s7(reg)		;\
	op s8, _thread_offset_to_s8(reg)		;\
	op s9, _thread_offset_to_s9(reg)		;\
	op s10, _thread_offset_to_s10(reg)		;\
	op s11, _thread_offset_to_s11(reg)		;

#define DO_CALLER_SAVED(op) \
	op ra, __z_arch_esf_t_ra_OFFSET(sp)		;\
	op t0, __z_arch_esf_t_t0_OFFSET(sp)		;\
	op t1, __z_arch_esf_t_t1_OFFSET(sp)		;\
	op t2, __z_arch_esf_t_t2_OFFSET(sp)		;\
	op t3, __z_arch_esf_t_t3_OFFSET(sp)		;\
	op t4, __z_arch_esf_t_t4_OFFSET(sp)		;\
	op t5, __z_arch_esf_t_t5_OFFSET(sp)		;\
	op t6, __z_arch_esf_t_t6_OFFSET(sp)		;\
	op a0, __z_arch_esf_t_a0_OFFSET(sp)		;\
	op a1, __z_arch_esf_t_a1_OFFSET(sp)		;\
	op a2, __z_arch_esf_t_a2_OFFSET(sp)		;\
	op a3, __z_arch_esf_t_a3_OFFSET(sp)		;\
	op a4, __z_arch_esf_t_a4_OFFSET(sp)		;\
	op a5, __z_arch_esf_t_a5_OFFSET(sp)		;\
	op a6, __z_arch_esf_t_a6_OFFSET(sp)		;\
	op a7, __z_arch_esf_t_a7_OFFSET(sp)		;

#define ASSUME_EQUAL(x, y)	.if x != y; .err; .endif

/* imports */
GDATA(_sw_isr_table)
GTEXT(__soc_is_irq)
GTEXT(__soc_handle_irq)
GTEXT(_Fault)
#ifdef CONFIG_RISCV_SOC_CONTEXT_SAVE
GTEXT(__soc_save_context)
GTEXT(__soc_restore_context)
#endif /* CONFIG_RISCV_SOC_CONTEXT_SAVE */

GTEXT(z_riscv_fatal_error)
GTEXT(_k_neg_eagain)
GTEXT(_is_next_thread_current)
GTEXT(z_get_next_ready_thread)

#ifdef CONFIG_INSTRUMENT_THREAD_SWITCHING
GTEXT(z_thread_mark_switched_in)
GTEXT(z_thread_mark_switched_out)
#ifdef CONFIG_TRACING
GTEXT(sys_trace_isr_enter)
#endif
#endif

#ifdef CONFIG_USERSPACE
GDATA(_k_syscall_table)
GTEXT(z_riscv_configure_user_allowed_stack)
#endif

#ifdef CONFIG_PMP_STACK_GUARD
GTEXT(z_riscv_configure_stack_guard)
#endif

/* exports */
GTEXT(__irq_wrapper)

/* use ABI name of registers for the sake of simplicity */

/*
 * Generic architecture-level IRQ handling, along with callouts to
 * SoC-specific routines.
 *
 * Architecture level IRQ handling includes basic context save/restore
 * of standard registers and calling ISRs registered at Zephyr's driver
 * level.
 *
 * Since RISC-V does not completely prescribe IRQ handling behavior,
 * implementations vary (some implementations also deviate from
 * what standard behavior is defined). Hence, the arch level code expects
 * the following functions to be provided at the SOC level:
 *
 *     - __soc_is_irq: decide if we're handling an interrupt or an exception
 *     - __soc_handle_irq: handle SoC-specific details for a pending IRQ
 *       (e.g. clear a pending bit in a SoC-specific register)
 *
 * If CONFIG_RISCV_SOC_CONTEXT_SAVE=y, calls to SoC-level context save/restore
 * routines are also made here. For details, see the Kconfig help text.
 */

/*
 * Handler called upon each exception/interrupt/fault
 * In this architecture, system call (ECALL) is used to perform context
 * switching or IRQ offloading (when enabled).
 */
SECTION_FUNC(exception.entry, __irq_wrapper)

#ifdef CONFIG_USERSPACE
	/*
	 * The scratch register contains either the privileged stack pointer
	 * to use when interrupting a user mode thread, or 0 when interrupting
	 * kernel mode in which case the current stack should be used.
	 */
	csrrw sp, mscratch, sp
	bnez sp, 1f

	/* restore privileged stack pointer and zero the scratch reg */
	csrrw sp, mscratch, sp
1:
#endif

	/* Save caller-saved registers on current thread stack. */
	addi sp, sp, -__z_arch_esf_t_SIZEOF
	DO_CALLER_SAVED(sr)		;

	/* Save MEPC register */
	csrr t0, mepc
	sr t0, __z_arch_esf_t_mepc_OFFSET(sp)

	/* Save MSTATUS register */
	csrr t0, mstatus
	sr t0, __z_arch_esf_t_mstatus_OFFSET(sp)

#ifdef CONFIG_USERSPACE
	/*
	 * The scratch register now contains either the user mode stack
	 * pointer, or 0 if entered from kernel mode. Retrieve that value
	 * and zero the scratch register as we are in kernel mode now.
	 */
	csrrw t0, mscratch, zero
	bnez t0, 1f

	/* came from kernel mode: adjust stack value */
	add t0, sp, __z_arch_esf_t_SIZEOF
1:
	/* save stack value to be restored later */
	sr t0, __z_arch_esf_t_sp_OFFSET(sp)

	/* Clear user mode variable */
	la t0, is_user_mode
	sw zero, 0(t0)
#endif

#if defined(CONFIG_FPU) && defined(CONFIG_FPU_SHARING)
	/* Assess whether floating-point registers need to be saved. */
	la t0, _kernel
	lr t0, _kernel_offset_to_current(t0)
	lb t0, _thread_offset_to_user_options(t0)
	andi t0, t0, K_FP_REGS
	beqz t0, skip_store_fp_caller_saved
	DO_FP_CALLER_SAVED(fsr, sp)
skip_store_fp_caller_saved:
#endif /* CONFIG_FPU && CONFIG_FPU_SHARING */

#ifdef CONFIG_RISCV_SOC_CONTEXT_SAVE
	/* Handle context saving at SOC level. */
	addi a0, sp, __z_arch_esf_t_soc_context_OFFSET
	jal ra, __soc_save_context
#endif /* CONFIG_RISCV_SOC_CONTEXT_SAVE */

	/*
	 * Check if exception is the result of an interrupt or not.
	 * (SOC dependent). Following the RISC-V architecture spec, the MSB
	 * of the mcause register is used to indicate whether an exception
	 * is the result of an interrupt or an exception/fault. But for some
	 * SOCs (like pulpino or riscv-qemu), the MSB is never set to indicate
	 * interrupt. Hence, check for interrupt/exception via the __soc_is_irq
	 * function (that needs to be implemented by each SOC). The result is
	 * returned via register a0 (1: interrupt, 0 exception)
	 */
	jal ra, __soc_is_irq

	/* If a0 != 0, jump to is_interrupt */
	bnez a0, is_interrupt

#ifdef CONFIG_PMP_STACK_GUARD
	li t0, MSTATUS_MPRV
	csrs mstatus, t0
#endif

	/*
	 * If the exception is the result of an ECALL, check whether to
	 * perform a context-switch or an IRQ offload. Otherwise call _Fault
	 * to report the exception.
	 */
	csrr t0, mcause
	li t2, SOC_MCAUSE_EXP_MASK
	and t0, t0, t2

	/*
	 * If mcause == SOC_MCAUSE_ECALL_EXP, handle system call from
	 * kernel thread.
	 */
	li t1, SOC_MCAUSE_ECALL_EXP
	beq t0, t1, is_kernel_syscall

#ifdef CONFIG_USERSPACE
	/*
	 * If mcause == SOC_MCAUSE_USER_ECALL_EXP, handle system call
	 * for user mode thread.
	 */
	li t1, SOC_MCAUSE_USER_ECALL_EXP
	beq t0, t1, is_user_syscall
#endif /* CONFIG_USERSPACE */

	/*
	 * Call _Fault to handle exception.
	 * Stack pointer is pointing to a z_arch_esf_t structure, pass it
	 * to _Fault (via register a0).
	 * If _Fault shall return, set return address to
	 * no_reschedule to restore stack.
	 */
	mv a0, sp
	la ra, no_reschedule
	tail _Fault

is_kernel_syscall:
	/*
	 * A syscall is the result of an ecall instruction, in which case the
	 * MEPC will contain the address of the ecall instruction.
	 * Increment saved MEPC by 4 to prevent triggering the same ecall
	 * again upon exiting the ISR.
	 *
	 * It's safe to always increment by 4, even with compressed
	 * instructions, because the ecall instruction is always 4 bytes.
	 */
	lr t0, __z_arch_esf_t_mepc_OFFSET(sp)
	addi t0, t0, 4
	sr t0, __z_arch_esf_t_mepc_OFFSET(sp)

	/* Determine what to do. Operation code is in a7. */
	lr a7, __z_arch_esf_t_a7_OFFSET(sp)

	ASSUME_EQUAL(RV_ECALL_CONTEXT_SWITCH, 0)
	beqz a7, reschedule

#if defined(CONFIG_IRQ_OFFLOAD)
	addi a7, a7, -1
	ASSUME_EQUAL(RV_ECALL_IRQ_OFFLOAD, 1)
	beqz a7, do_irq_offload
	addi a7, a7, -1
#else
	addi a7, a7, -2
#endif

	ASSUME_EQUAL(RV_ECALL_RUNTIME_EXCEPT, 2)
	beqz a7, do_fault

	/* default fault code is K_ERR_KERNEL_OOPS */
	li a0, 3
	j 1f

do_fault:
	/* Handle RV_ECALL_RUNTIME_EXCEPT. Retrieve reason in a0, esf in A1. */
	lr a0, __z_arch_esf_t_a0_OFFSET(sp)
1:	mv a1, sp
	tail z_riscv_fatal_error

#if defined(CONFIG_IRQ_OFFLOAD)
do_irq_offload:
	/*
	 * Retrieve provided routine and argument from the stack.
	 * Routine pointer is in saved a0, argument in saved a1
	 * so we load them with a1/a0 (reversed).
	 */
	lr a1, __z_arch_esf_t_a0_OFFSET(sp)
	lr a0, __z_arch_esf_t_a1_OFFSET(sp)

	/* Increment _kernel.cpus[0].nested variable */
	la t2, _kernel
	lw t3, _kernel_offset_to_nested(t2)
	addi t4, t3, 1
	sw t4, _kernel_offset_to_nested(t2)
	bnez t3, 1f

	/* Switch to interrupt stack */
	mv t0, sp
	lr sp, _kernel_offset_to_irq_stack(t2)

	/* Save thread stack pointer on interrupt stack */
	addi sp, sp, -16
	sr t0, 0(sp)
1:
	/* Execute provided routine (argument is in a0 already). */
	jalr ra, a1, 0

	/* Leave through the regular IRQ exit path */
	j irq_done
#endif /* CONFIG_IRQ_OFFLOAD */

#ifdef CONFIG_USERSPACE
is_user_syscall:
	/* It is safe to re-enable IRQs now */
	csrs mstatus, MSTATUS_IEN

	/*
	 * Same as for is_kernel_syscall: increment saved MEPC by 4 to
	 * prevent triggering the same ecall again upon exiting the ISR.
	 */
	lr t1, __z_arch_esf_t_mepc_OFFSET(sp)
	addi t1, t1, 4
	sr t1, __z_arch_esf_t_mepc_OFFSET(sp)

	/* Restore argument registers from user stack */
	lr a0, __z_arch_esf_t_a0_OFFSET(sp)
	lr a1, __z_arch_esf_t_a1_OFFSET(sp)
	lr a2, __z_arch_esf_t_a2_OFFSET(sp)
	lr a3, __z_arch_esf_t_a3_OFFSET(sp)
	lr a4, __z_arch_esf_t_a4_OFFSET(sp)
	lr a5, __z_arch_esf_t_a5_OFFSET(sp)
	mv a6, sp
	lr a7, __z_arch_esf_t_a7_OFFSET(sp)

	/* validate syscall limit */
	li t0, K_SYSCALL_LIMIT
	bltu a7, t0, valid_syscall_id

	/* bad syscall id.  Set arg1 to bad id and set call_id to SYSCALL_BAD */
	mv a0, a7
	li a7, K_SYSCALL_BAD

valid_syscall_id:

	la t0, _k_syscall_table

	slli t1, a7, RV_REGSHIFT	# Determine offset from indice value
	add t0, t0, t1			# Table addr + offset = function addr
	lr t3, 0(t0)			# Load function address

	/* Execute syscall function */
	jalr ra, t3, 0

	/* Update a0 (return value) on the stack */
	sr a0, __z_arch_esf_t_a0_OFFSET(sp)

	/* Disable IRQs again before leaving */
	csrc mstatus, MSTATUS_IEN
	j no_reschedule
#endif /* CONFIG_USERSPACE */

is_interrupt:
	/* Increment _kernel.cpus[0].nested variable */
	la t2, _kernel
	lw t3, _kernel_offset_to_nested(t2)
	addi t4, t3, 1
	sw t4, _kernel_offset_to_nested(t2)
	bnez t3, on_irq_stack

	/* Switch to interrupt stack */
	mv t0, sp
	lr sp, _kernel_offset_to_irq_stack(t2)

	/*
	 * Save thread stack pointer on interrupt stack
	 * In RISC-V, stack pointer needs to be 16-byte aligned
	 */
	addi sp, sp, -16
	sr t0, 0(sp)

on_irq_stack:

#ifdef CONFIG_TRACING_ISR
	call sys_trace_isr_enter
#endif

	/* Get IRQ causing interrupt */
	csrr a0, mcause
	li t0, SOC_MCAUSE_EXP_MASK
	and a0, a0, t0

	/*
	 * Clear pending IRQ generating the interrupt at SOC level
	 * Pass IRQ number to __soc_handle_irq via register a0
	 */
	jal ra, __soc_handle_irq

	/*
	 * Call corresponding registered function in _sw_isr_table.
	 * (table is 2-word wide, we should shift index accordingly)
	 */
	la t0, _sw_isr_table
	slli a0, a0, (RV_REGSHIFT + 1)
	add t0, t0, a0

	/* Load argument in a0 register */
	lr a0, 0(t0)

	/* Load ISR function address in register t1 */
	lr t1, RV_REGSIZE(t0)

	/* Call ISR function */
	jalr ra, t1, 0

irq_done:
	/* Decrement _kernel.cpus[0].nested variable */
	la t1, _kernel
	lw t2, _kernel_offset_to_nested(t1)
	addi t2, t2, -1
	sw t2, _kernel_offset_to_nested(t1)
	bnez t2, no_reschedule

	/* nested count is back to 0: Return to thread stack */
	lr sp, 0(sp)

#ifdef CONFIG_STACK_SENTINEL
	call z_check_stack_sentinel
#endif

reschedule:
	/* Get reference to _kernel */
	la t1, _kernel

	/*
	 * Check if next thread to schedule is current thread.
	 * If yes do not perform a reschedule
	 */
	lr t2, _kernel_offset_to_current(t1)
	lr t3, _kernel_offset_to_ready_q_cache(t1)
	beq t3, t2, no_reschedule

#if CONFIG_INSTRUMENT_THREAD_SWITCHING
	call z_thread_mark_switched_out
#endif

	/* Get reference to _kernel */
	la t0, _kernel

	/* Get pointer to _kernel.current */
	lr t1, _kernel_offset_to_current(t0)

	/*
	 * Save callee-saved registers of current kernel thread
	 * prior to handle context-switching
	 */
	DO_CALLEE_SAVED(sr, t1)

#if defined(CONFIG_FPU) && defined(CONFIG_FPU_SHARING)
	/* Assess whether floating-point registers need to be saved. */
	lb t2, _thread_offset_to_user_options(t1)
	andi t2, t2, K_FP_REGS
	beqz t2, skip_store_fp_callee_saved
	frcsr t2
	sw t2, _thread_offset_to_fcsr(t1)
	DO_FP_CALLEE_SAVED(fsr, t1)
skip_store_fp_callee_saved:
#endif /* CONFIG_FPU && CONFIG_FPU_SHARING */

	/*
	 * Save stack pointer of current thread and set the default return value
	 * of z_swap to _k_neg_eagain for the thread.
	 */
	sr sp, _thread_offset_to_sp(t1)
	la t2, _k_neg_eagain
	lw t3, 0(t2)
	sw t3, _thread_offset_to_swap_return_value(t1)

	/* Get next thread to schedule. */
	lr t1, _kernel_offset_to_ready_q_cache(t0)

	/* Set _kernel.current to new thread loaded in t1 */
	sr t1, _kernel_offset_to_current(t0)

	/* Switch to new thread stack */
	lr sp, _thread_offset_to_sp(t1)

	/* Restore callee-saved registers of new thread */
	DO_CALLEE_SAVED(lr, t1)

#if defined(CONFIG_FPU) && defined(CONFIG_FPU_SHARING)
	/* Determine if we need to restore floating-point registers. */
	lb t2, _thread_offset_to_user_options(t1)
	andi t2, t2, K_FP_REGS
	beqz t2, skip_load_fp_callee_saved

	/*
	 * If we are switching from a thread with floating-point disabled the
	 * mstatus FS bits will still be cleared, which can cause an illegal
	 * instruction fault. Set the FS state before restoring the registers.
	 * mstatus will be restored later on.
	 */
	li t2, MSTATUS_FS_INIT
	csrs mstatus, t2

	lw t2, _thread_offset_to_fcsr(t1)
	fscsr t2
	DO_FP_CALLEE_SAVED(flr, t1)

skip_load_fp_callee_saved:
#endif /* CONFIG_FPU && CONFIG_FPU_SHARING */

#ifdef CONFIG_PMP_STACK_GUARD
	mv a0, t1	/* kernel current */
	jal ra, z_riscv_configure_stack_guard
#endif /* CONFIG_PMP_STACK_GUARD */

#ifdef CONFIG_USERSPACE
	la t0, _kernel
	lr a0, _kernel_offset_to_current(t0)
	jal ra, z_riscv_configure_user_allowed_stack
#endif /* CONFIG_USERSPACE */

#if CONFIG_INSTRUMENT_THREAD_SWITCHING
	call z_thread_mark_switched_in
#endif

no_reschedule:

#ifdef CONFIG_RISCV_SOC_CONTEXT_SAVE
	/* Restore context at SOC level */
	addi a0, sp, __z_arch_esf_t_soc_context_OFFSET
	jal ra, __soc_restore_context
#endif /* CONFIG_RISCV_SOC_CONTEXT_SAVE */

#if defined(CONFIG_FPU) && defined(CONFIG_FPU_SHARING)
	/*
	 * Determine if we need to restore floating-point registers. This needs
	 * to happen before restoring integer registers to avoid stomping on
	 * t0.
	 */
	la t0, _kernel
	lr t0, _kernel_offset_to_current(t0)
	lb t0, _thread_offset_to_user_options(t0)
	andi t0, t0, K_FP_REGS
	beqz t0, skip_load_fp_caller_saved
	DO_FP_CALLER_SAVED(flr, sp)
skip_load_fp_caller_saved:
#endif /* CONFIG_FPU && CONFIG_FPU_SHARING */

	/* Restore MEPC register */
	lr t0, __z_arch_esf_t_mepc_OFFSET(sp)
	csrw mepc, t0

	/* Restore MSTATUS register */
	lr t4, __z_arch_esf_t_mstatus_OFFSET(sp)
	csrw mstatus, t4

#ifdef CONFIG_USERSPACE
	/*
	 * Check if we are returning to user mode. If so then we must
	 * set is_user_mode to true and load the scratch register with
	 * the stack pointer to be used with the next exception to come.
	 */
	li t1, MSTATUS_MPP
	and t0, t4, t1
	bnez t0, 1f

	/* Set user mode variable */
	li t0, 1
	la t1, is_user_mode
	sw t0, 0(t1)

	/* load scratch reg with stack pointer for next exception entry */
	add t0, sp, __z_arch_esf_t_SIZEOF
	csrw mscratch, t0
1:
#endif

	/* Restore caller-saved registers from thread stack */
	DO_CALLER_SAVED(lr)

#ifdef CONFIG_USERSPACE
	/* retrieve saved stack pointer */
	lr sp, __z_arch_esf_t_sp_OFFSET(sp)
#else
	/* remove esf from the stack */
	addi sp, sp, __z_arch_esf_t_SIZEOF
#endif

	/* Call SOC_ERET to exit ISR */
	SOC_ERET
