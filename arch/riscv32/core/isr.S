/*
 * Copyright (c) 2016 Jean-Paul Etienne <fractalclone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <toolchain.h>
#include <sections.h>
#include <kernel_structs.h>
#include <offsets_short.h>

/* imports */
GDATA(_sw_isr_table)
GTEXT(__soc_save_context)
GTEXT(__soc_restore_context)
GTEXT(__soc_is_irq)
GTEXT(__soc_handle_irq)
GTEXT(_Fault)

GTEXT(_k_neg_eagain)
GTEXT(_is_next_thread_current)
GTEXT(_get_next_ready_thread)

#ifdef CONFIG_KERNEL_EVENT_LOGGER_CONTEXT_SWITCH
GTEXT(_sys_k_event_logger_context_switch)
#endif

#ifdef CONFIG_KERNEL_EVENT_LOGGER_SLEEP
GTEXT(_sys_k_event_logger_exit_sleep)
#endif

#ifdef CONFIG_KERNEL_EVENT_LOGGER_INTERRUPT
GTEXT(_sys_k_event_logger_interrupt)
#endif

#ifdef CONFIG_IRQ_OFFLOAD
GTEXT(_offload_routine)
#endif

#ifdef CONFIG_TIMESLICING
GTEXT(_update_time_slice_before_swap)
#endif

/* exports */
GTEXT(__irq_wrapper)

/* use ABI name of registers for the sake of simplicity */

/*
 * ISR is handled at both ARCH and SOC levels.
 * At the ARCH level, ISR handles basic context saving/restore of registers
 * onto/from the thread stack and calls corresponding IRQ function registered
 * at driver level.

 * At SOC level, ISR handles saving/restoring of SOC-specific registers
 * onto/from the thread stack (handled via __soc_save_context and
 * __soc_restore_context functions). SOC level save/restore context
 * is accounted for only if CONFIG_RISCV_SOC_CONTEXT_SAVE variable is set
 *
 * Moreover, given that RISC-V architecture does not provide a clear ISA
 * specification about interrupt handling, each RISC-V SOC handles it in
 * its own way. Hence, the generic RISC-V ISR handler expects the following
 * functions to be provided at the SOC level:
 * __soc_is_irq: to check if the exception is the result of an interrupt or not.
 * __soc_handle_irq: handle pending IRQ at SOC level (ex: clear pending IRQ in
 * SOC-specific IRQ register)
 */

/*
 * Handler called upon each exception/interrupt/fault
 * In this architecture, system call (ECALL) is used to perform context
 * switching or IRQ offloading (when enabled).
 */
SECTION_FUNC(exception.entry, __irq_wrapper)
	/* Allocate space on thread stack to save registers */
	addi sp, sp, -__NANO_ESF_SIZEOF

	/*
	 * Save caller-saved registers on current thread stack.
	 * NOTE: need to be updated to account for floating-point registers
	 * floating-point registers should be accounted for when corresponding
	 * config variable is set
	 */
	sw ra, __NANO_ESF_ra_OFFSET(sp)
	sw gp, __NANO_ESF_gp_OFFSET(sp)
	sw tp, __NANO_ESF_tp_OFFSET(sp)
	sw t0, __NANO_ESF_t0_OFFSET(sp)
	sw t1, __NANO_ESF_t1_OFFSET(sp)
	sw t2, __NANO_ESF_t2_OFFSET(sp)
	sw t3, __NANO_ESF_t3_OFFSET(sp)
	sw t4, __NANO_ESF_t4_OFFSET(sp)
	sw t5, __NANO_ESF_t5_OFFSET(sp)
	sw t6, __NANO_ESF_t6_OFFSET(sp)
	sw a0, __NANO_ESF_a0_OFFSET(sp)
	sw a1, __NANO_ESF_a1_OFFSET(sp)
	sw a2, __NANO_ESF_a2_OFFSET(sp)
	sw a3, __NANO_ESF_a3_OFFSET(sp)
	sw a4, __NANO_ESF_a4_OFFSET(sp)
	sw a5, __NANO_ESF_a5_OFFSET(sp)
	sw a6, __NANO_ESF_a6_OFFSET(sp)
	sw a7, __NANO_ESF_a7_OFFSET(sp)

	/* Save MEPC register */
	csrr t0, mepc
	sw t0, __NANO_ESF_mepc_OFFSET(sp)

	/* Save SOC-specific MSTATUS register */
	csrr t0, SOC_MSTATUS_REG
	sw t0, __NANO_ESF_mstatus_OFFSET(sp)

