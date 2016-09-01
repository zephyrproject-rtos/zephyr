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
 * Enable the interrupt. After this call, the CPU will receive interrupts for
 * the specified <irq>.
 *
 * @return N/A
 */
void _arch_irq_enable(unsigned int irq)
{
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
	/* The last priority level is also used by PendSV exception, but
	 * allow other interrupts to use the same level, even if it ends up
	 * affecting performance (can still be useful on systems with a
	 * reduced set of priorities, like Cortex-M0/M0+).
	 */
	__ASSERT(prio <= ((1 << CONFIG_NUM_IRQ_PRIO_BITS) - 1),
		 "invalid priority %d! values must be less than %d\n",
		 prio - IRQ_PRIORITY_OFFSET,
		 (1 << CONFIG_NUM_IRQ_PRIO_BITS) - (IRQ_PRIORITY_OFFSET));
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

