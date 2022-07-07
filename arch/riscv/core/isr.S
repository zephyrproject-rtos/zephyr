/*
 * Copyright (c) 2016 Jean-Paul Etienne <fractalclone@gmail.com>
 * Copyright (c) 2018 Foundries.io Ltd
 * Copyright (c) 2020 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <offsets_short.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <zephyr/syscall.h>
#include <zephyr/arch/riscv/csr.h>
#include <zephyr/arch/riscv/syscall.h>
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

#define DO_CALLER_SAVED(op) \
	RV_E(	op t0, __z_arch_esf_t_t0_OFFSET(sp)	);\
	RV_E(	op t1, __z_arch_esf_t_t1_OFFSET(sp)	);\
	RV_E(	op t2, __z_arch_esf_t_t2_OFFSET(sp)	);\
	RV_I(	op t3, __z_arch_esf_t_t3_OFFSET(sp)	);\
	RV_I(	op t4, __z_arch_esf_t_t4_OFFSET(sp)	);\
	RV_I(	op t5, __z_arch_esf_t_t5_OFFSET(sp)	);\
	RV_I(	op t6, __z_arch_esf_t_t6_OFFSET(sp)	);\
	RV_E(	op a0, __z_arch_esf_t_a0_OFFSET(sp)	);\
	RV_E(	op a1, __z_arch_esf_t_a1_OFFSET(sp)	);\
	RV_E(	op a2, __z_arch_esf_t_a2_OFFSET(sp)	);\
	RV_E(	op a3, __z_arch_esf_t_a3_OFFSET(sp)	);\
	RV_E(	op a4, __z_arch_esf_t_a4_OFFSET(sp)	);\
	RV_E(	op a5, __z_arch_esf_t_a5_OFFSET(sp)	);\
	RV_I(	op a6, __z_arch_esf_t_a6_OFFSET(sp)	);\
	RV_I(	op a7, __z_arch_esf_t_a7_OFFSET(sp)	);\
	RV_E(	op ra, __z_arch_esf_t_ra_OFFSET(sp)	)

#ifdef CONFIG_SMP
#define GET_CURRENT_CPU(dst, tmp) \
	csrr tmp, mhartid				;\
	la dst, _kernel + ___kernel_t_cpus_OFFSET	;\
	shiftmul_add dst, tmp, ___cpu_t_SIZEOF
#else
#define GET_CURRENT_CPU(dst, tmp) \
	la dst, _kernel + ___kernel_t_cpus_OFFSET
#endif

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
GTEXT(z_get_next_switch_handle)
GTEXT(z_riscv_switch)
GTEXT(z_riscv_thread_start)

#ifdef CONFIG_TRACING
GTEXT(sys_trace_isr_enter)
GTEXT(sys_trace_isr_exit)
#endif

#ifdef CONFIG_USERSPACE
GDATA(_k_syscall_table)
#endif

/* exports */
GTEXT(_isr_wrapper)

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
 */
SECTION_FUNC(exception.entry, _isr_wrapper)

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

	/* Save s0 in the esf and load it with &_current_cpu. */
	sr s0, __z_arch_esf_t_s0_OFFSET(sp)
	GET_CURRENT_CPU(s0, t0)

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

	/* Make sure tls pointer is sane */
	lr t0, ___cpu_t_current_OFFSET(s0)
	lr tp, _thread_offset_to_tls(t0)

	/* Clear our per-thread usermode flag */
	lui t0, %tprel_hi(is_user_mode)
	add t0, t0, tp, %tprel_add(is_user_mode)
	sb zero, %tprel_lo(is_user_mode)(t0)
#endif

	/* Save MEPC register */
	csrr t0, mepc
	sr t0, __z_arch_esf_t_mepc_OFFSET(sp)

	/* Save MSTATUS register */
	csrr t2, mstatus
	sr t2, __z_arch_esf_t_mstatus_OFFSET(sp)

#if defined(CONFIG_FPU) && defined(CONFIG_FPU_SHARING)
	/* Assess whether floating-point registers need to be saved. */
	li t1, MSTATUS_FS_INIT
	and t0, t2, t1
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

