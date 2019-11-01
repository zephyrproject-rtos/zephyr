/*
 * Copyright (c) 2016 Jean-Paul Etienne <fractalclone@gmail.com>
 * Copyright (c) 2018 Foundries.io Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <toolchain.h>
#include <linker/sections.h>
#include <kernel_structs.h>
#include <offsets_short.h>

/* imports */
GDATA(_sw_isr_table)
GTEXT(__soc_is_irq)
GTEXT(__soc_handle_irq)
GTEXT(_Fault)
#ifdef CONFIG_RISCV_SOC_CONTEXT_SAVE
GTEXT(__soc_save_context)
GTEXT(__soc_restore_context)
#endif /* CONFIG_RISCV_SOC_CONTEXT_SAVE */

GTEXT(_k_neg_eagain)
GTEXT(_is_next_thread_current)
GTEXT(z_get_next_ready_thread)

#ifdef CONFIG_TRACING
GTEXT(sys_trace_thread_switched_in)
GTEXT(sys_trace_isr_enter)
#endif

#ifdef CONFIG_IRQ_OFFLOAD
GTEXT(_offload_routine)
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
	/* Allocate space on thread stack to save registers */
	addi sp, sp, -__z_arch_esf_t_SIZEOF

	/*
	 * Save caller-saved registers on current thread stack.
	 * NOTE: need to be updated to account for floating-point registers
	 * floating-point registers should be accounted for when corresponding
	 * config variable is set
	 */
	RV_OP_STOREREG ra, __z_arch_esf_t_ra_OFFSET(sp)
	RV_OP_STOREREG gp, __z_arch_esf_t_gp_OFFSET(sp)
	RV_OP_STOREREG tp, __z_arch_esf_t_tp_OFFSET(sp)
	RV_OP_STOREREG t0, __z_arch_esf_t_t0_OFFSET(sp)
	RV_OP_STOREREG t1, __z_arch_esf_t_t1_OFFSET(sp)
	RV_OP_STOREREG t2, __z_arch_esf_t_t2_OFFSET(sp)
	RV_OP_STOREREG t3, __z_arch_esf_t_t3_OFFSET(sp)
	RV_OP_STOREREG t4, __z_arch_esf_t_t4_OFFSET(sp)
	RV_OP_STOREREG t5, __z_arch_esf_t_t5_OFFSET(sp)
	RV_OP_STOREREG t6, __z_arch_esf_t_t6_OFFSET(sp)
	RV_OP_STOREREG a0, __z_arch_esf_t_a0_OFFSET(sp)
	RV_OP_STOREREG a1, __z_arch_esf_t_a1_OFFSET(sp)
	RV_OP_STOREREG a2, __z_arch_esf_t_a2_OFFSET(sp)
	RV_OP_STOREREG a3, __z_arch_esf_t_a3_OFFSET(sp)
	RV_OP_STOREREG a4, __z_arch_esf_t_a4_OFFSET(sp)
	RV_OP_STOREREG a5, __z_arch_esf_t_a5_OFFSET(sp)
	RV_OP_STOREREG a6, __z_arch_esf_t_a6_OFFSET(sp)
	RV_OP_STOREREG a7, __z_arch_esf_t_a7_OFFSET(sp)

	/* Save MEPC register */
	csrr t0, mepc
	RV_OP_STOREREG t0, __z_arch_esf_t_mepc_OFFSET(sp)

	/* Save SOC-specific MSTATUS register */
	csrr t0, SOC_MSTATUS_REG
	RV_OP_STOREREG t0, __z_arch_esf_t_mstatus_OFFSET(sp)

#ifdef CONFIG_RISCV_SOC_CONTEXT_SAVE
	/* Handle context saving at SOC level. */
	addi a0, sp, __z_arch_esf_t_soc_context_OFFSET
	jal ra, __soc_save_context
#endif /* CONFIG_RISCV_SOC_CONTEXT_SAVE */

#ifdef CONFIG_EXECUTION_BENCHMARKING
	call read_timer_start_of_isr
#endif

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
	addi t1, x0, 0
	bnez a0, is_interrupt

	/*
	 * If the exception is the result of an ECALL, check whether to
	 * perform a context-switch or an IRQ offload. Otherwise call _Fault
	 * to report the exception.
	 */
	csrr t0, mcause
	li t2, SOC_MCAUSE_EXP_MASK
	and t0, t0, t2
	li t1, SOC_MCAUSE_ECALL_EXP

	/*
	 * If mcause == SOC_MCAUSE_ECALL_EXP, handle system call,
	 * otherwise handle fault
	 */
	beq t0, t1, is_syscall

	/*
	 * Call _Fault to handle exception.
	 * Stack pointer is pointing to a z_arch_esf_t structure, pass it
	 * to _Fault (via register a0).
	 * If _Fault shall return, set return address to no_reschedule
	 * to restore stack.
	 */
	addi a0, sp, 0
	la ra, no_reschedule
	tail _Fault

