/*
 * Copyright (c) 2015 Intel Corporation
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
 * @brief Code to compensate for Lakemont EOI forwarding bug
 *
 * Lakemont CPU on Quark SE has a bug where LOAPIC EOI does not
 * correctly forward EOI to the IOAPIC, causing the IRR bit in the RTE
 * to never get cleared. We need to set the IOAPIC EOI register manually
 * with the vector of the interrupt.
 */

#include <nanokernel.h>
#include <arch/x86/irq_controller.h>
#include <sys_io.h>
#include <interrupt_controller/ioapic_priv.h>

void _lakemont_eoi(void)
{
	/* It is difficult to know whether the IRQ being serviced is
	 * a level interrupt handled by the IOAPIC; the only information
	 * we have is the vector # in the IDT. So unconditionally
	 * write to IOAPIC_EOI for every interrupt
	 */
	sys_write32(_irq_controller_isr_vector_get(),
		    CONFIG_IOAPIC_BASE_ADDRESS + IOAPIC_EOI);

	/* Send EOI to the LOAPIC as well */
	sys_write32(0, CONFIG_LOAPIC_BASE_ADDRESS + LOAPIC_EOI);
}

