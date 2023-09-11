/*
 * Copyright (c) 2016 Jean-Paul Etienne <fractalclone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <kernel_internal.h>
#include <zephyr/logging/log.h>
#include <zephyr/arch/riscv/csr.h>

LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

FUNC_NORETURN void z_irq_spurious(const void *unused)
{
	unsigned long mcause;

	ARG_UNUSED(unused);

	mcause = csr_read(mcause);

	mcause &= SOC_MCAUSE_EXP_MASK;

	LOG_ERR("Spurious interrupt detected! IRQ: %ld", mcause);
#if defined(CONFIG_RISCV_HAS_PLIC)
	if (mcause == RISCV_MACHINE_EXT_IRQ) {
		LOG_ERR("PLIC interrupt instance causing the IRQ: %p",
			riscv_plic_get_dev());
		LOG_ERR("PLIC interrupt line causing the IRQ: %d",
			riscv_plic_get_irq());
	}
#endif
	z_riscv_fatal_error(K_ERR_SPURIOUS_IRQ, NULL);
}

#ifdef CONFIG_DYNAMIC_INTERRUPTS
int arch_irq_intc_connect_dynamic(unsigned int irq, unsigned int priority,
				  void (*routine)(const void *parameter),
				  const void *parameter, uint32_t flags,
				  const struct device *dev)
{
#if defined(CONFIG_RISCV_HAS_PLIC)
	z_riscv_plic_isr_intc_install(irq, priority, routine, parameter, dev);
#else
	ARG_UNUSED(dev);
	ARG_UNUSED(flags);
	z_isr_install(irq, routine, parameter);
	ARG_UNUSED(priority);
#endif
	return irq;
}

int arch_irq_connect_dynamic(unsigned int irq, unsigned int priority,
			     void (*routine)(const void *parameter),
			     const void *parameter, uint32_t flags)
{
	arch_irq_intc_connect_dynamic(irq, priority, routine, parameter, flags, NULL);

	return irq;
}
#endif /* CONFIG_DYNAMIC_INTERRUPTS */
