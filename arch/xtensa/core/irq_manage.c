/*
 * Copyright (c) 2016 Cadence Design Systems, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stdio.h>
#include <zephyr/arch/xtensa/irq.h>
#include <zephyr/sys/__assert.h>

#include <kernel_arch_func.h>

#include <xtensa_internal.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

/**
 * @internal
 *
 * @brief Set an interrupt's priority
 *
 * The priority is verified if ASSERT_ON is enabled.
 *
 * The priority is verified if ASSERT_ON is enabled. The maximum number of
 * priority levels is a little complex, as there are some hardware priority
 * levels which are reserved: three for various types of exceptions, and
 * possibly one additional to support zero latency interrupts.
 *
 * Valid values are from 1 to 6. Interrupts of priority 1 are not masked when
 * interrupts are locked system-wide, so care must be taken when using them.
 * ISR installed with priority 0 interrupts cannot make kernel calls.
 */
void z_irq_priority_set(unsigned int irq, unsigned int prio, uint32_t flags)
{
	__ASSERT(prio < XCHAL_EXCM_LEVEL + 1,
		 "invalid priority %d! values must be less than %d\n",
		 prio, XCHAL_EXCM_LEVEL + 1);
	/* TODO: Write code to set priority if this is ever possible on
	 * Xtensa
	 */
}

#ifdef CONFIG_DYNAMIC_INTERRUPTS
#ifndef CONFIG_MULTI_LEVEL_INTERRUPTS
int z_arch_irq_connect_dynamic(unsigned int irq, unsigned int priority,
			       void (*routine)(const void *parameter),
			       const void *parameter, uint32_t flags)
{
	ARG_UNUSED(flags);
	ARG_UNUSED(priority);

	z_isr_install(irq, routine, parameter);
	return irq;
}
#else /* !CONFIG_MULTI_LEVEL_INTERRUPTS */
int z_arch_irq_connect_dynamic(unsigned int irq, unsigned int priority,
			       void (*routine)(const void *parameter),
			       const void *parameter, uint32_t flags)
{
	return z_soc_irq_connect_dynamic(irq, priority, routine, parameter,
					 flags);
}
#endif /* !CONFIG_MULTI_LEVEL_INTERRUPTS */
#endif /* CONFIG_DYNAMIC_INTERRUPTS */

void z_irq_spurious(const void *arg)
{
	int irqs, ie;

	ARG_UNUSED(arg);

	__asm__ volatile("rsr.interrupt %0" : "=r"(irqs));
	__asm__ volatile("rsr.intenable %0" : "=r"(ie));
	LOG_ERR(" ** Spurious INTERRUPT(s) %p, INTENABLE = %p",
		(void *)irqs, (void *)ie);
	xtensa_fatal_error(K_ERR_SPURIOUS_IRQ, NULL);
}

int xtensa_irq_is_enabled(unsigned int irq)
{
	uint32_t ie;

	__asm__ volatile("rsr.intenable %0" : "=r"(ie));

	return (ie & (1 << irq)) != 0U;
}
