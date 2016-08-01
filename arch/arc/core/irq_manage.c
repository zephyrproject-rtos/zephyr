/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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

#include <nanokernel.h>
#include <arch/cpu.h>
#include <misc/__assert.h>
#include <toolchain.h>
#include <sections.h>
#include <sw_isr_table.h>
#include <irq.h>

/*
 * @brief Enable an interrupt line
 *
 * Clear possible pending interrupts on the line, and enable the interrupt
 * line. After this call, the CPU will receive interrupts for the specified
 * @a irq.
 *
 * @return N/A
 */

void _arch_irq_enable(unsigned int irq)
{
	int key = irq_lock();

	_arc_v2_irq_unit_int_enable(irq);
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

void _arch_irq_disable(unsigned int irq)
{
	int key = irq_lock();

	_arc_v2_irq_unit_int_disable(irq);
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

void _irq_priority_set(unsigned int irq, unsigned int prio, uint32_t flags)
{
	ARG_UNUSED(flags);

	int key = irq_lock();

	__ASSERT(prio < CONFIG_NUM_IRQ_PRIO_LEVELS,
		 "invalid priority %d for irq %d", prio, irq);
	_arc_v2_irq_unit_prio_set(irq, prio);
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

#include <misc/printk.h>
void _irq_spurious(void *unused)
{
	ARG_UNUSED(unused);
	printk("_irq_spurious(). Spinning...\n");
	for (;;)
		;
}

