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

LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

void __weak z_riscv_log_saved_irq_and_device(unsigned long cause)
{
	ARG_UNUSED(cause);
}

FUNC_NORETURN void z_irq_spurious(const void *unused)
{
#ifdef CONFIG_EMPTY_IRQ_SPURIOUS
	while (1) {
	}

	CODE_UNREACHABLE;
#else
	unsigned long cause;

	ARG_UNUSED(unused);

#ifdef CONFIG_RISCV_S_MODE
	cause = csr_read(scause);
#else
	cause = csr_read(mcause);
#endif

	cause &= CONFIG_RISCV_MCAUSE_EXCEPTION_MASK;

	LOG_ERR("Spurious interrupt detected! IRQ: %ld", cause);
	z_riscv_log_saved_irq_and_device(cause);

#if defined(CONFIG_RISCV_SOC_HAS_SPURIOUS_IRQ_HOOK)
	z_riscv_spurious_irq_hook();
#endif

	z_riscv_fatal_error(K_ERR_SPURIOUS_IRQ, NULL);
	CODE_UNREACHABLE;
#endif /* CONFIG_EMPTY_IRQ_SPURIOUS */
}

#ifdef CONFIG_DYNAMIC_INTERRUPTS
int arch_irq_connect_dynamic(unsigned int irq, unsigned int priority,
			     void (*routine)(const void *parameter),
			     const void *parameter, uint32_t flags)
{
	z_isr_install(irq + CONFIG_RISCV_RESERVED_IRQ_ISR_TABLES_OFFSET, routine, parameter);

#if defined(CONFIG_RISCV_HAS_PLIC) || defined(CONFIG_RISCV_HAS_CLIC) ||                            \
	defined(CONFIG_RISCV_HAS_AIA)
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
