/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sw_isr_table.h>
#include <arch/cpu.h>
#include <sys/__assert.h>
/*
 * Common code for arches that use software ISR tables (CONFIG_GEN_ISR_TABLES)
 */

#ifdef CONFIG_DYNAMIC_INTERRUPTS
void z_isr_install(unsigned int irq, void (*routine)(void *), void *param)
{
	unsigned int table_idx = irq - CONFIG_GEN_IRQ_START_VECTOR;

	__ASSERT(!irq_is_enabled(irq), "IRQ %d is enabled", irq);

	/* If dynamic IRQs are enabled, then the _sw_isr_table is in RAM and
	 * can be modified
	 */
	_sw_isr_table[table_idx].arg = param;
	_sw_isr_table[table_idx].isr = routine;
}

/* Some architectures don't/can't interpret flags or priority and have
 * no more processing to do than this.  Provide a generic fallback.
 */
int __weak z_arch_irq_connect_dynamic(unsigned int irq,
				      unsigned int priority,
				      void (*routine)(void *),
				      void *parameter,
				      u32_t flags)
{
	ARG_UNUSED(flags);
	ARG_UNUSED(priority);

	z_isr_install(irq, routine, parameter);
	return irq;
}

#endif /* CONFIG_DYNAMIC_INTERRUPTS */
