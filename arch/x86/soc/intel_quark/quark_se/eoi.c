/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
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

#include <kernel.h>
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

