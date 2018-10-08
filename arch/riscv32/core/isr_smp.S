/*
 * Copyright (c) 2018 SiFive Inc.
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
GTEXT(_get_next_switch_handle)
GTEXT(_Fault)

#ifdef CONFIG_RISCV_SOC_CONTEXT_SAVE
GTEXT(__soc_save_context)
GTEXT(__soc_restore_context)
#endif

#ifdef CONFIG_TRACING
GTEXT(z_sys_trace_thread_switched_in)
GTEXT(z_sys_trace_isr_enter)
#endif

#ifdef CONFIG_IRQ_OFFLOAD
GTEXT(_offload_routine)
#endif

#ifdef CONFIG_TIMESLICING
GTEXT(_update_time_slice_before_swap)
#endif

/* exports */
GTEXT(__irq_wrapper)

SECTION_FUNC(exception.entry, __irq_wrapper)
	/* Allocate space on thread stack to save registers */
	addi sp, sp, -__NANO_ESF_SIZEOF

	/*
	 * Save caller and callee-saved registers onto the stack
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
	sw s0, __NANO_ESF_s0_OFFSET(sp)
	sw s1, __NANO_ESF_s1_OFFSET(sp)
	sw s2, __NANO_ESF_s2_OFFSET(sp)
	sw s3, __NANO_ESF_s3_OFFSET(sp)
	sw s4, __NANO_ESF_s4_OFFSET(sp)
	sw s5, __NANO_ESF_s5_OFFSET(sp)
	sw s6, __NANO_ESF_s6_OFFSET(sp)
	sw s7, __NANO_ESF_s7_OFFSET(sp)
	sw s8, __NANO_ESF_s8_OFFSET(sp)
	sw s9, __NANO_ESF_s9_OFFSET(sp)
	sw s10, __NANO_ESF_s10_OFFSET(sp)
	sw s11, __NANO_ESF_s11_OFFSET(sp)

#ifdef CONFIG_EXECUTION_BENCHMARKING
	call read_timer_start_of_isr
#endif
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

switch_to_interrupt_stack:
	/* Get cpu context */
	csrr t0, mscratch

	/* Increment nested interrupt count */
	lw t1, _cpu_offset_to_nested(t0)
	addi t2, t1, 1
	sw t2, _cpu_offset_to_nested(t0)

	/* Save thread stack pointer to temp register t2 */
	mv t2, sp

	/* If the nested count was nonzero, we're already on the
	 * interrupt stack */
	bnez t1, on_interrupt_stack

	/* Switch to interrupt stack */
	lw sp, _cpu_offset_to_irq_stack(t0)

on_interrupt_stack:
	/*
	 * Save thread stack pointer on interrupt stack
	 * In RISC-V, stack pointer needs to be 16-byte aligned
	 */
	addi sp, sp, -16
	sw t2, 0x00(sp)

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
	 * If mcause == SOC_MCAUSE_ECALL_EXP, handle system call,
	 * otherwise handle fault
	 */
	li t1, SOC_MCAUSE_ECALL_EXP
	beq t0, t1, is_syscall

	/*
	 * Call _Fault to handle exception.
	 * Load the stored stack frame off of the interrupt stack and pass it
	 * to _Fault (via register a0).
	 *
	 * If _Fault returns, set return address to leave_interrupt_stack
	 * to restore stack.
	 */
	lw a0, 0x00(sp)
	la ra, leave_interrupt_stack
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
	lw t0, __NANO_ESF_mepc_OFFSET(sp)
	addi t0, t0, 4
	sw t0, __NANO_ESF_mepc_OFFSET(sp)

	j leave_interrupt_stack

is_interrupt:
#ifdef CONFIG_TRACING
	call z_sys_trace_isr_enter
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
	 * Load corresponding registered function in _sw_isr_table.
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

#ifdef CONFIG_EXECUTION_BENCHMARKING
	call read_timer_end_of_isr
#endif

leave_interrupt_stack:
	/* Get cpu context */
	csrr t0, mscratch

	/* Decrement nested count */
	lw t1, _cpu_offset_to_nested(t0)
	addi t1, t1, -1
	sw t1, _cpu_offset_to_nested(t0)

	/* Pop saved thread stack pointer off of the interrupt stack and into
	 * the argument to _get_next_switch_handle */
	lw a0, 0x00(sp)
	addi sp, sp, 16

	/* Ask the scheduler for the next thread to run */
	call _get_next_switch_handle

	/* Switch to the new stack */
	mv sp, a0

restore_next_thread:
#ifdef CONFIG_RISCV_SOC_CONTEXT_SAVE
	/* Handle context saving at SOC level. */
	jal ra, __soc_save_context
#endif /* CONFIG_RISCV_SOC_CONTEXT_SAVE */

	/* Restore MEPC register */
	lw t0, __NANO_ESF_mepc_OFFSET(sp)
	csrw mepc, t0

	/* Restore SOC-specific MSTATUS register */
	lw t0, __NANO_ESF_mstatus_OFFSET(sp)
	csrw SOC_MSTATUS_REG, t0

	/* Load saved registers */
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
	lw s0, __NANO_ESF_s0_OFFSET(sp)
	lw s1, __NANO_ESF_s1_OFFSET(sp)
	lw s2, __NANO_ESF_s2_OFFSET(sp)
	lw s3, __NANO_ESF_s3_OFFSET(sp)
	lw s4, __NANO_ESF_s4_OFFSET(sp)
	lw s5, __NANO_ESF_s5_OFFSET(sp)
	lw s6, __NANO_ESF_s6_OFFSET(sp)
	lw s7, __NANO_ESF_s7_OFFSET(sp)
	lw s8, __NANO_ESF_s8_OFFSET(sp)
	lw s9, __NANO_ESF_s9_OFFSET(sp)
	lw s10, __NANO_ESF_s10_OFFSET(sp)
	lw s11, __NANO_ESF_s11_OFFSET(sp)

	/* Free stack space */
	addi sp, sp, __NANO_ESF_SIZEOF

	/* Return from interrupt */
	SOC_ERET

