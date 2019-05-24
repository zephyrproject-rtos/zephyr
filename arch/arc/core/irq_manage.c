/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARCv2 interrupt management
 *
 *
 * Interrupt management:
 *
 * - enabling/disabling
 *
 * An IRQ number passed to the @a irq parameters found in this file is a
 * number from 16 to last IRQ number on the platform.
 */

#include <kernel.h>
#include <arch/cpu.h>
#include <misc/__assert.h>
#include <toolchain.h>
#include <linker/sections.h>
#include <sw_isr_table.h>
#include <irq.h>
#include <misc/printk.h>

/*
 * @brief Enable an interrupt line
 *
 * Clear possible pending interrupts on the line, and enable the interrupt
 * line. After this call, the CPU will receive interrupts for the specified
 * @a irq.
 *
 * @return N/A
 */

void z_arch_irq_enable(unsigned int irq)
{
	unsigned int key = irq_lock();

	z_arc_v2_irq_unit_int_enable(irq);
	irq_unlock(key);
}

/*
 * @brief Disable an interrupt line
 *
 * Disable an interrupt line. After this call, the CPU will stop receiving
 * interrupts for the specified @a irq.
 *
 * @return N/A
 */

void z_arch_irq_disable(unsigned int irq)
{
	unsigned int key = irq_lock();

	z_arc_v2_irq_unit_int_disable(irq);
	irq_unlock(key);
}

/*
 * @internal
 *
 * @brief Set an interrupt's priority
 *
 * Lower values take priority over higher values. Special case priorities are
 * expressed via mutually exclusive flags.

 * The priority is verified if ASSERT_ON is enabled; max priority level
 * depends on CONFIG_NUM_IRQ_PRIO_LEVELS.
 *
 * @return N/A
 */

void z_irq_priority_set(unsigned int irq, unsigned int prio, u32_t flags)
{
	ARG_UNUSED(flags);

	unsigned int key = irq_lock();

	__ASSERT(prio < CONFIG_NUM_IRQ_PRIO_LEVELS,
		 "invalid priority %d for irq %d", prio, irq);
	z_arc_v2_irq_unit_prio_set(irq, prio);
	irq_unlock(key);
}

/*
 * @brief Spurious interrupt handler
 *
 * Installed in all dynamic interrupt slots at boot time. Throws an error if
 * called.
 *
 * @return N/A
 */

void z_irq_spurious(void *unused)
{
	ARG_UNUSED(unused);
	printk("z_irq_spurious(). Spinning...\n");
	for (;;) {
		;
	}
}

#ifdef CONFIG_DYNAMIC_INTERRUPTS
int z_arch_irq_connect_dynamic(unsigned int irq, unsigned int priority,
			      void (*routine)(void *parameter), void *parameter,
			      u32_t flags)
{
	z_isr_install(irq, routine, parameter);
	z_irq_priority_set(irq, priority, flags);
	return irq;
}
#endif /* CONFIG_DYNAMIC_INTERRUPTS */
