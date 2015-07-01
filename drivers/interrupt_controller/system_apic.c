/* systemApic.c - system module for variants with LOAPIC */

/*
 * Copyright (c) 2013-2015, Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
DESCRIPTION
This module provides routines to initialize and support board-level hardware
for the atom_n28xx variant of generic_pc BSP.
 */

#include <misc/__assert.h>
#include "board.h"
#include <nanokernel.h>
#include <arch/cpu.h>
#include <drivers/ioapic.h>
#include <drivers/loapic.h>

/**
 *
 * @brief Allocate interrupt vector
 *
 * This BSP provided routine supports the irq_connect() API.  This
 * routine is required to perform the following 3 functions:
 *
 *  a) Allocate a vector satisfying the requested priority.  The utility routine
 *     _IntVecAlloc() provided by the nanokernel will be used to perform the
 *     the allocation since the local APIC prioritizes interrupts as assumed
 *     by _IntVecAlloc().
 *  b) Return End of Interrupt (EOI) and Beginning of Interrupt (BOI) related
 *     information to be used when generating the interrupt stub code, and
 *  c) If an interrupt vector can be allocated, and the <irq> argument is not
 *     equal to NANO_SOFT_IRQ, the IOAPIC redirection table (RED) or the
 *     LOAPIC local vector table (LVT) will be updated with the allocated
 *     interrupt vector.
 *
 * The board virtualizes IRQs as follows:
 *
 * - The first IOAPIC_NUM_RTES IRQs are provided by the IOAPIC
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
 * The IOAPIC_NUM_RTES macro is provided by board.h, and it specifies the number
 * of IRQs supported by the on-board I/O APIC device.
 *
 * @return the allocated interrupt vector
 *
 * INTERNAL
 * For debug kernels, this routine will return -1 if there are no vectors
 * remaining in the specified <priority> level, or if the <priority> or <irq>
 * parameters are invalid.
 */

int _SysIntVecAlloc(
	unsigned int irq,		 /* virtualized IRQ */
	unsigned int priority,		 /* get vector from <priority> group */
	NANO_EOI_GET_FUNC * boiRtn,       /* ptr to BOI routine; NULL if none */
	NANO_EOI_GET_FUNC * eoiRtn,       /* ptr to EOI routine; NULL if none */
	void **boiRtnParm,		 /* BOI routine parameter, if any */
	void **eoiRtnParm,		 /* EOI routine parameter, if any */
	unsigned char *boiParamRequired, /* BOI routine parameter req? */
	unsigned char *eoiParamRequired  /* BOI routine parameter req? */
	)
{
	int vector;

	ARG_UNUSED(boiRtnParm);
	ARG_UNUSED(boiParamRequired);

#if defined(DEBUG)
	if ((priority > 15) ||
	    ((irq > (IOAPIC_NUM_RTES + 5)) && (irq != NANO_SOFT_IRQ)))
		return -1;
#endif

	/*
	 * Use the nanokernel utility function _IntVecAlloc().  A value of
	 * -1 will be returned if there are no free vectors in the requested
	 * priority.
	 */

	vector = _IntVecAlloc(priority);
	__ASSERT(vector != -1, "No free vectors in the requested priority");

	/*
	 * Set up the appropriate interrupt controller to generate the allocated
	 * interrupt vector for the specified IRQ.  Also, provide the required
	 * EOI and BOI related information for the interrupt stub code
	 *generation
	 * step.
	 *
	 * For software interrupts (NANO_SOFT_IRQ), skip the interrupt
	 *controller
	 * programming step, and indicate that a BOI and EOI handler is not
	 * required.
	 *
	 * Skip both steps if a vector could not be allocated.
	 */

	*boiRtn = (NANO_EOI_GET_FUNC)NULL; /* a BOI handler is never required */
	*eoiRtn = (NANO_EOI_GET_FUNC)NULL; /* assume NANO_SOFT_IRQ */

#if defined(DEBUG)
	if ((vector != -1) && (irq != NANO_SOFT_IRQ))
#else
	if (irq != NANO_SOFT_IRQ)
#endif
	{
		if (irq < IOAPIC_NUM_RTES) {
			_ioapic_int_vec_set(irq, vector);

			/*
			 * query IOAPIC driver to obtain EOI handler information
			 * for the
			 * interrupt vector that was just assigned to the
			 * specified IRQ
			 */

			*eoiRtn = (NANO_EOI_GET_FUNC)_ioapic_eoi_get(
				irq, (char *)eoiParamRequired, eoiRtnParm);
		} else {
			_loapic_int_vec_set(irq - IOAPIC_NUM_RTES, vector);

			/* specify that the EOI handler in loApicIntr.c driver
			 * be invoked */

			*eoiRtn = (NANO_EOI_GET_FUNC)_loapic_eoi;
			*eoiParamRequired = 0;
		}
	}

	return vector;
}

/**
 *
 * @brief Program interrupt controller
 *
 * This BSP provided routine programs the appropriate interrupt controller
 * with the given vector based on the given IRQ parameter.
 *
 * Drivers call this routine instead of irq_connect() when interrupts are
 * configured statically.
 *
 * The Clanton board virtualizes IRQs as follows:
 *
 * - The first IOAPIC_NUM_RTES IRQs are provided by the IOAPIC so the IOAPIC
 *     is programmed for these IRQs
 * - The remaining IRQs are provided by the LOAPIC and hence the LOAPIC is
 *     programmed.
 *
 * The IOAPIC_NUM_RTES macro is provided by board.h, and it specifies the number
 * of IRQs supported by the on-board I/O APIC device.
 *
 */

void _SysIntVecProgram(unsigned int vector, /* vector number */
		       unsigned int irq     /* virtualized IRQ */
		       )
{

	if (irq < IOAPIC_NUM_RTES) {
		_ioapic_int_vec_set(irq, vector);
	} else {
		_loapic_int_vec_set(irq - IOAPIC_NUM_RTES, vector);
	}
}


/**
 *
 * @brief Enable an individual interrupt (IRQ)
 *
 * The public interface for enabling/disabling a specific IRQ for the IA-32
 * architecture is defined as follows in include/nanokernel/x86/arch.h
 *
 *   extern void  irq_enable  (unsigned int irq);
 *   extern void  irq_disable (unsigned int irq);
 *
 * The irq_enable() routine is provided by the BSP due to the
 * IRQ virtualization that is performed by this BSP.  See the comments
 * in _SysIntVecAlloc() for more information regarding IRQ virtualization.
 *
 * @return N/A
 */

void irq_enable(unsigned int irq)
{
	if (irq < IOAPIC_NUM_RTES) {
		_ioapic_irq_enable(irq);
	} else {
		_loapic_irq_enable(irq - IOAPIC_NUM_RTES);
	}
}

/**
 *
 * @brief Disable an individual interrupt (IRQ)
 *
 * The irq_disable() routine is provided by the BSP due to the
 * IRQ virtualization that is performed by this BSP.  See the comments
 * in _SysIntVecAlloc() for more information regarding IRQ virtualization.
 *
 * @return N/A
 */

void irq_disable(unsigned int irq)
{
	if (irq < IOAPIC_NUM_RTES) {
		_ioapic_irq_disable(irq);
	} else {
		_loapic_irq_disable(irq - IOAPIC_NUM_RTES);
	}
}
