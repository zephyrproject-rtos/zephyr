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

#if defined(CONFIG_GEN_SW_ISR_TABLE)
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
#endif

void z_openrisc_enter_irq(unsigned int irq)
{
#if defined(CONFIG_GEN_SW_ISR_TABLE)
	enter_irq(irq);
#else
	ARG_UNUSED(irq);
	z_irq_spurious(NULL);
#endif
}

void z_openrisc_handle_irqs(void)
{
#if defined(CONFIG_GEN_SW_ISR_TABLE)
	uint32_t picsr;

	/* Iteratively process every interrupt flag */
	while ((picsr = openrisc_read_spr(SPR_PICSR))) {
		const unsigned int irq = find_lsb_set(picsr) - 1;

		/* Clear interrupt flag */
		openrisc_write_spr(SPR_PICSR, picsr & ~(1 << irq));

		enter_irq(irq);
	}

	if (IS_ENABLED(CONFIG_STACK_SENTINEL)) {
		z_check_stack_sentinel();
	}
#else
	z_irq_spurious(NULL);
#endif
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
