/*
 * Copyright (c) 2026 Analog Devices, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/riscv/irq.h>
#include <zephyr/devicetree.h>

extern void _isr_wrapper(void);

#define PLIC_THRESHOLD_ADDR (DT_REG_ADDR(DT_INST(0, sifive_plic_1_0_0)) + 0x200000)
#define PLIC_MAX_PRIO (DT_PROP(DT_INST(0, sifive_plic_1_0_0), riscv_max_priority))

#define MIE_MTIE BIT(7)
#define MIE_MSIE BIT(3)

/*
 * mtvec[0] handles exceptions and local interrupts, which need no
 * special mcause handling, so route it straight to the architecture
 * ISR entry point.
 */
Z_ISR_DECLARE_DIRECT(0, ISR_FLAG_DIRECT, _isr_wrapper);


#ifdef CONFIG_ZERO_LATENCY_IRQS

unsigned int z_soc_irq_lock(void)
{
	/* Boost the HART threshold to prevent
	 * low priority interrupts from running
	 */
	unsigned int current_threshold = sys_read32(PLIC_THRESHOLD_ADDR);

	sys_write32(PLIC_MAX_PRIO - CONFIG_ZERO_LATENCY_LEVELS, PLIC_THRESHOLD_ADDR);
	__asm__ volatile (
		/* Clear mie.MTIE and mie.MSIE to prevent local interrupts from firing */
		"csrc mie, %0\n"
		/* Set mstatus.MIE to enable high priority interrupts to preempt */
		"csrs mie, %1\n"
		:
		: "r" (MIE_MTIE | MIE_MSIE), "r" (RV_STATUS_IE)
		: "memory");
	return current_threshold;
}

void z_soc_irq_unlock(unsigned int key)
{
	/* Some callers just use RV_STATUS_IE as the key, we should unlock then too */
	if (key != 0 && key != RV_STATUS_IE) {
		return;
	}
	/* Restore the HART threshold to enable all interrupts */
	sys_write32(0, PLIC_THRESHOLD_ADDR);
	__asm__ volatile (
		/* Set mie.MTIE and mie.MSIE to re-enable local interrupts */
		"csrs mie, %0\n"
		/* Make sure mstatus.MIE is set to allow all interrupts */
		"csrs mstatus, %1\n"
		:
		: "r" (MIE_MTIE | MIE_MSIE), "r" (RV_STATUS_IE)
		: "memory");
}

bool z_soc_irq_unlocked(unsigned int key)
{
	return (key == 0);
}

void arch_cpu_idle(void)
{
	/* We need to set mie.MTIE to enable timer interrupts, and
	 * disable mstatus.MIE to prevent vectoring before we unlock interrupts
	 */
	__asm__ volatile (
		/* Set mie.MTIE to enable timer interrupts */
		"csrs mie, %0\n"
		/* Clear mstatus.MIE to prevent vectoring before we unlock interrupts */
		"csrc mstatus, %1\n"
		:
		: "r" (MIE_MTIE), "r" (RV_STATUS_IE)
		: "memory");
#if defined(CONFIG_SYS_IDLE_HOOKS)
	sys_trace_idle();
#endif
	__asm__ volatile("wfi");
#if defined(CONFIG_SYS_IDLE_HOOKS)
	sys_trace_idle_exit();
#endif
	irq_unlock(RV_STATUS_IE);
}

#endif
