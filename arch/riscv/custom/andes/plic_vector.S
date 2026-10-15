/*
 * Copyright (c) 2026 Andes Technology Corporation
 * Copyright (c) 2026 Analog Devices, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/riscv/irq.h>
#include <offsets_short.h>
#include <zephyr/devicetree.h>
#include "csr_irq.inc"

/* exports */
GTEXT(_andes_plic_isr_entry)
GTEXT(__soc_enable_isr_nesting)
GTEXT(__soc_disable_isr_nesting)
/* imports */
GTEXT(_isr_wrapper)
GDATA(plic_mcause)

#define MIE_MTIE BIT(7)
#define MIE_MSIE BIT(3)

#define PLIC_THRESHOLD_ADDR (DT_REG_ADDR(DT_INST(0, sifive_plic_1_0_0)) + 0x200000)
#define PLIC_MAX_PRIO (DT_PROP(DT_INST(0, sifive_plic_1_0_0), riscv_max_priority))

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
 * Entry point for PLIC-sourced interrupts (mtvec[i], i >= 1). Vectored
 * PLIC hardware claims the interrupt and leaves the source ID in mcause
 * with no INTERRUPT bit set. Swap it into plic_mcause, then fake mcause
 * as a standard machine external interrupt so the architecture entry
 * point dispatches through plic_irq_handler as usual.
 */
SECTION_FUNC(exception.entry, _andes_plic_isr_entry)
#ifdef CONFIG_USERSPACE
	/* retrieve address of _current_cpu preserving s0 */
	csrrw s0, mscratch, s0

	/* preserve t0 and t1 temporarily */
	sr t0, _curr_cpu_arch_user_exc_tmp0(s0)
	sr t1, _curr_cpu_arch_user_exc_tmp1(s0)
#else
	/* The esf is allocated later, but it is safe to borrow the members
	 * holding these registers already.
	 */
	sr t0, (-__struct_arch_esf_SIZEOF + __struct_arch_esf_t0_OFFSET)(sp)
	sr t1, (-__struct_arch_esf_SIZEOF + __struct_arch_esf_t1_OFFSET)(sp)
#endif /* CONFIG_USERSPACE */

	/* Swap the source ID into plic_mcause. */
	.option push
	.option norelax
	csrr t1, mcause
	la t0, plic_mcause
	sw t1, 0(t0)
	.option pop

	/* Fake mcause as a machine external interrupt so _isr_wrapper
	 * dispatches it normally.
	 */
	li t1, RISCV_MCAUSE_IRQ_BIT | RISCV_IRQ_MEXT
	csrw mcause, t1

#ifdef CONFIG_USERSPACE
	/* retrieve original t0/t1 values */
	lr t0, _curr_cpu_arch_user_exc_tmp0(s0)
	lr t1, _curr_cpu_arch_user_exc_tmp1(s0)

	/* retrieve original s0 and restore _current_cpu in mscratch */
	csrrw s0, mscratch, s0
#else
	lr t0, (-__struct_arch_esf_SIZEOF + __struct_arch_esf_t0_OFFSET)(sp)
	lr t1, (-__struct_arch_esf_SIZEOF + __struct_arch_esf_t1_OFFSET)(sp)
#endif /* CONFIG_USERSPACE */

	/* Jump to the architecture ISR wrapper */
	j _isr_wrapper

#ifdef CONFIG_ZERO_LATENCY_IRQS

SECTION_FUNC(exception.other, __soc_enable_isr_nesting)
	/* Boost PLIC priority to block low priority interrupts */
	li t0, PLIC_THRESHOLD_ADDR
	li t1, PLIC_MAX_PRIO
	addi t1, t1, -CONFIG_ZERO_LATENCY_LEVELS
	sw t1, 0(t0)
	/* Clear mie.MTIE and mie.MSIE */
	li t0, (MIE_MTIE | MIE_MSIE)
	csrc mie, t0
	/* Set mstatus.MIE */
	csrsi mstatus, 0x8
	ret

SECTION_FUNC(exception.other, __soc_disable_isr_nesting)
	/* Disable interrupts, mret will re-enable them */
	csrci mstatus, 0x8
	/* Set mie.MTIE and mie.MSIE */
	li t0, (MIE_MTIE | MIE_MSIE)
	csrs mie, t0
	/* Restore PLIC priority */
	li t0, PLIC_THRESHOLD_ADDR
	sw x0, 0(t0)
	ret

#endif /* CONFIG_ZERO_LATENCY_IRQS */
