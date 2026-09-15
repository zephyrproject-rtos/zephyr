/*
 * Copyright (c) 2026 Andes Technology Corporation
 * Copyright (c) 2026 Analog Devices, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/riscv/irq.h>
#include <zephyr/devicetree.h>
#include "csr_irq.inc"

#define PLIC_MCAUSE_OFFSET (__struct_arch_esf_soc_context_OFFSET + \
	__soc_esf_t_plic_mcause_OFFSET - __struct_arch_esf_SIZEOF)

/* MTIMECMP base address from devicetree (hart 0) */
#define MTIMECMP0  DT_REG_ADDR_BY_NAME(DT_INST(0, riscv_machine_timer), mtimecmp)

/* exports */
GTEXT(rv_exception_handler)
GTEXT(__soc_handle_irq)
GTEXT(_andes_plic_isr_wrapper)
GTEXT(__soc_read_mcause)
/* imports */
GTEXT(_isr_wrapper)
GTEXT(riscv_plic_irq_complete)

/* Used to record that the exception was a trap or local interrupt */
GDATA(_trap_active)

/*
 * Each Andes Vectored PLIC vector table entry is 4 bytes for both RV32 and RV64.
 * On RV64, the upper 32 bits of the ISR address are taken from mtvec[63:32].
 *
 * The table base must be aligned to a 2^log2(2N+2) byte boundary, where N is
 * the number of PLIC interrupt sources. For example, if N is 1023, the minimum
 * alignment requirement is 4096 bytes.
 *
 * Vector table entries:
 * - mtvec[0] is used for exceptions and non-external local interrupts.
 * - mtvec[i] is used for PLIC external interrupt source i.
 */

/*
 * These functions must be called by the SoC's implementations of
 * __soc_save_context and __soc_restore_context
 */
GTEXT(andes_plic_save_context)
GTEXT(andes_plic_restore_context)

SECTION_FUNC(exception.other, andes_plic_save_context)
	/*
	 * Andes Vectored PLIC hardware automatically claims the pending
	 * interrupt and records the source ID in mcause.
	 *
	 * Check if _trap_active is set
	 */
	la t0, _trap_active
	lr t1, 0(t0)
	/* Clear _trap_active */
	sr x0, 0(t0)
	/* If _trap_active was set before, skip mcause handling */
	bnez t1, _soc_exception_done
	/* Check mstatus.MIE. If set, we should load mcause from the stack */
	csrr t1, mstatus
	andi t0, t1, 0x8
	beqz t0, 1f
	lr t0, (PLIC_MCAUSE_OFFSET + __struct_arch_esf_SIZEOF)(sp)
	csrw mcause, t0
1:
	/* Load mcause */
	csrr t0, mcause
	/* We need to store the exception
	 * code in mcause as the interrupt source ID + CONFIG_2ND_LVL_ISR_TBL_OFFSET,
	 * so that the _isr_wrapper picks the correct offset in the SW isr table.
	 */
	addi t0, t0, CONFIG_2ND_LVL_ISR_TBL_OFFSET
	/* Set the INTERRUPT bit in mcause */
	li t1, RISCV_MCAUSE_IRQ_BIT
	or t0, t0, t1
	/* Store to mcause */
	csrw mcause, t0
	/* Save mcause in case this interrupt is preempted */
	sr t0, __soc_esf_t_plic_mcause_OFFSET(a0)
	/* Set mstatus.MIE, in case it is not already enabled */
	csrsi mstatus, 0x8
_soc_exception_done:
	ret

SECTION_FUNC(exception.other, andes_plic_restore_context)
	addi sp, sp, -16
	sr ra, 0(sp)
	/*
	 * Load the saved mcause value. If RISCV_MCAUSE_IRQ_BIT isn't set, then
	 * the request does not require the claim complete protocol
	 */
	lw a0, __soc_esf_t_plic_mcause_OFFSET(a0)
	csrw mcause, a0
	bgez a0, _restore_done
	/* Clear MIE bit prior to running claim complete protocol */
	csrci mstatus, 0x8

	/*
	 * The NCEPLIC100 handles the claim portion of the
	 * claim/complete protocol automatically, but software
	 * must implement the complete side.
	 *
	 * Construct the L2 interrupt value for riscv_plic_irq_complete
	 */
	li t1, CONFIG_RISCV_MCAUSE_EXCEPTION_MASK
	and a0, a0, t1
	/* Remove CONFIG_2ND_LVL_ISR_TBL_OFFSET */
	addi a0, a0, -(CONFIG_2ND_LVL_ISR_TBL_OFFSET - 1)
	slli a0, a0, CONFIG_1ST_LEVEL_INTERRUPT_BITS
	addi a0, a0, DT_IRQN(DT_INST(0, sifive_plic_1_0_0))

	call riscv_plic_irq_complete
	/*
	 * Per andes datasheet, this fence is required in vectored mode
	 * to ensure that the completion message has reached PLIC
	 * before the interrupt handler returns and reenables MIE
	 */
	fence io, io
