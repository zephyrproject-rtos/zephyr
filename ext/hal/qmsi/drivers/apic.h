/*
 * Copyright (c) 2016, Intel Corporation
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

#ifndef __APIC_H__
#define __APIC_H__

#include <stdint.h>

#include "qm_common.h"
#include "qm_soc_regs.h"

#define LAPIC_VECTOR_MASK (0xFF)

static void _ioapic_set_redtbl_entry(unsigned int irq, uint64_t value)
{
	unsigned int offset = QM_IOAPIC_REG_REDTBL + (irq * 2);

	QM_IOAPIC->ioregsel.reg = offset;
	QM_IOAPIC->iowin.reg = value & 0x00000000FFFFFFFF;
	QM_IOAPIC->ioregsel.reg = offset + 1;
	QM_IOAPIC->iowin.reg = (value & 0xFFFFFFFF00000000) >> 32;
}

/* Get redirection table size */
static __inline__ int _ioapic_get_redtbl_size(void)
{
	int max_entry_number;

	QM_IOAPIC->ioregsel.reg = QM_IOAPIC_REG_VER;
	max_entry_number = (QM_IOAPIC->iowin.reg & 0x00FF0000) >> 16;

	return max_entry_number + 1;
}

static uint32_t _ioapic_get_redtbl_entry_lo(unsigned int irq)
{
	QM_IOAPIC->ioregsel.reg = QM_IOAPIC_REG_REDTBL + (irq * 2);
	return QM_IOAPIC->iowin.reg;
}

static void _ioapic_set_redtbl_entry_lo(unsigned int irq, uint32_t value)
{
	QM_IOAPIC->ioregsel.reg = QM_IOAPIC_REG_REDTBL + (irq * 2);
	QM_IOAPIC->iowin.reg = value;
}

/*
 * Initialize Local and IOAPIC
 */
static __inline__ void apic_init(void)
{
	int i;
	int size;

	/* Enable LAPIC */
	QM_LAPIC->svr.reg |= BIT(8);

	/* Set up LVT LINT0 to ExtINT and unmask it */
	QM_LAPIC->lvtlint0.reg |= (BIT(8) | BIT(9) | BIT(10));
	QM_LAPIC->lvtlint0.reg &= ~BIT(16);

	/* Clear up any spurious LAPIC interrupts */
	QM_LAPIC->eoi.reg = 0;

	/* Setup IOAPIC Redirection Table */
	size = _ioapic_get_redtbl_size();
	for (i = 0; i < size; i++) {
		_ioapic_set_redtbl_entry(i, BIT(16));
	}
}

static __inline__ void ioapic_register_irq(unsigned int irq,
					   unsigned int vector)
{
	uint32_t value;

	value = _ioapic_get_redtbl_entry_lo(irq);

	/* Assign vector and set polarity (positive). */
	value &= ~LAPIC_VECTOR_MASK;
	value |= (vector & LAPIC_VECTOR_MASK);
	value &= ~BIT(13);

	/* Set trigger mode. */
	switch (irq) {
	case QM_IRQ_RTC_0:
	case QM_IRQ_AONPT_0:
	case QM_IRQ_WDT_0:
		/* Edge sensitive. */
		value &= ~BIT(15);
		break;
	default:
		/* Level sensitive. */
		value |= BIT(15);
		break;
	}

	_ioapic_set_redtbl_entry_lo(irq, value);
}

static __inline__ void ioapic_mask_irq(unsigned int irq)
{
	uint32_t value = _ioapic_get_redtbl_entry_lo(irq);

	value |= BIT(16);

	_ioapic_set_redtbl_entry_lo(irq, value);
}

static __inline__ void ioapic_unmask_irq(unsigned int irq)
{
	uint32_t value = _ioapic_get_redtbl_entry_lo(irq);

	value &= ~BIT(16);

	_ioapic_set_redtbl_entry_lo(irq, value);
}

#endif /* __APIC_H__ */
