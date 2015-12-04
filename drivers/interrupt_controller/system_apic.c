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
 * This module provides routines to initialize and support
 * board-level hardware for the basic_atom configuration of
 * ia32 platform.
 */

#include <misc/__assert.h>
#include "board.h"
#include <nanokernel.h>
#include <arch/cpu.h>
#include <drivers/ioapic.h>
#include <drivers/loapic.h>

#if !defined(LOAPIC_IRQ_BASE) && !defined (LOAPIC_IRQ_COUNT)

/* Default IA32 system APIC definitions with local APIC IRQs after IO APIC. */

#define LOAPIC_IRQ_BASE  CONFIG_IOAPIC_NUM_RTES
#define LOAPIC_IRQ_COUNT 6  /* Default to LOAPIC_TIMER to LOAPIC_ERROR */

#define IS_IOAPIC_IRQ(irq)  (irq < LOAPIC_IRQ_BASE)

#define HARDWARE_IRQ_LIMIT ((LOAPIC_IRQ_BASE + LOAPIC_IRQ_COUNT) - 1)

#else

/*
  Assumption for boards that define LOAPIC_IRQ_BASE & LOAPIC_IRQ_COUNT that
  local APIC IRQs are within IOAPIC RTE range.
*/

#define IS_IOAPIC_IRQ(irq)  ((irq < LOAPIC_IRQ_BASE) || \
	(irq >= (LOAPIC_IRQ_BASE + LOAPIC_IRQ_COUNT)))

#define HARDWARE_IRQ_LIMIT (CONFIG_IOAPIC_NUM_RTES - 1)

#endif


/* forward declarations */

static int __LocalIntVecAlloc(unsigned int irq, unsigned int priority);

/**
 *
 * @brief Allocate interrupt vector
 *
 * This routine is used by the x86's irq_connect().  It performs the following
 * functions:
 *
 *  a) Allocates a vector satisfying the requested priority.  The utility
 *     routine _IntVecAlloc() provided by the nanokernel will be used to
 *     perform the the allocation since the local APIC prioritizes interrupts
 *     as assumed by _IntVecAlloc().
 *  b) If an interrupt vector can be allocated, and the <irq> argument is not
 *     equal to NANO_SOFT_IRQ, the IOAPIC redirection table (RED) or the
 *     LOAPIC local vector table (LVT) will be updated with the allocated
 *     interrupt vector.
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

#if defined(CONFIG_LOAPIC_DEBUG)
	if ((priority > 15) ||
		((irq > HARDWARE_IRQ_LIMIT) && (irq != NANO_SOFT_IRQ)))
		return -1;
#endif

	vector = __LocalIntVecAlloc(irq, priority);

	/*
	 * Set up the appropriate interrupt controller to generate the allocated
	 * interrupt vector for the specified IRQ.  Also, provide the required
	 * EOI related information for the interrupt stub code generation
	 * step.
	 *
	 * For software interrupts (NANO_SOFT_IRQ), skip the interrupt
	 * controller programming step, and indicate that an EOI handler is not
	 * required.
	 */

#if defined(CONFIG_LOAPIC_DEBUG)
	if ((vector != -1) && (irq != NANO_SOFT_IRQ))
#else
	if (irq != NANO_SOFT_IRQ)
#endif
	{
		_SysIntVecProgram(vector, irq, flags);
	}

	return vector;
}

/**
 *
 * @brief Program interrupt controller
 *
 * This routine programs the interrupt controller with the given vector
 * based on the given IRQ parameter.
 *
 * Drivers call this routine instead of irq_connect() when interrupts are
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
void irq_enable(unsigned int irq)
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
void irq_disable(unsigned int irq)
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
	if (irq != NANO_SOFT_IRQ) {

		/*
		 * On this board Hardware IRQs are fixed and not programmable.
		 */
		*vector = FIXED_HARDWARE_IRQ_TO_VEC_MAPPING(irq);

		/* mark vector as allocated */
		_IntVecMarkAllocated(*vector);

		return *vector;
	}
	return 0;
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