is_syscall:
	/*
	 * A syscall is the result of an ecall instruction, in which case the
	 * MEPC will contain the address of the ecall instruction.
	 * Increment saved MEPC by 4 to prevent triggering the same ecall
	 * again upon exiting the ISR.
	 *
	 * It's safe to always increment by 4, even with compressed
	 * instructions, because the ecall instruction is always 4 bytes.
	 */
	RV_OP_LOADREG t0, __z_arch_esf_t_mepc_OFFSET(sp)
	addi t0, t0, 4
	RV_OP_STOREREG t0, __z_arch_esf_t_mepc_OFFSET(sp)

#ifdef CONFIG_IRQ_OFFLOAD
	/*
	 * Determine if the system call is the result of an IRQ offloading.
	 * Done by checking if _offload_routine is not pointing to NULL.
	 * If NULL, jump to reschedule to perform a context-switch, otherwise,
	 * jump to is_interrupt to handle the IRQ offload.
	 */
	la t0, _offload_routine
	RV_OP_LOADREG t1, 0x00(t0)
	bnez t1, is_interrupt
#endif

	/*
	 * Go to reschedule to handle context-switch
	 */
	j reschedule

is_interrupt:
	/*
	 * Save current thread stack pointer and switch
	 * stack pointer to interrupt stack.
	 */

	/* Save thread stack pointer to temp register t0 */
	addi t0, sp, 0

	/* Switch to interrupt stack */
	la t2, _kernel
	RV_OP_LOADREG sp, _kernel_offset_to_irq_stack(t2)

	/*
	 * Save thread stack pointer on interrupt stack
	 * In RISC-V, stack pointer needs to be 16-byte aligned
	 */
	addi sp, sp, -16
	RV_OP_STOREREG t0, 0x00(sp)

on_irq_stack:
	/* Increment _kernel.nested variable */
	lw t3, _kernel_offset_to_nested(t2)
	addi t3, t3, 1
	sw t3, _kernel_offset_to_nested(t2)

#ifdef CONFIG_IRQ_OFFLOAD
	/*
	 * If we are here due to a system call, t1 register should != 0.
	 * In this case, perform IRQ offloading, otherwise jump to call_irq
	 */
	beqz t1, call_irq

	/*
	 * Call z_irq_do_offload to handle IRQ offloading.
	 * Set return address to on_thread_stack in order to jump there
	 * upon returning from z_irq_do_offload
	 */
	la ra, on_thread_stack
	tail z_irq_do_offload

call_irq:
#endif /* CONFIG_IRQ_OFFLOAD */
#ifdef CONFIG_TRACING
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
	RV_OP_LOADREG a0, 0x00(t0)

	/* Load ISR function address in register t1 */
	RV_OP_LOADREG t1, RV_REGSIZE(t0)

#ifdef CONFIG_EXECUTION_BENCHMARKING
	addi sp, sp, -16
	RV_OP_STOREREG a0, 0x00(sp)
	RV_OP_STOREREG t1, RV_REGSIZE(sp)
	call read_timer_end_of_isr
	RV_OP_LOADREG t1, RV_REGSIZE(sp)
	RV_OP_LOADREG a0, 0x00(sp)
	addi sp, sp, 16
#endif
	/* Call ISR function */
	jalr ra, t1

on_thread_stack:
	/* Get reference to _kernel */
	la t1, _kernel

	/* Decrement _kernel.nested variable */
	lw t2, _kernel_offset_to_nested(t1)
	addi t2, t2, -1
	sw t2, _kernel_offset_to_nested(t1)

	/* Restore thread stack pointer */
	RV_OP_LOADREG t0, 0x00(sp)
	addi sp, t0, 0

#ifdef CONFIG_STACK_SENTINEL
	call z_check_stack_sentinel
	la t1, _kernel
#endif

#ifdef CONFIG_PREEMPT_ENABLED
	/*
	 * Check if we need to perform a reschedule
	 */

	/* Get pointer to _kernel.current */
	RV_OP_LOADREG t2, _kernel_offset_to_current(t1)

	/*
	 * Check if next thread to schedule is current thread.
	 * If yes do not perform a reschedule
	 */
	RV_OP_LOADREG t3, _kernel_offset_to_ready_q_cache(t1)
	beq t3, t2, no_reschedule
#else
	j no_reschedule
#endif /* CONFIG_PREEMPT_ENABLED */

reschedule:
#if CONFIG_TRACING
	call sys_trace_thread_switched_in
