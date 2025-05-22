/*
 * Copyright (c) 2019 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/posix/posix_soc_if.h>
#include "board_irq.h"

#ifdef CONFIG_IRQ_OFFLOAD
#include <zephyr/irq_offload.h>

void arch_irq_offload(irq_offload_routine_t routine, const void *parameter)
{
	posix_irq_offload(routine, parameter);
}

void arch_irq_offload_init(void)
{
}
#endif

void arch_irq_enable(unsigned int irq)
{
	posix_irq_enable(irq);
}

void arch_irq_disable(unsigned int irq)
{
	posix_irq_disable(irq);
}

int arch_irq_is_enabled(unsigned int irq)
{
	return posix_irq_is_enabled(irq);
}

#ifdef CONFIG_DYNAMIC_INTERRUPTS
/**
 * Configure a dynamic interrupt.
 *
 * Use this instead of IRQ_CONNECT() if arguments cannot be known at build time.
 *
 * @param irq IRQ line number
 * @param priority Interrupt priority
 * @param routine Interrupt service routine
 * @param parameter ISR parameter
 * @param flags Arch-specific IRQ configuration flags
 *
 * @return The vector assigned to this interrupt
 */
int arch_irq_connect_dynamic(unsigned int irq, unsigned int priority,
			     void (*routine)(const void *parameter),
			     const void *parameter, uint32_t flags)
{
	posix_isr_declare(irq, (int)flags, routine, parameter);
	posix_irq_priority_set(irq, priority, flags);
	return irq;
}
#endif /* CONFIG_DYNAMIC_INTERRUPTS */
