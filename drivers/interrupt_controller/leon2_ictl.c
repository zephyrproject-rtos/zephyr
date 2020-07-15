/*
 * Copyright (c) 2018 ispace, inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <arch/cpu.h>
#include <init.h>
#include <soc.h>

#define MIN_IRQ_NUMBER (1)
#define MAX_IRQ_NUMBER (15)

void arch_irq_enable(uint32_t irq)
{
	uint32_t val;

	if ((irq < MIN_IRQ_NUMBER) || (irq > MAX_IRQ_NUMBER)) {
		printk("Invalid irq number: %d\n", irq);
		return;
	}

	/* get current irq mask value and add enable bit.*/
	val = sys_read32(LEON2_INTR_MASK);
	val |= (1 << irq);
	sys_write32(val, LEON2_INTR_MASK);
}

void arch_irq_disable(uint32_t irq)
{
	uint32_t val;

	if ((irq < MIN_IRQ_NUMBER) || (irq > MAX_IRQ_NUMBER)) {
		printk("Invalid irq number: %d\n", irq);
		return;
	}

	/* get current irq mask value and add enable bit.*/
	val = sys_read32(LEON2_INTR_MASK);
	val &= ~(1 << irq);
	sys_write32(val, LEON2_INTR_MASK);
}

static void _irq_disable_all(void)
{
	sys_write32(0xff, LEON2_INTR_MASK);
}

static int leon2_ictl_init(struct device *dev)
{
	ARG_UNUSED(dev);

	_irq_disable_all();

	return 0;
}

SYS_INIT(leon2_ictl_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