_restore_done:
	lr ra, 0(sp)
	addi sp, sp, 16
	ret

SECTION_FUNC(exception.other, __soc_handle_irq)
	/* No-op, we handle custom IRQ logic via __soc_read_mcause */
	ret

/*
 * This handler is installed at mtvec[0] for handling
 * exceptions and local interrupts, since they don't require special mcause
 * modification.
*/
SECTION_FUNC(exception.other, rv_exception_handler)
	addi sp, sp, -16
	sr t0, 0x00(sp)
	sr t1, 0x08(sp)
	/* Set trap_active to true */
	la t0, _trap_active
	li t1, 1
	sr t1, 0(t0)
	/*
	 * If interrupt was from timer, push MTIMECMP far into the future.
	 * Otherwise, the interrupt will re-assert the moment we set mstatus.MIE.
	 * This is a bit of a hack, but it should be safe as the timer_isr will
	 * reprogram mtimecmp to a logical value.
	 */
	li t0, 0x80000007
	csrr t1, mcause
	bne t0, t1, 1f
	li t0, MTIMECMP0
	li t1, -1
	sr t1 0(t0)
1:
	lr t0, 0x00(sp)
	lr t1, 0x08(sp)
	addi sp, sp, 16
	/* Jump to _andes_plic_isr_wrapper */
	j _andes_plic_isr_wrapper

/*
 * This handler wraps the _isr_wrapper function, and is used to enable
 * mstatus.MIE as early as possible in order to support nested interrupts
 */
SECTION_FUNC(exception.entry, _andes_plic_isr_wrapper)
	/*
	 * Userspace handling logic should match that in the _isr_wrapper,
	 * we need the same here since we want to save mcause to the
	 * privileged stack
	 */
#ifdef CONFIG_USERSPACE
	/* retrieve address of _current_cpu preserving s0 */
	csrrw s0, mscratch, s0

	/* preserve t0 and t1 temporarily */
	sr t0, _curr_cpu_arch_user_exc_tmp0(s0)
	sr t1, _curr_cpu_arch_user_exc_tmp1(s0)

	/* determine if we come from user space */
	csrr t0, mstatus
	li t1, MSTATUS_MPP
	and t0, t0, t1
	bnez t0, 1f

	/* in user space we were: switch to our privileged stack */
	mv t0, sp
	lr sp, _curr_cpu_arch_user_exc_sp(s0)

	/* Save user stack value. Coming from user space, we know this
	 * can't overflow the privileged stack. The esf will be allocated
	 * later but it is safe to store our saved user sp here. */
	sr t0, (-__struct_arch_esf_SIZEOF + __struct_arch_esf_sp_OFFSET)(sp)
	/* Store mcause */
	csrr t1, mcause
	sr t1, PLIC_MCAUSE_OFFSET(sp)
	/* Restore original user stack pointer */
	mv sp, t0
1:
	/* Store mcause */
	csrr t1, mcause
	sr t1, PLIC_MCAUSE_OFFSET(sp)

	/* retrieve original t0/t1 values */
	lr t0, _curr_cpu_arch_user_exc_tmp0(s0)
	lr t1, _curr_cpu_arch_user_exc_tmp1(s0)

	/* retrieve original s0 and restore _current_cpu in mscratch */
	csrrw s0, mscratch, s0
	/* Preserve t1 */
	csrrw t1, mscratch, t1
#else /* !CONFIG_USERSPACE */
	/*
	 * Before we set mstatus.MIE to enable nested interrupts we need to save
	 * mcause, as it will be overwritten by the hardware if an interrupt fires
	 */
	/* Preserve t1 */
	csrrw t1, mscratch, t1
	csrr t1, mcause
	sr t1, PLIC_MCAUSE_OFFSET(sp)
#endif
	/* Set mstatus.MIE */
	csrsi mstatus, 0x8
	/* Restore t1 */
	csrrw t1, mscratch, t1
	/* Jump to ISR wrapper */
	j _isr_wrapper

SECTION_FUNC(exception.other, __soc_read_mcause)
	/* Check the stack pointer to see if we are on thread or ISR stack */
	mv t1, sp
	lr t0, ___cpu_t_irq_stack_OFFSET(s0)
	addi t0, t0, -16
	bne t0, t1, 1f
	/* We weren't on thread stack. Load thread sp to t1 */
	lr t1, 0(t0)
1:
	/* Pull mcause from the plic_mcause offset, using the thread stack pointer */
	lr a0, (PLIC_MCAUSE_OFFSET + __struct_arch_esf_SIZEOF)(t1)
	ret
