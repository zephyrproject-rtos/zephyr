/*
 * Copyright (c) 2025 NVIDIA Corporation <jholdsworth@nvidia.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <kswap.h>

LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);


FUNC_NORETURN void z_irq_spurious(const void *unused)
{
	ARG_UNUSED(unused);

	LOG_ERR("Spurious interrupt detected!");

	z_openrisc_fatal_error(K_ERR_SPURIOUS_IRQ, NULL);
}

void arch_irq_enable(unsigned int irq)
{
	const unsigned int key = irq_lock();

	openrisc_write_spr(SPR_PICMR, openrisc_read_spr(SPR_PICMR) | BIT(irq));
	irq_unlock(key);
}

void arch_irq_disable(unsigned int irq)
{
	const unsigned int key = irq_lock();

	openrisc_write_spr(SPR_PICMR, openrisc_read_spr(SPR_PICMR) & ~BIT(irq));
	irq_unlock(key);
};

int arch_irq_is_enabled(unsigned int irq)
{
	return (openrisc_read_spr(SPR_PICMR) & BIT(irq)) != 0;
}

static ALWAYS_INLINE void enter_irq(unsigned int irq)
{
	if (IS_ENABLED(CONFIG_TRACING_ISR)) {
		sys_trace_isr_enter();
	}

	const struct _isr_table_entry *const ite = _sw_isr_table + irq;

	ite->isr(ite->arg);

	if (IS_ENABLED(CONFIG_TRACING_ISR)) {
		sys_trace_isr_exit();
	}
}

void z_openrisc_enter_irq(unsigned int irq)
{
	enter_irq(irq);
}

void z_openrisc_handle_irqs(void)
{
	uint32_t picsr;

	/* Interatively process every interrupt flag */
	while ((picsr = openrisc_read_spr(SPR_PICSR))) {
		enter_irq(find_lsb_set(picsr) - 1);
	}
}

#ifdef CONFIG_DYNAMIC_INTERRUPTS
int arch_irq_connect_dynamic(unsigned int irq, unsigned int priority,
			     void (*routine)(const void *parameter),
			     const void *parameter, uint32_t flags)
{
	ARG_UNUSED(flags);
	ARG_UNUSED(priority);

	z_isr_install(irq, routine, parameter);
	return irq;
}
#endif /* CONFIG_DYNAMIC_INTERRUPTS */
