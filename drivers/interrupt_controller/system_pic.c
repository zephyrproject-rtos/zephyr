/* systemPic.c - system module for variants with PIC */

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
for select configurations of the ia32 platform.
 */

#include "board.h"
#include <nanokernel.h>
#include <arch/cpu.h>
#include <drivers/pic.h>

/* Handle possible stray or spurious interrupts on the master and slave PICs */
IRQ_CONNECT_STATIC(pic_master, PIC_MASTER_STRAY_INT_LVL, 0, _i8259_boi_master, 0);
IRQ_CONNECT_STATIC(pic_slave, PIC_SLAVE_STRAY_INT_LVL, 0, _i8259_boi_slave, 0);

/**
 *
 * @brief Allocate interrupt vector
 *
 * This routine is used by the x86's irq_connect(). It performs the following
 * functions:
 *
 *  a) Allocates a vector satisfying the requested priority, where possible.
 *     When the <irq> argument is not equal to NANO_SOFT_IRQ, the vector
 *     assigned to the <irq> during interrupt controller initialization is
 *     returned, which may or may not have the desired prioritization.
 *     (Prioritization of such vectors is fixed by the 8259 interrupt
 *     controllers, and cannot be programmed on an IRQ basis; for example, IRQ0
 *     is always the highest priority interrupt no matter which interrupt
 *     vector was assigned to IRQ0.)
 *  b) Provides End of Interrupt (EOI) and Beginning of Interrupt (BOI) related
 *     information to be used when generating the interrupt stub code.
 *
 * The pcPentium4 board virtualizes IRQs as follows:
 *
 *  - IRQ0 to IRQ7  are provided by the master i8259 PIC
 *  - IRQ8 to IRQ15 are provided by the slave i8259 PIC
 *
 * @return the allocated interrupt vector
 *
 * INTERNAL
 * For debug kernels, this routine will return -1 for invalid <priority> or
 * <irq> parameter values.
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
	ARG_UNUSED(eoiRtnParm);

#if defined(DEBUG)
	if ((priority > 15) || (irq > 15) && (irq != NANO_SOFT_IRQ))
		return -1;
#endif

	/* The PIC BOI does not require a parameter */

	*boiParamRequired = 0;

	/* Assume BOI is not required */

	*boiRtn = (NANO_EOI_GET_FUNC)NULL;

	if (irq != NANO_SOFT_IRQ) {
		/* convert interrupt 'vector' to an interrupt controller IRQ
		 * number */

		vector = INT_VEC_IRQ0 + irq;

		/* mark vector as allocated */

		_IntVecMarkAllocated(vector);

		/* vector not handled by PIC, thus don't specify an EOI handler
		 */

		if (irq >= N_PIC_IRQS) {
			*eoiRtn = (NANO_EOI_GET_FUNC)NULL;
			return vector;
		}

		if (irq == PIC_MASTER_STRAY_INT_LVL) {
			*boiRtn = (NANO_EOI_GET_FUNC)_i8259_boi_master;
		} else if (irq == PIC_SLAVE_STRAY_INT_LVL) {
			*boiRtn = (NANO_EOI_GET_FUNC)_i8259_boi_slave;
		}

		if (irq <= PIC_MASTER_STRAY_INT_LVL)
			*eoiRtn = (NANO_EOI_GET_FUNC)_i8259_eoi_master;
		else
			*eoiRtn = (NANO_EOI_GET_FUNC)_i8259_eoi_slave;

		*eoiParamRequired = 0;
	} else {
		/*
		 * Use the nanokernel utility function _IntVecAlloc() to
		 * allocate
		 * a vector for software generated interrupts.
		 */

		vector = _IntVecAlloc(priority);
		*eoiRtn = (NANO_EOI_GET_FUNC)NULL;
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
 * For PIC-equipped boards this routine does nothing, as PIC does not need
 * any additional setup
 *
 */

void _SysIntVecProgram(unsigned int vector, /* vector number */
		       unsigned int irq     /* virtualized IRQ */
		       )
{
	ARG_UNUSED(vector);
	ARG_UNUSED(irq);
}
