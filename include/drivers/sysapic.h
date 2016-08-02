/*
 * Copyright (c) 2016 Intel Corporation
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

#ifndef __INC_SYS_APIC_H
#define __INC_SYS_APIC_H

#include <drivers/ioapic.h>
#include <drivers/loapic.h>

#define _IRQ_TRIGGER_EDGE	IOAPIC_EDGE
#define _IRQ_TRIGGER_LEVEL	IOAPIC_LEVEL

#define _IRQ_POLARITY_HIGH	IOAPIC_HIGH
#define _IRQ_POLARITY_LOW	IOAPIC_LOW

#ifndef _ASMLANGUAGE

#define LOAPIC_IRQ_BASE  CONFIG_IOAPIC_NUM_RTES
#define LOAPIC_IRQ_COUNT 6  /* Default to LOAPIC_TIMER to LOAPIC_ERROR */

/* irq_controller.h interface */
void __irq_controller_irq_config(unsigned int vector, unsigned int irq,
				 uint32_t flags);

int __irq_controller_isr_vector_get(void);

#else /* _ASMLANGUAGE */

#if CONFIG_EOI_FORWARDING_BUG
.macro __irq_controller_eoi
	call	_lakemont_eoi
.endm
#else
.macro __irq_controller_eoi
	xorl %eax, %eax			/* zeroes eax */
	loapic_eoi_reg = (CONFIG_LOAPIC_BASE_ADDRESS + LOAPIC_EOI)
	movl %eax, loapic_eoi_reg	/* tell LOAPIC the IRQ is handled */
.endm
#endif /* CONFIG_EOI_FORMWARDING_BUG */

#endif /* _ASMLANGUAGE */

#endif /* __INC_SYS_APIC_H */