#endif
	/* Get reference to _kernel */
	la t0, _kernel

	/* Get pointer to _kernel.current */
	RV_OP_LOADREG t1, _kernel_offset_to_current(t0)

	/*
	 * Save callee-saved registers of current thread
	 * prior to handle context-switching
	 */
	RV_OP_STOREREG s0, _thread_offset_to_s0(t1)
	RV_OP_STOREREG s1, _thread_offset_to_s1(t1)
	RV_OP_STOREREG s2, _thread_offset_to_s2(t1)
	RV_OP_STOREREG s3, _thread_offset_to_s3(t1)
	RV_OP_STOREREG s4, _thread_offset_to_s4(t1)
	RV_OP_STOREREG s5, _thread_offset_to_s5(t1)
	RV_OP_STOREREG s6, _thread_offset_to_s6(t1)
	RV_OP_STOREREG s7, _thread_offset_to_s7(t1)
	RV_OP_STOREREG s8, _thread_offset_to_s8(t1)
	RV_OP_STOREREG s9, _thread_offset_to_s9(t1)
	RV_OP_STOREREG s10, _thread_offset_to_s10(t1)
	RV_OP_STOREREG s11, _thread_offset_to_s11(t1)

	/*
	 * Save stack pointer of current thread and set the default return value
	 * of z_swap to _k_neg_eagain for the thread.
	 */
	RV_OP_STOREREG sp, _thread_offset_to_sp(t1)
	la t2, _k_neg_eagain
	lw t3, 0x00(t2)
	sw t3, _thread_offset_to_swap_return_value(t1)

	/* Get next thread to schedule. */
	RV_OP_LOADREG t1, _kernel_offset_to_ready_q_cache(t0)

	/*
	 * Set _kernel.current to new thread loaded in t1
	 */
	RV_OP_STOREREG t1, _kernel_offset_to_current(t0)

	/* Switch to new thread stack */
	RV_OP_LOADREG sp, _thread_offset_to_sp(t1)

	/* Restore callee-saved registers of new thread */
	RV_OP_LOADREG s0, _thread_offset_to_s0(t1)
	RV_OP_LOADREG s1, _thread_offset_to_s1(t1)
	RV_OP_LOADREG s2, _thread_offset_to_s2(t1)
	RV_OP_LOADREG s3, _thread_offset_to_s3(t1)
	RV_OP_LOADREG s4, _thread_offset_to_s4(t1)
	RV_OP_LOADREG s5, _thread_offset_to_s5(t1)
	RV_OP_LOADREG s6, _thread_offset_to_s6(t1)
	RV_OP_LOADREG s7, _thread_offset_to_s7(t1)
	RV_OP_LOADREG s8, _thread_offset_to_s8(t1)
	RV_OP_LOADREG s9, _thread_offset_to_s9(t1)
	RV_OP_LOADREG s10, _thread_offset_to_s10(t1)
	RV_OP_LOADREG s11, _thread_offset_to_s11(t1)

