/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
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
 * @brief ARM CORTEX-M3 interrupt management
 *
 *
 * Interrupt management: enabling/disabling and dynamic ISR
 * connecting/replacing.  SW_ISR_TABLE_DYNAMIC has to be enabled for
 * connecting ISRs at runtime.
 */

#include <nanokernel.h>
#include <arch/cpu.h>
#include <misc/__assert.h>
#include <toolchain.h>
#include <sections.h>
#include <sw_isr_table.h>

extern void __reserved(void);

/**
 * @internal
 *
 * @brief Replace an interrupt handler by another
 *
 * An interrupt's ISR can be replaced at runtime.
 *
 * @return N/A
 */
void _irq_handler_set(unsigned int irq,
						void (*new)(void *arg),
						void *arg)
{
	int key = irq_lock();

	_sw_isr_table[irq].isr = new;
	_sw_isr_table[irq].arg = arg;

	irq_unlock(key);
}

/**
 *
 * @brief Enable an interrupt line
 *
 * Clear possible pending interrupts on the line, and enable the interrupt
 * line. After this call, the CPU will receive interrupts for the specified
 * <irq>.
 *
 * @return N/A
 */
void irq_enable(unsigned int irq)
{
	/* before enabling interrupts, ensure that interrupt is cleared */
	_NvicIrqUnpend(irq);
	_NvicIrqEnable(irq);
}

/**
 *
 * @brief Disable an interrupt line
 *
 * Disable an interrupt line. After this call, the CPU will stop receiving
 * interrupts for the specified <irq>.
 *
 * @return N/A
 */
void irq_disable(unsigned int irq)
{
	_NvicIrqDisable(irq);
}

/**
 * @internal
 *
 * @brief Set an interrupt's priority
 *
 * Valid values are from 1 to 255. Interrupts of priority 1 are not masked when
 * interrupts are locked system-wide, so care must be taken when using them. ISR
 * installed with priority 1 interrupts cannot make kernel calls.
 *
 * Priority 0 is reserved for kernel usage and cannot be used.
 *
 * The priority is verified if ASSERT_ON is enabled.
 *
 * @return N/A
 */
void _irq_priority_set(unsigned int irq,
					     unsigned int prio)
{
	__ASSERT(prio > 0 && prio < 256, "invalid priority!");
	_NvicIrqPrioSet(irq, _EXC_PRIO(prio));
}

/**
 *
 * @brief Spurious interrupt handler
 *
 * Installed in all dynamic interrupt slots at boot time. Throws an error if
 * called.
 *
 * See __reserved().
 *
 * @return N/A
 */
void _irq_spurious(void *unused)
{
	ARG_UNUSED(unused);
	__reserved();
}

/**
 *
 * @brief Connect an ISR to an interrupt line
 *
 * <isr> is connected to interrupt line <irq> (exception #<irq>+16). No prior
 * ISR can have been connected on <irq> interrupt line since the system booted.
 *
 * This routine will hang if another ISR was connected for interrupt line <irq>
 * and ASSERT_ON is enabled; if ASSERT_ON is disabled, it will fail silently.
 *
 * @return the interrupt line number
 */
int irq_connect(unsigned int irq,
					    unsigned int prio,
					    void (*isr)(void *arg),
					    void *arg,
					    uint32_t flags)
{
	ARG_UNUSED(flags);
	_irq_handler_set(irq, isr, arg);
	_irq_priority_set(irq, prio);
	return irq;
}

/**
 *
 * @internal
 *
 * @brief Disconnect an ISR from an interrupt line
 *
 * Interrupt line <irq> (exception #<irq>+16) is disconnected from its ISR and
 * the latter is replaced by _irq_spurious(). irq_disable() should have
 * been called before invoking this routine.
 *
 * @return N/A
 */
void _irq_disconnect(unsigned int irq)
{
	_irq_handler_set(irq, _irq_spurious, NULL);
}
