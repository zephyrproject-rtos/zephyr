/*
 * Copyright (c) 2015, Intel Corporation
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

#else
#error "Unsupported / unspecified processor detected."
#endif

/* SCSS base addr for LMT interrupt routing, for linear IRQ mapping */
#define SCSS_LMT_INT_MASK_BASE (&QM_SCSS_INT->int_i2c_mst_0_mask)

/* SCSS interrupt router: Lakemont delivery masking */
#define SCSS_LMT_INT_MASK BIT(0)

void qm_irq_disable(void)
{
	__asm__ __volatile__("cli");
}

void qm_irq_enable(void)
{
	__asm__ __volatile__("sti");
}

void qm_irq_mask(uint32_t irq)
{
#if (HAS_APIC)
	ioapic_mask_irq(irq);

#elif(HAS_MVIC)
	mvic_mask_irq(irq);

#else
#error "Unsupported / unspecified processor detected."
#endif
}

void qm_irq_unmask(uint32_t irq)
{
#if (HAS_APIC)
	ioapic_unmask_irq(irq);

#elif(HAS_MVIC)
	mvic_unmask_irq(irq);

#else
#error "Unsupported / unspecified processor detected."
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
#else
#error "Unsupported / unspecified interrupt controller detected."
#endif

	/* Route peripheral interrupt to Lakemont */
	scss_intmask = (uint32_t *)SCSS_LMT_INT_MASK_BASE + register_offset;

#if (QUARK_SE || QUARK_D2000)
	/* On Quark D2000 and SE the register for the analog comparator host
	 * mask has a different bit field than the other host mask registers.
	 */

	if (QM_IRQ_AC_MASK_OFFSET == register_offset) {
		*scss_intmask &= ~0x0007ffff;
	} else {
		*scss_intmask &= ~SCSS_LMT_INT_MASK;
	}
#else
	*scss_intmask &= ~SCSS_LMT_INT_MASK;
#endif

#if (HAS_APIC)
	ioapic_unmask_irq(irq);
#elif(HAS_MVIC)
	mvic_unmask_irq(irq);
#else
#error "Unsupported / unspecified interrupt controller detected."
#endif
}

void _qm_register_isr(uint32_t vector, qm_isr_t isr)
{
	idt_set_intr_gate_desc(vector, (uint32_t)isr);
}
