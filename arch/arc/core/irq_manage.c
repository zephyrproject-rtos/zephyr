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
 * - dynamic ISR connecting/replacing
 *
 * SW_ISR_TABLE_DYNAMIC has to be enabled for connecting ISRs at runtime.
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

/*
 * @internal
 *
 * @brief Replace an interrupt handler by another
 *
 * An interrupt's ISR can be replaced at runtime.
 *
 * @return N/A
 */

void _irq_handler_set(
	unsigned int irq,
	void (*new)(void *arg),
	void *arg
)
{
	int key = irq_lock();
	int index = irq - 16;

	_sw_isr_table[index].isr = new;
	_sw_isr_table[index].arg = arg;

	irq_unlock(key);
}

/*
 * @brief Enable an interrupt line
 *
 * Clear possible pending interrupts on the line, and enable the interrupt
 * line. After this call, the CPU will receive interrupts for the specified
 * @a irq.
 *
 * @return N/A
 */

void irq_enable(unsigned int irq)
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

void irq_disable(unsigned int irq)
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
 * Valid values are from 0 to 15. Interrupts of priority 1 are not masked when
 * interrupts are locked system-wide, so care must be taken when using them. ISR
 * installed with priority 0 interrupts cannot make kernel calls.
 *
 * The priority is verified if ASSERT_ON is enabled.
 *
 * @return N/A
 */

void _irq_priority_set(
	unsigned int irq,
	unsigned int prio
)
{
	int key = irq_lock();

	__ASSERT(prio >= 0 && prio < CONFIG_NUM_IRQ_PRIO_LEVELS,
			 "invalid priority!");
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

/*
 * @brief Connect an ISR to an interrupt line
 *
 * @a isr is connected to interrupt line @a irq, a number greater than or equal
 * 16. No prior ISR can have been connected on @a irq interrupt line since the
 * system booted.
 *
 * This routine will hang if another ISR was connected for interrupt line @a irq
 * and ASSERT_ON is enabled; if ASSERT_ON is disabled, it will fail silently.
 *
 * @return the interrupt line number
 */

int irq_connect(
	unsigned int irq,
	unsigned int prio,
	void (*isr)(void *arg),
	void *arg,
	uint32_t flags
)
{
	ARG_UNUSED(flags);
	_irq_handler_set(irq, isr, arg);
	_irq_priority_set(irq, prio);
	return irq;
}

/*
 * @internal
 *
 * @brief Disconnect an ISR from an interrupt line
 *
 * Interrupt line @a irq is disconnected from its ISR and the latter is
 * replaced by _irq_spurious(). irq_disable() should have been called before
 * invoking this routine.
 *
 * @return N/A
 */

void _irq_disconnect(unsigned int irq)
{
	_irq_handler_set(irq, _irq_spurious, NULL);
}