#ifdef CONFIG_PMP_STACK_GUARD
	/* Re-activate PMP for m-mode */
	li t1, MSTATUS_MPP
	csrc mstatus, t1
	li t1, MSTATUS_MPRV
	csrs mstatus, t1
#endif

	/* Determine what to do. Operation code is in t0. */
	lr t0, __z_arch_esf_t_t0_OFFSET(sp)

	ASSUME_EQUAL(RV_ECALL_RUNTIME_EXCEPT, 0)
	beqz t0, do_fault

#if defined(CONFIG_IRQ_OFFLOAD)
	addi t0, t0, -1
	ASSUME_EQUAL(RV_ECALL_IRQ_OFFLOAD, 1)
	beqz t0, do_irq_offload
#endif

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

	/* Increment _current_cpu->nested */
	lw t1, ___cpu_t_nested_OFFSET(s0)
	addi t2, t1, 1
	sw t2, ___cpu_t_nested_OFFSET(s0)
	bnez t1, 1f

	/* Switch to interrupt stack */
	mv t0, sp
	lr sp, ___cpu_t_irq_stack_OFFSET(s0)

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

#ifdef CONFIG_PMP_STACK_GUARD
	/*
	 * We came from userspace and need to reconfigure the
	 * PMP for kernel mode stack guard.
	 */
	lr a0, ___cpu_t_current_OFFSET(s0)
	call z_riscv_pmp_stackguard_enable
#endif

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
	lr t0, __z_arch_esf_t_t0_OFFSET(sp)
#if defined(CONFIG_RISCV_ISA_RV32E)
	/* Stack alignment for RV32E is 4 bytes */
	addi sp, sp, -4
	mv t1, sp
	sw t1, 0(sp)
#else
	mv a6, sp
#endif /* CONFIG_RISCV_ISA_RV32E */

	/* validate syscall limit */
	li t1, K_SYSCALL_LIMIT
	bltu t0, t1, valid_syscall_id

	/* bad syscall id.  Set arg1 to bad id and set call_id to SYSCALL_BAD */
	mv a0, t0
	li t0, K_SYSCALL_BAD

valid_syscall_id:

	la t2, _k_syscall_table

	slli t1, t0, RV_REGSHIFT	# Determine offset from indice value
	add t2, t2, t1			# Table addr + offset = function addr
	lr t2, 0(t2)			# Load function address

	/* Execute syscall function */
	jalr ra, t2, 0

#if defined(CONFIG_RISCV_ISA_RV32E)
	addi sp, sp, 4
#endif /* CONFIG_RISCV_ISA_RV32E */

	/* Update a0 (return value) on the stack */
	sr a0, __z_arch_esf_t_a0_OFFSET(sp)

	/* Disable IRQs again before leaving */
	csrc mstatus, MSTATUS_IEN
	j might_have_rescheduled
#endif /* CONFIG_USERSPACE */

is_interrupt:

#ifdef CONFIG_PMP_STACK_GUARD
#ifdef CONFIG_USERSPACE
	/*
	 * If we came from userspace then we need to reconfigure the
	 * PMP for kernel mode stack guard.
	 */
	lr t0, __z_arch_esf_t_mstatus_OFFSET(sp)
	li t1, MSTATUS_MPP
	and t0, t0, t1
	bnez t0, 1f
	lr a0, ___cpu_t_current_OFFSET(s0)
	call z_riscv_pmp_stackguard_enable
	j 2f
#endif /* CONFIG_USERSPACE */
1:	/* Re-activate PMP for m-mode */
	li t1, MSTATUS_MPP
	csrc mstatus, t1
	li t1, MSTATUS_MPRV
	csrs mstatus, t1
2:
#endif

	/* Increment _current_cpu->nested */
	lw t1, ___cpu_t_nested_OFFSET(s0)
	addi t2, t1, 1
	sw t2, ___cpu_t_nested_OFFSET(s0)
	bnez t1, on_irq_stack

	/* Switch to interrupt stack */
	mv t0, sp
	lr sp, ___cpu_t_irq_stack_OFFSET(s0)

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