#ifdef CONFIG_RISCV_SOC_CONTEXT_SAVE
	/* Handle context saving at SOC level. */
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
	addi t1, x0, 0
	bnez a0, is_interrupt

	/*
	 * If exception is not an interrupt, MEPC will contain
	 * the instruction address, which has caused the exception.
	 * Increment saved MEPC by 4 to prevent running into the
	 * exception again, upon exiting the ISR.
	 */
	lw t0, __NANO_ESF_mepc_OFFSET(sp)
	addi t0, t0, 4
	sw t0, __NANO_ESF_mepc_OFFSET(sp)

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
#ifdef CONFIG_IRQ_OFFLOAD
	/* If not system call, jump to is_fault */
	bne t0, t1, is_fault

	/*
	 * Determine if the system call is the result of an IRQ offloading.
	 * Done by checking if _offload_routine is not pointing to NULL.
	 * If NULL, jump to reschedule to perform a context-switch, otherwise,
	 * jump to is_interrupt to handle the IRQ offload.
	 */
	la t0, _offload_routine
	lw t1, 0x00(t0)
	beqz t1, reschedule
	bnez t1, is_interrupt

is_fault:
#else
	/*
	 * Go to reschedule to handle context-switch if system call,
	 * otherwise call _Fault to handle exception
	 */
	beq t0, t1, reschedule
#endif

	/*
	 * Call _Fault to handle exception.
	 * Stack pointer is pointing to a NANO_ESF structure, pass it
	 * to _Fault (via register a0).
	 * If _Fault shall return, set return address to no_reschedule
	 * to restore stack.
	 */
	addi a0, sp, 0
	la ra, no_reschedule
	tail _Fault

is_interrupt:
	/*
	 * Save current thread stack pointer and switch
	 * stack pointer to interrupt stack.
	 */

	/* Save thread stack pointer to temp register t0 */
	addi t0, sp, 0

	/* Switch to interrupt stack */
	la t2, _kernel
	lw sp, _kernel_offset_to_irq_stack(t2)

	/*
	 * Save thread stack pointer on interrupt stack
	 * In RISC-V, stack pointer needs to be 16-byte aligned
	 */
	addi sp, sp, -16
	sw t0, 0x00(sp)

on_irq_stack:
	/* Increment _kernel.nested variable */
	lw t3, _kernel_offset_to_nested(t2)
	addi t3, t3, 1
	sw t3, _kernel_offset_to_nested(t2)

	/*
	 * If we are here due to a system call, t1 register should != 0.
	 * In this case, perform IRQ offloading, otherwise jump to call_irq
	 */
	beqz t1, call_irq

	/*
	 * Call _irq_do_offload to handle IRQ offloading.
	 * Set return address to on_thread_stack in order to jump there
	 * upon returning from _irq_do_offload
	 */
	la ra, on_thread_stack
	tail _irq_do_offload

call_irq:
#ifdef CONFIG_KERNEL_EVENT_LOGGER_SLEEP
	call _sys_k_event_logger_exit_sleep
#endif

#ifdef CONFIG_KERNEL_EVENT_LOGGER_INTERRUPT
	call _sys_k_event_logger_interrupt
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
	 * (table is 8-bytes wide, we should shift index by 3)
	 */
	la t0, _sw_isr_table
	slli a0, a0, 3
	add t0, t0, a0

	/* Load argument in a0 register */
	lw a0, 0x00(t0)

	/* Load ISR function address in register t1 */
	lw t1, 0x04(t0)

	/* Call ISR function */
	jalr ra, t1

on_thread_stack:
	/* Get reference to _kernel */
	la t1, _kernel

	/* Decrement _kernel.nested variable */
	lw t2, _kernel_offset_to_nested(t1)
	addi t2, t2, -1
	sw t2, _kernel_offset_to_nested(t1)

#ifdef CONFIG_STACK_SENTINEL
	call _check_stack_sentinel
	la t1, _kernel
#endif

	/* Restore thread stack pointer */
	lw t0, 0x00(sp)
	addi sp, t0, 0

#ifdef CONFIG_PREEMPT_ENABLED
	/*
	 * Check if we need to perform a reschedule
	 */

	/* Get pointer to _kernel.current */
	lw t2, _kernel_offset_to_current(t1)

	/*
	 * If non-preemptible thread, do not schedule
	 * (see explanation of preempt field in kernel_structs.h
	 */
	lhu t3, _thread_offset_to_preempt(t2)
	li t4, _NON_PREEMPT_THRESHOLD
	bgeu t3, t4, no_reschedule

	/*
	 * Check if next thread to schedule is current thread.
	 * If yes do not perform a reschedule
	 */
	lw t3, _kernel_offset_to_ready_q_cache(t1)
	beq t3, t2, no_reschedule
#else
	j no_reschedule
#endif /* CONFIG_PREEMPT_ENABLED */

reschedule:
#if CONFIG_TIMESLICING
	call _update_time_slice_before_swap