#ifdef CONFIG_EXECUTION_BENCHMARKING
	addi sp, sp, -__z_arch_esf_t_SIZEOF

	RV_OP_STOREREG ra, __z_arch_esf_t_ra_OFFSET(sp)
	RV_OP_STOREREG gp, __z_arch_esf_t_gp_OFFSET(sp)
	RV_OP_STOREREG tp, __z_arch_esf_t_tp_OFFSET(sp)
	RV_OP_STOREREG t0, __z_arch_esf_t_t0_OFFSET(sp)
	RV_OP_STOREREG t1, __z_arch_esf_t_t1_OFFSET(sp)
	RV_OP_STOREREG t2, __z_arch_esf_t_t2_OFFSET(sp)
	RV_OP_STOREREG t3, __z_arch_esf_t_t3_OFFSET(sp)
	RV_OP_STOREREG t4, __z_arch_esf_t_t4_OFFSET(sp)
	RV_OP_STOREREG t5, __z_arch_esf_t_t5_OFFSET(sp)
	RV_OP_STOREREG t6, __z_arch_esf_t_t6_OFFSET(sp)
	RV_OP_STOREREG a0, __z_arch_esf_t_a0_OFFSET(sp)
	RV_OP_STOREREG a1, __z_arch_esf_t_a1_OFFSET(sp)
	RV_OP_STOREREG a2, __z_arch_esf_t_a2_OFFSET(sp)
	RV_OP_STOREREG a3, __z_arch_esf_t_a3_OFFSET(sp)
	RV_OP_STOREREG a4, __z_arch_esf_t_a4_OFFSET(sp)
	RV_OP_STOREREG a5, __z_arch_esf_t_a5_OFFSET(sp)
	RV_OP_STOREREG a6, __z_arch_esf_t_a6_OFFSET(sp)
	RV_OP_STOREREG a7, __z_arch_esf_t_a7_OFFSET(sp)

	call read_timer_end_of_swap

	RV_OP_LOADREG ra, __z_arch_esf_t_ra_OFFSET(sp)
	RV_OP_LOADREG gp, __z_arch_esf_t_gp_OFFSET(sp)
	RV_OP_LOADREG tp, __z_arch_esf_t_tp_OFFSET(sp)
	RV_OP_LOADREG t0, __z_arch_esf_t_t0_OFFSET(sp)
	RV_OP_LOADREG t1, __z_arch_esf_t_t1_OFFSET(sp)
	RV_OP_LOADREG t2, __z_arch_esf_t_t2_OFFSET(sp)
	RV_OP_LOADREG t3, __z_arch_esf_t_t3_OFFSET(sp)
	RV_OP_LOADREG t4, __z_arch_esf_t_t4_OFFSET(sp)
	RV_OP_LOADREG t5, __z_arch_esf_t_t5_OFFSET(sp)
	RV_OP_LOADREG t6, __z_arch_esf_t_t6_OFFSET(sp)
	RV_OP_LOADREG a0, __z_arch_esf_t_a0_OFFSET(sp)
	RV_OP_LOADREG a1, __z_arch_esf_t_a1_OFFSET(sp)
	RV_OP_LOADREG a2, __z_arch_esf_t_a2_OFFSET(sp)
	RV_OP_LOADREG a3, __z_arch_esf_t_a3_OFFSET(sp)
	RV_OP_LOADREG a4, __z_arch_esf_t_a4_OFFSET(sp)
	RV_OP_LOADREG a5, __z_arch_esf_t_a5_OFFSET(sp)
	RV_OP_LOADREG a6, __z_arch_esf_t_a6_OFFSET(sp)
	RV_OP_LOADREG a7, __z_arch_esf_t_a7_OFFSET(sp)

	/* Release stack space */
	addi sp, sp, __z_arch_esf_t_SIZEOF
#endif

no_reschedule:
#ifdef CONFIG_RISCV_SOC_CONTEXT_SAVE
	/* Restore context at SOC level */
	addi a0, sp, __z_arch_esf_t_soc_context_OFFSET
	jal ra, __soc_restore_context
#endif /* CONFIG_RISCV_SOC_CONTEXT_SAVE */

	/* Restore MEPC register */
	RV_OP_LOADREG t0, __z_arch_esf_t_mepc_OFFSET(sp)
	csrw mepc, t0

	/* Restore SOC-specific MSTATUS register */
	RV_OP_LOADREG t0, __z_arch_esf_t_mstatus_OFFSET(sp)
	csrw SOC_MSTATUS_REG, t0

	/* Restore caller-saved registers from thread stack */
	RV_OP_LOADREG ra, __z_arch_esf_t_ra_OFFSET(sp)
	RV_OP_LOADREG gp, __z_arch_esf_t_gp_OFFSET(sp)
	RV_OP_LOADREG tp, __z_arch_esf_t_tp_OFFSET(sp)
	RV_OP_LOADREG t0, __z_arch_esf_t_t0_OFFSET(sp)
	RV_OP_LOADREG t1, __z_arch_esf_t_t1_OFFSET(sp)
	RV_OP_LOADREG t2, __z_arch_esf_t_t2_OFFSET(sp)
	RV_OP_LOADREG t3, __z_arch_esf_t_t3_OFFSET(sp)
	RV_OP_LOADREG t4, __z_arch_esf_t_t4_OFFSET(sp)
	RV_OP_LOADREG t5, __z_arch_esf_t_t5_OFFSET(sp)
	RV_OP_LOADREG t6, __z_arch_esf_t_t6_OFFSET(sp)
	RV_OP_LOADREG a0, __z_arch_esf_t_a0_OFFSET(sp)
	RV_OP_LOADREG a1, __z_arch_esf_t_a1_OFFSET(sp)
	RV_OP_LOADREG a2, __z_arch_esf_t_a2_OFFSET(sp)
	RV_OP_LOADREG a3, __z_arch_esf_t_a3_OFFSET(sp)
	RV_OP_LOADREG a4, __z_arch_esf_t_a4_OFFSET(sp)
	RV_OP_LOADREG a5, __z_arch_esf_t_a5_OFFSET(sp)
	RV_OP_LOADREG a6, __z_arch_esf_t_a6_OFFSET(sp)
	RV_OP_LOADREG a7, __z_arch_esf_t_a7_OFFSET(sp)

	/* Release stack space */
	addi sp, sp, __z_arch_esf_t_SIZEOF

	/* Call SOC_ERET to exit ISR */
	SOC_ERET
