/*
 * Copyright (c) 2025 Andrew Featherstone
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT hazard3_hazard3_intc

#include <zephyr/kernel.h>
#include <zephyr/drivers/interrupt_controller/intc.h>
#include <zephyr/drivers/interrupt_controller/intc_hazard3.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/irq.h>
#include <zephyr/device.h>
#include <zephyr/types.h>
#include <zephyr/arch/riscv/csr.h>
#include <zephyr/arch/riscv/irq.h>

#include <pico/runtime_init.h>
#include <hardware/irq.h>

#define CSR_WINDOW_SIZE 16

void intc_hazard3_irq_enable(unsigned int irq)
{
	irq_set_enabled(irq, true);
}

void intc_hazard3_irq_disable(unsigned int irq)
{
	irq_set_enabled(irq, false);
}

int intc_hazard3_irq_is_enabled(unsigned int irq)
{
	return pico_irq_is_enabled(irq);
}


static void intc_hazard3_enable(const struct device *dev, unsigned int irq)
{
	ARG_UNUSED(dev);
	intc_hazard3_irq_enable(irq);
}

static void intc_hazard3_disable(const struct device *dev, unsigned int irq)
{
	ARG_UNUSED(dev);
	intc_hazard3_irq_disable(irq);
}

static int intc_hazard3_is_enabled(const struct device *dev, unsigned int irq)
{
	ARG_UNUSED(dev);
	return intc_hazard3_irq_is_enabled(irq);
}

static void intc_hazard3_priority_set(const struct device *dev, unsigned int irq,
				      unsigned int prio, uint32_t flags)
{
	/* Priorities are configured through the RISC-V arch hooks, not here */
	ARG_UNUSED(dev);
	ARG_UNUSED(irq);
	ARG_UNUSED(prio);
	ARG_UNUSED(flags);
}

static DEVICE_API(intc, intc_hazard3_api) = {
	.enable = intc_hazard3_enable,
	.disable = intc_hazard3_disable,
	.is_enabled = intc_hazard3_is_enabled,
	.priority_set = intc_hazard3_priority_set,
};

INTC_ROOT_DEVICE_DEFINE(DT_DRV_INST(0));

static int hazard3_irq_init(const struct device *dev)
{
	/* Clear all IRQ force array bits. */
	for (int i = 0; (i * CSR_WINDOW_SIZE) < CONFIG_NUM_IRQS; i++) {
		hazard3_irqarray_clear(RVCSR_MEIFA_OFFSET, i, -1);
	}

	/* Global external IRQ enable. */
	csr_write(mie, RVCSR_MIE_MEIE_BITS);

	return 0;
}

DEVICE_DT_INST_DEFINE(0, hazard3_irq_init, NULL, NULL, NULL,
		      PRE_KERNEL_1, CONFIG_INTC_INIT_PRIORITY, &intc_hazard3_api);
