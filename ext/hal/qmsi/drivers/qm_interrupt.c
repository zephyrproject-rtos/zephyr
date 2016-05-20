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

#include "qm_common.h"
#include "qm_interrupt.h"
#include "idt.h"

#if (HAS_APIC)
#include "apic.h"

#elif(HAS_MVIC)
#include "mvic.h"

#elif(QM_SENSOR)
#include "qm_ss_interrupt.h"
#include "qm_sensor_regs.h"
extern qm_ss_isr_t __ivt_vect_table[];

#else
#error "Unsupported / unspecified processor detected."
#endif

/* SCSS base addr for LMT interrupt routing, for linear IRQ mapping */
#define SCSS_LMT_INT_MASK_BASE (&QM_SCSS_INT->int_i2c_mst_0_mask)

#if (QM_SENSOR)
#define SCSS_INT_MASK BIT(8) /* Sensor Subsystem interrupt masking */
static void ss_register_irq(unsigned int vector);
#else
#define SCSS_INT_MASK BIT(0) /* Lakemont interrupt masking */
#endif

void qm_irq_disable(void)
{
#if (QM_SENSOR)
	qm_ss_irq_disable();
#else
	__asm__ __volatile__("cli");
#endif
}

void qm_irq_enable(void)
{
#if (QM_SENSOR)
	qm_ss_irq_enable();
#else
	__asm__ __volatile__("sti");
#endif
}

void qm_irq_mask(uint32_t irq)
{
#if (HAS_APIC)
	ioapic_mask_irq(irq);

#elif(HAS_MVIC)
	mvic_mask_irq(irq);

#elif(QM_SENSOR)
	qm_ss_irq_mask(irq);

#endif
}

void qm_irq_unmask(uint32_t irq)
{
#if (HAS_APIC)
	ioapic_unmask_irq(irq);

#elif(HAS_MVIC)
	mvic_unmask_irq(irq);

#elif(QM_SENSOR)
	qm_ss_irq_unmask(irq);

#endif
}

void _qm_irq_setup(uint32_t irq, uint16_t register_offset)
{
	uint32_t *scss_intmask;

#if (HAS_APIC)
	/*
	 * Quark SE SOC has an APIC. Other SoCs uses a simple, fixed-vector
	 * non-8259 PIC that requires no configuration.
	 */
	ioapic_register_irq(irq, QM_IRQ_TO_VECTOR(irq));
#elif(HAS_MVIC)
	mvic_register_irq(irq);
#elif(QM_SENSOR)
	ss_register_irq(QM_IRQ_TO_VECTOR(irq));
#endif

	/* Route peripheral interrupt to Lakemont/Sensor Subsystem */
	scss_intmask = (uint32_t *)SCSS_LMT_INT_MASK_BASE + register_offset;

#if (QUARK_SE || QUARK_D2000 || QM_SENSOR)
	/* On Quark D2000 and Quark SE the register for the analog comparator
	 * host mask has a different bit field than the other host mask
	 * registers. */
	if (QM_IRQ_AC_MASK_OFFSET == register_offset) {
		*scss_intmask &= ~0x0007ffff;
#if !defined(QUARK_D2000)
	} else if (QM_IRQ_MBOX_MASK_OFFSET == register_offset) {
/* Masking MAILBOX irq id done inside mbox driver */
#endif
	} else {
		*scss_intmask &= ~SCSS_INT_MASK;
	}
#else
	*scss_intmask &= ~SCSS_INT_MASK;
#endif

#if (HAS_APIC)
	ioapic_unmask_irq(irq);
#elif(HAS_MVIC)
	mvic_unmask_irq(irq);
#elif(QM_SENSOR)
	qm_ss_irq_unmask(QM_IRQ_TO_VECTOR(irq));
#endif
}

/*
 * Register an Interrupt Service Routine to a given interrupt vector.
 *
 * @param[in] vector Interrupt Vector number.
 * @param[in] isr ISR to register to given vector. Must be a valid x86 ISR.
 *            If this can't be provided, qm_irq_request() or
 *            qm_int_vector_request() should be used instead.
 */
void _qm_register_isr(uint32_t vector, qm_isr_t isr)
{
#if (QM_SENSOR)
	__ivt_vect_table[vector] = isr;
#else
	idt_set_intr_gate_desc(vector, (uint32_t)isr);
#endif
}

#if (QM_SENSOR)
static void ss_register_irq(unsigned int vector)
{
	/*
	 * By hardware power-on default, SS interrupts are level triggered.
	 * The following switch statement sets some of the peripherals to edge
	 * triggered.
	 */
	switch (vector) {
	case QM_SS_IRQ_ADC_PWR_VECTOR:
	case QM_IRQ_RTC_0_VECTOR:
	case QM_IRQ_AONPT_0_VECTOR:
	case QM_IRQ_WDT_0_VECTOR:
		/* Edge sensitive. */
		__builtin_arc_sr(vector, QM_SS_AUX_IRQ_SELECT);
		__builtin_arc_sr(QM_SS_IRQ_EDGE_SENSITIVE,
				 QM_SS_AUX_IRQ_TRIGER);
	}
}
#endif
