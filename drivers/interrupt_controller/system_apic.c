/*
 * Copyright (c) 2013-2015, Wind River Systems, Inc.
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
 * @brief  system module for variants with LOAPIC
 *
 */

#include <misc/__assert.h>
#include "board.h"
#include <nanokernel.h>
#include <arch/cpu.h>
#include <drivers/ioapic.h>
#include <drivers/loapic.h>
#include <drivers/sysapic.h>
#include <irq.h>


/* forward declarations */

static int __LocalIntVecAlloc(unsigned int irq, unsigned int priority);

/**
 *
 * @brief Allocate interrupt vector
 *
 * This routine is used by the x86's irq_connect_dynamic().  It performs the
 * following functions:
 *
 *  a) Allocates a vector satisfying the requested priority.  The utility
 *     routine _IntVecAlloc() provided by the nanokernel will be used to
 *     perform the the allocation since the local APIC prioritizes interrupts
 *     as assumed by _IntVecAlloc().
 *  b) If an interrupt vector can be allocated, the IOAPIC redirection table
 *     (RED) or the LOAPIC local vector table (LVT) will be updated with the
 *     allocated interrupt vector.
 *
 * The board virtualizes IRQs as follows:
 *
 * - The first CONFIG_IOAPIC_NUM_RTES IRQs are provided by the IOAPIC
 * - The remaining IRQs are provided by the LOAPIC.
 *
 * Thus, for example, if the IOAPIC supports 24 IRQs:
 *
 * - IRQ0 to IRQ23   map to IOAPIC IRQ0 to IRQ23
 * - IRQ24 to IRQ29  map to LOAPIC LVT entries as follows:
 *
 *       IRQ24 -> LOAPIC_TIMER
 *       IRQ25 -> LOAPIC_THERMAL
 *       IRQ26 -> LOAPIC_PMC
 *       IRQ27 -> LOAPIC_LINT0
 *       IRQ28 -> LOAPIC_LINT1
 *       IRQ29 -> LOAPIC_ERROR
 *
 * @param irq virtualized IRQ
 * @param priority get vector from <priority> group
 * @param flags Interrupt flags
 *
 * @return the allocated interrupt vector
 *
 * @internal
 * For debug kernels, this routine will return -1 if there are no vectors
 * remaining in the specified <priority> level, or if the <priority> or <irq>
 * parameters are invalid.
 * @endinternal
 */
int _SysIntVecAlloc(
	unsigned int irq,		 /* virtualized IRQ */
	unsigned int priority,		 /* get vector from <priority> group */
	uint32_t flags			 /* interrupt flags */
	)
{
	int vector;

	__ASSERT(priority < 14, "invalid priority");
	__ASSERT(irq >= 0 && irq <= HARDWARE_IRQ_LIMIT, "invalid irq line");

	vector = __LocalIntVecAlloc(irq, priority);

	/*
	 * Set up the appropriate interrupt controller to generate the allocated
	 * interrupt vector for the specified IRQ
	 */
	__ASSERT(vector != -1, "bad vector id");
	_SysIntVecProgram(vector, irq, flags);

	return vector;
}

/**
 *
 * @brief Program interrupt controller
 *
 * This routine programs the interrupt controller with the given vector
 * based on the given IRQ parameter.
 *
 * Drivers call this routine instead of IRQ_CONNECT() when interrupts are
 * configured statically.
 *
 * The Galileo board virtualizes IRQs as follows:
 *
 * - The first CONFIG_IOAPIC_NUM_RTES IRQs are provided by the IOAPIC so the
 *     IOAPIC is programmed for these IRQs
 * - The remaining IRQs are provided by the LOAPIC and hence the LOAPIC is
 *     programmed.
 *
 * @param vector the vector number
 * @param irq the virtualized IRQ
 * @param flags interrupt flags
 *
 */
void _SysIntVecProgram(unsigned int vector, unsigned int irq, uint32_t flags)
{
	if (IS_IOAPIC_IRQ(irq)) {
		_ioapic_irq_set(irq, vector, flags);
	} else {
		_loapic_int_vec_set(irq - LOAPIC_IRQ_BASE, vector);
	}
}

/**
 *
 * @brief Enable an individual interrupt (IRQ)
 *
 * The public interface for enabling/disabling a specific IRQ for the IA-32
 * architecture is defined as follows in include/arch/x86/arch.h
 *
 *   extern void  irq_enable  (unsigned int irq);
 *   extern void  irq_disable (unsigned int irq);
 *
 * The irq_enable() routine is provided by the interrupt controller driver due
 * to the IRQ virtualization that is performed by this platform.  See the
 * comments in _SysIntVecAlloc() for more information regarding IRQ
 * virtualization.
 *
 * @return N/A
 */
void _arch_irq_enable(unsigned int irq)
{
	if (IS_IOAPIC_IRQ(irq)) {
		_ioapic_irq_enable(irq);
	} else {
		_loapic_irq_enable(irq - LOAPIC_IRQ_BASE);
	}
}

/**
 *
 * @brief Disable an individual interrupt (IRQ)
 *
 * The irq_disable() routine is provided by the interrupt controller driver due
 * to the IRQ virtualization that is performed by this platform.  See the
 * comments in _SysIntVecAlloc() for more information regarding IRQ
 * virtualization.
 *
 * @return N/A
 */
void _arch_irq_disable(unsigned int irq)
{
	if (IS_IOAPIC_IRQ(irq)) {
		_ioapic_irq_disable(irq);
	} else {
		_loapic_irq_disable(irq - LOAPIC_IRQ_BASE);
	}
}

#ifdef FIXED_HARDWARE_IRQ_TO_VEC_MAPPING
static inline int handle_fixed_mapping(int irq, int *vector)
{
	/*
	 * On this board Hardware IRQs are fixed and not programmable.
	 */
	*vector = FIXED_HARDWARE_IRQ_TO_VEC_MAPPING(irq);

	/* mark vector as allocated */
	_IntVecMarkAllocated(*vector);

	return *vector;
}
#else
static inline int handle_fixed_mapping(int irq, int *vector)
{
	ARG_UNUSED(irq);
	ARG_UNUSED(vector);
	return 0;
}
#endif

/**
 *
 * @brief Local allocate interrupt vector.
 *
 * @returns The allocated interrupt vector
 *
 */
static int __LocalIntVecAlloc(
	unsigned int irq,		 /* virtualized IRQ */
	unsigned int priority	/* get vector from <priority> group */
	)
{
	int vector;

	if (handle_fixed_mapping(irq, &vector)) {
		return vector;
	}

	/*
	* Use the nanokernel utility function _IntVecAlloc().  A value of
	* -1 will be returned if there are no free vectors in the requested
	* priority.
	*/

	vector = _IntVecAlloc(priority);
	__ASSERT(vector != -1, "No free vectors in the requested priority");

	return vector;
}
