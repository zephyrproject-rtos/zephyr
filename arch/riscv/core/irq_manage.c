/*
 * Copyright (c) 2016 Jean-Paul Etienne <fractalclone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <kernel_internal.h>
#include <zephyr/logging/log.h>
#include <zephyr/arch/riscv/csr.h>
#include <zephyr/irq_multilevel.h>
#include <zephyr/sw_isr_table.h>
#include <zephyr/pm/pm.h>

#ifdef CONFIG_RISCV_HAS_PLIC
#include <zephyr/drivers/interrupt_controller/riscv_plic.h>
#endif

LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

FUNC_NORETURN void z_irq_spurious(const void *unused)
{
#ifdef CONFIG_EMPTY_IRQ_SPURIOUS
	while (1) {
	}

	CODE_UNREACHABLE;
#else
	unsigned long mcause;

	ARG_UNUSED(unused);

	mcause = csr_read(mcause);

	mcause &= CONFIG_RISCV_MCAUSE_EXCEPTION_MASK;

	LOG_ERR("Spurious interrupt detected! IRQ: %ld", mcause);
#if defined(CONFIG_RISCV_HAS_PLIC)
	if (mcause == RISCV_IRQ_MEXT) {
		unsigned int save_irq = riscv_plic_get_irq();
		const struct device *save_dev = riscv_plic_get_dev();

		LOG_ERR("PLIC interrupt line causing the IRQ: %d (%p)", save_irq, save_dev);
	}
#endif
	z_riscv_fatal_error(K_ERR_SPURIOUS_IRQ, NULL);
#endif /* CONFIG_EMPTY_IRQ_SPURIOUS */
}

#ifdef CONFIG_DYNAMIC_INTERRUPTS
int arch_irq_connect_dynamic(unsigned int irq, unsigned int priority,
			     void (*routine)(const void *parameter),
			     const void *parameter, uint32_t flags)
{
	z_isr_install(irq + CONFIG_RISCV_RESERVED_IRQ_ISR_TABLES_OFFSET, routine, parameter);

#if defined(CONFIG_RISCV_HAS_PLIC) || defined(CONFIG_RISCV_HAS_CLIC)
	z_riscv_irq_priority_set(irq, priority, flags);
#else
	ARG_UNUSED(flags);
	ARG_UNUSED(priority);
#endif
	return irq;
}

#ifdef CONFIG_SHARED_INTERRUPTS
int arch_irq_disconnect_dynamic(unsigned int irq, unsigned int priority,
				void (*routine)(const void *parameter), const void *parameter,
				uint32_t flags)
{
	ARG_UNUSED(priority);
	ARG_UNUSED(flags);

	return z_isr_uninstall(irq + CONFIG_RISCV_RESERVED_IRQ_ISR_TABLES_OFFSET, routine,
			       parameter);
}
#endif /* CONFIG_SHARED_INTERRUPTS */
#endif /* CONFIG_DYNAMIC_INTERRUPTS */

#ifdef CONFIG_PM
void arch_isr_direct_pm(void)
{
	unsigned int key;

	key = irq_lock();

	if (_kernel.idle) {
		_kernel.idle = 0;
		pm_system_resume();
	}

	irq_unlock(key);
}
#endif
