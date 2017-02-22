/*
 * Copyright (c) 2017, Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the Intel Corporation nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL CORPORATION OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __MVIC_H__
#define __MVIC_H__

#include <stdint.h>

#include "qm_common.h"
#include "qm_soc_regs.h"

#define NUM_IRQ_LINES (32)

static uint32_t _mvic_get_irq_val(unsigned int irq)
{
	/*  Register Select - select which IRQ line we are configuring
	 *  Bits 0 and 4 are reserved
	 *  So, for IRQ 15 ( 0x01111 ) write 0x101110
	 */
	QM_IOAPIC->ioregsel.reg = ((irq & 0x7) << 1) | ((irq & 0x18) << 2);
	return QM_IOAPIC->iowin.reg;
}

static void _mvic_set_irq_val(unsigned int irq, uint32_t value)
{
	/*  Register Select - select which IRQ line we are configuring
	 *  Bits 0 and 4 are reserved
	 *  So, for IRQ 15 ( 0x01111 ) write 0x101110
	 */
	QM_IOAPIC->ioregsel.reg = ((irq & 0x7) << 1) | ((irq & 0x18) << 2);
	QM_IOAPIC->iowin.reg = value;
}

/**
 * Initialise MVIC.
 */
static __inline__ void mvic_init(void)
{
	uint32_t i;

	for (i = 0; i < NUM_IRQ_LINES; i++) {
		/* Clear up any spurious LAPIC interrupts, each call only
		 * clears one bit.
		 */
		QM_MVIC->eoi.reg = 0;

		/* Mask interrupt */
		_mvic_set_irq_val(i, BIT(16));
	}
}

/**
 * Register IRQ with MVIC.
 *
 * @param irq IRQ to register.
 */
static __inline__ void mvic_register_irq(uint32_t irq)
{
	/* Set IRQ triggering scheme and unmask the line. */

	switch (irq) {
	case QM_IRQ_RTC_0_INT:
	case QM_IRQ_AONPT_0_INT:
	case QM_IRQ_PIC_TIMER:
	case QM_IRQ_WDT_0_INT:
		/* positive edge */
		_mvic_set_irq_val(irq, 0);
		break;
	default:
		/* high level */
		_mvic_set_irq_val(irq, BIT(15));
		break;
	}
}

/**
 * Unmask IRQ with MVIC.
 *
 * @param irq IRQ to unmask.
 */
static __inline__ void mvic_unmask_irq(uint32_t irq)
{
	uint32_t value = _mvic_get_irq_val(irq);

	value &= ~BIT(16);

	_mvic_set_irq_val(irq, value);
}

/**
 * Mask IRQ with MVIC.
 *
 * @param irq IRQ to mask.
 */
static __inline__ void mvic_mask_irq(uint32_t irq)
{
	uint32_t value = _mvic_get_irq_val(irq);

	value |= BIT(16);

	_mvic_set_irq_val(irq, value);
}

#endif /* __MVIC_H__ */
