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
#include <irq.h>

extern void __reserved(void);


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
void _arch_irq_enable(unsigned int irq)
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
void _arch_irq_disable(unsigned int irq)
{
	_NvicIrqDisable(irq);
}

/**
 * @internal
 *
 * @brief Set an interrupt's priority
 *
 * The priority is verified if ASSERT_ON is enabled. The maximum number
 * of priority levels is a little complex, as there are some hardware
 * priority levels which are reserved: three for various types of exceptions,
 * and possibly one additional to support zero latency interrupts.
 *
 * @return N/A
 */
void _irq_priority_set(unsigned int irq, unsigned int prio, uint32_t flags)
{
	/* Hardware priority levels 0 and 1 reserved for Kernel use.
	 * So we add 2 to the requested priority level. If we support
	 * ZLI, 2 is also reserved so we add 3.
	 */

#if CONFIG_ZERO_LATENCY_IRQS
#define IRQ_PRIORITY_OFFSET 3
	/* If we have zero latency interrupts, that makes priority level 2
	 * a case with special semantics; it is not masked by irq_lock().
	 * Our policy is to express priority levels with special properties
	 * via flags
	 */
	if (flags | IRQ_ZERO_LATENCY) {
		prio = 2;
	} else {
		prio += IRQ_PRIORITY_OFFSET;
	}
#else
#define IRQ_PRIORITY_OFFSET 2
	ARG_UNUSED(flags);
	prio += IRQ_PRIORITY_OFFSET;
#endif
	/* Last priority level reserved for PendSV exception */
	__ASSERT(prio < ((1 << CONFIG_NUM_IRQ_PRIO_BITS) - 1),
		 "invalid priority %d! values must be less than %d\n",
		 prio - IRQ_PRIORITY_OFFSET,
		 (1 << CONFIG_NUM_IRQ_PRIO_BITS) - (IRQ_PRIORITY_OFFSET + 1));
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

#if CONFIG_SW_ISR_TABLE_DYNAMIC
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
int _arch_irq_connect_dynamic(unsigned int irq,
					    unsigned int prio,
					    void (*isr)(void *arg),
					    void *arg,
					    uint32_t flags)
{
	ARG_UNUSED(flags);
	_irq_handler_set(irq, isr, arg);
	_irq_priority_set(irq, prio, flags);
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
#endif /* CONFIG_SW_ISR_TABLE_DYNAMIC */
