/*
 * Copyright (c) 2025 Andrew Featherstone
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT raspberrypi_hazard3_intc

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/irq.h>
#include <zephyr/device.h>
#include <zephyr/types.h>
#include <zephyr/arch/riscv/csr.h>
#include <zephyr/arch/riscv/irq.h>

#include <pico/runtime_init.h>
#include <hardware/irq.h>

void arch_irq_enable(unsigned int irq)
{
	irq_set_enabled(irq, true);
}

void arch_irq_disable(unsigned int irq)
{
	irq_set_enabled(irq, false);
}

int arch_irq_is_enabled(unsigned int irq)
{
	return pico_irq_is_enabled(irq);
}

static int hazard3_irq_init(const struct device *dev)
{
	/* Clear all IRQ force array bits. */
	for (int i = 0; i < 4; i++) {
		hazard3_irqarray_clear(RVCSR_MEIFA_OFFSET, i, -1);
	}

	/* Global external IRQ enable. */
	csr_write(mie, RVCSR_MIE_MEIE_BITS);

	csr_set(mstatus, MSTATUS_IEN);

	return 0;
}

DEVICE_DT_INST_DEFINE(0, hazard3_irq_init, NULL, NULL, NULL,
		      PRE_KERNEL_1, CONFIG_INTC_INIT_PRIORITY, NULL);
