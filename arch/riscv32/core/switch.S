/*
 * Copyright (c) 2018 SiFive Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <irq.h>
#include <kernel_structs.h>
#include <offsets_short.h>

/* exports */
GTEXT(_riscv_switch)

/* Use ABI name of registers for the sake of simplicity */

/*
 * void _riscv_switch(void *switch_to, void **switch_from)
 */
SECTION_FUNC(exception.other, _riscv_switch)
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

	/* Set MEPC register to return from _riscv_switch */
	la t0, leave_riscv_switch
	sw t0, __NANO_ESF_mepc_OFFSET(sp)

	/*
	 * Emulate the behavior of the MSTATUS register when an interrupt is taken
	 * by setting the MPIE bit if the MIE bit is set and clearing the MIE bit,
	 * then setting MPP to the previous privilege mode (machine mode)
	 */
	csrr t0, SOC_MSTATUS_REG

	/* Extract the interrupt enable bit */
	li t1, SOC_MSTATUS_IEN
	and t1, t0, t1

	/* Set the MPIE bit if the IEN bit was set */
	beqz t1, switch_no_set_mpie
	li t1, SOC_MSTATUS_MPIE
	or t0, t0, t1

switch_no_set_mpie:
	/* Clear the MIE bit */
	li t1, SOC_MSTATUS_IEN
	not t1, t1
	and t0, t0, t1

	/* Set the previous privilege mode to M-mode */
	li t1, SOC_MSTATUS_MPP_M_MODE
	or t0, t0, t1

	/* Save the new value of MSTATUS onto the stack frame */
	sw t0, __NANO_ESF_mstatus_OFFSET(sp)

	/* Store stack pointer in switch_from */
	sw sp, 0(a1)

	/* Load stack pointer from switch_to */
	mv sp, a0

	/* Restore MEPC register */
	lw t0, __NANO_ESF_mepc_OFFSET(sp)
	csrw mepc, t0

	/* Restore SOC-specific MSTATUS register */
	lw t0, __NANO_ESF_mstatus_OFFSET(sp)
	csrw SOC_MSTATUS_REG, t0

	/* Load caller-saved registers */
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

	/* Free allocated stack space */
	addi sp, sp, __NANO_ESF_SIZEOF

	SOC_ERET

leave_riscv_switch:
	ret