#ifdef CONFIG_TRACING_ISR
    call sys_trace_isr_exit
#endif

irq_done:
	/* Decrement _current_cpu->nested */
	lw t2, ___cpu_t_nested_OFFSET(s0)
	addi t2, t2, -1
	sw t2, ___cpu_t_nested_OFFSET(s0)
	bnez t2, no_reschedule

	/* nested count is back to 0: Return to thread stack */
	lr sp, 0(sp)

#ifdef CONFIG_STACK_SENTINEL
	call z_check_stack_sentinel
#endif

reschedule:

	/* Get pointer to current thread on this CPU */
	lr a1, ___cpu_t_current_OFFSET(s0)

	/*
	 * Get next thread to schedule with z_get_next_switch_handle().
	 * We pass it a NULL as we didn't save the whole thread context yet.
	 * If no scheduling is necessary then NULL will be returned.
	 */
	addi sp, sp, -16
	sr a1, 0(sp)
	mv a0, zero
	call z_get_next_switch_handle
	lr a1, 0(sp)
	addi sp, sp, 16
	beqz a0, no_reschedule

	/*
	 * Perform context switch:
	 * a0 = new thread
	 * a1 = old thread
	 */
	call z_riscv_switch

z_riscv_thread_start:
might_have_rescheduled:
#ifdef CONFIG_SMP
	/* reload s0 with &_current_cpu as it might have changed */
	GET_CURRENT_CPU(s0, t0)
#endif

no_reschedule:

#ifdef CONFIG_RISCV_SOC_CONTEXT_SAVE
	/* Restore context at SOC level */
	addi a0, sp, __z_arch_esf_t_soc_context_OFFSET
	jal ra, __soc_restore_context
#endif /* CONFIG_RISCV_SOC_CONTEXT_SAVE */

	/* Restore MEPC register */
	lr t0, __z_arch_esf_t_mepc_OFFSET(sp)
	csrw mepc, t0

	/* Restore MSTATUS register */
	lr t2, __z_arch_esf_t_mstatus_OFFSET(sp)
	csrrw t0, mstatus, t2

#if defined(CONFIG_FPU) && defined(CONFIG_FPU_SHARING)
	/*
	 * Determine if we need to restore FP regs based on the previous
	 * (before the csr above) mstatus value available in t0.
	 */
	li t1, MSTATUS_FS_INIT
	and t0, t0, t1
	beqz t0, no_fp

	/* make sure FP is enabled in the restored mstatus */
	csrs mstatus, t1
	DO_FP_CALLER_SAVED(flr, sp)
	j 1f

no_fp:	/* make sure this is reflected in the restored mstatus */
	csrc mstatus, t1
1:
#endif /* CONFIG_FPU && CONFIG_FPU_SHARING */

#ifdef CONFIG_USERSPACE
	/*
	 * Check if we are returning to user mode. If so then we must
	 * set is_user_mode to true and load the scratch register with
	 * the stack pointer to be used with the next exception to come.
	 */
	li t1, MSTATUS_MPP
	and t0, t2, t1
	bnez t0, 1f

#ifdef CONFIG_PMP_STACK_GUARD
	/* Remove kernel stack guard and Reconfigure PMP for user mode */
	lr a0, ___cpu_t_current_OFFSET(s0)
	call z_riscv_pmp_usermode_enable
#endif

	/* Set our per-thread usermode flag */
	li t1, 1
	lui t0, %tprel_hi(is_user_mode)
	add t0, t0, tp, %tprel_add(is_user_mode)
	sb t1, %tprel_lo(is_user_mode)(t0)

	/* load scratch reg with stack pointer for next exception entry */
	add t0, sp, __z_arch_esf_t_SIZEOF
	csrw mscratch, t0
1:
#endif

	/* Restore s0 (it is no longer ours) */
	lr s0, __z_arch_esf_t_s0_OFFSET(sp)

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