#endif
#if CONFIG_KERNEL_EVENT_LOGGER_CONTEXT_SWITCH
	call _sys_k_event_logger_context_switch
#endif /* CONFIG_KERNEL_EVENT_LOGGER_CONTEXT_SWITCH */
#ifdef CONFIG_STACK_SENTINEL
	call _check_stack_sentinel
#endif
	/* Get reference to _kernel */
	la t0, _kernel

	/* Get pointer to _kernel.current */
	lw t1, _kernel_offset_to_current(t0)

	/*
	 * Save callee-saved registers of current thread
	 * prior to handle context-switching
	 */
	sw s0, _thread_offset_to_s0(t1)
	sw s1, _thread_offset_to_s1(t1)
	sw s2, _thread_offset_to_s2(t1)
	sw s3, _thread_offset_to_s3(t1)
	sw s4, _thread_offset_to_s4(t1)
	sw s5, _thread_offset_to_s5(t1)
	sw s6, _thread_offset_to_s6(t1)
	sw s7, _thread_offset_to_s7(t1)
	sw s8, _thread_offset_to_s8(t1)
	sw s9, _thread_offset_to_s9(t1)
	sw s10, _thread_offset_to_s10(t1)
	sw s11, _thread_offset_to_s11(t1)

	/*
	 * Save stack pointer of current thread and set the default return value
	 * of _Swap to _k_neg_eagain for the thread.
	 */
	sw sp, _thread_offset_to_sp(t1)
	la t2, _k_neg_eagain
	lw t3, 0x00(t2)
	sw t3, _thread_offset_to_swap_return_value(t1)

	/* Get next thread to schedule. */
	lw t1, _kernel_offset_to_ready_q_cache(t0)

	/*
	 * Set _kernel.current to new thread loaded in t1
	 */
	sw t1, _kernel_offset_to_current(t0)

	/* Switch to new thread stack */
	lw sp, _thread_offset_to_sp(t1)

	/* Restore callee-saved registers of new thread */
	lw s0, _thread_offset_to_s0(t1)
	lw s1, _thread_offset_to_s1(t1)
	lw s2, _thread_offset_to_s2(t1)
	lw s3, _thread_offset_to_s3(t1)
	lw s4, _thread_offset_to_s4(t1)
	lw s5, _thread_offset_to_s5(t1)
	lw s6, _thread_offset_to_s6(t1)
	lw s7, _thread_offset_to_s7(t1)
	lw s8, _thread_offset_to_s8(t1)
	lw s9, _thread_offset_to_s9(t1)
	lw s10, _thread_offset_to_s10(t1)
	lw s11, _thread_offset_to_s11(t1)

no_reschedule:
#ifdef CONFIG_RISCV_SOC_CONTEXT_SAVE
	/* Restore context at SOC level */
	jal ra, __soc_restore_context
#endif /* CONFIG_RISCV_SOC_CONTEXT_SAVE */
	/* Restore caller-saved registers from thread stack */
	lw ra, __NANO_ESF_ra_OFFSET(sp)
	lw gp, __NANO_ESF_gp_OFFSET(sp)
	lw tp, __NANO_ESF_tp_OFFSET(sp)
	lw t0, __NANO_ESF_t0_OFFSET(sp)
	lw t1, __NANO_ESF_t1_OFFSET(sp)
	lw t2, __NANO_ESF_t2_OFFSET(sp)
	lw t3, __NANO_ESF_t3_OFFSET(sp)
	lw t4, __NANO_ESF_t4_OFFSET(sp)
	lw t5, __NANO_ESF_t5_OFFSET(sp)
	lw t6, __NANO_ESF_t6_OFFSET(sp)
	lw a0, __NANO_ESF_a0_OFFSET(sp)
	lw a1, __NANO_ESF_a1_OFFSET(sp)
	lw a2, __NANO_ESF_a2_OFFSET(sp)
	lw a3, __NANO_ESF_a3_OFFSET(sp)
	lw a4, __NANO_ESF_a4_OFFSET(sp)
	lw a5, __NANO_ESF_a5_OFFSET(sp)
	lw a6, __NANO_ESF_a6_OFFSET(sp)
	lw a7, __NANO_ESF_a7_OFFSET(sp)

	/* Restore MEPC register */
	lw t0, __NANO_ESF_mepc_OFFSET(sp)
	csrw mepc, t0

	/* Restore SOC-specific MSTATUS register */
	lw t0, __NANO_ESF_mstatus_OFFSET(sp)
	csrw SOC_MSTATUS_REG, t0

	/* Release stack space */
	addi sp, sp, __NANO_ESF_SIZEOF

	/* Call SOC_ERET to exit ISR */
	SOC_ERET
