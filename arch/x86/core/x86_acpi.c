/*
 * Copyright (c) 2023, Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/acpi/acpi.h>
#include <zephyr/dt-bindings/interrupt-controller/intel-ioapic.h>

uint32_t arch_acpi_encode_irq_flags(uint8_t polarity, uint8_t trigger)
{
	uint32_t irq_flag = IRQ_DELIVERY_LOWEST;

	if (trigger == ACPI_LEVEL_SENSITIVE) {
		irq_flag |= IRQ_TYPE_LEVEL;
	} else {
		irq_flag |= IRQ_TYPE_EDGE;
	}

	if (polarity == ACPI_ACTIVE_HIGH) {
		irq_flag |= IRQ_TYPE_HIGH;
	} else if (polarity == ACPI_ACTIVE_LOW) {
		irq_flag |= IRQ_TYPE_LOW;
	}

	return irq_flag;
}
