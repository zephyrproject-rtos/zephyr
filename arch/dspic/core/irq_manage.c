/*
 * Copyright (c) 2025, Microchip Technology Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <kswap.h>

void z_irq_spurious(const void *unused)
{
	(void)unused;
}

void arch_irq_enable(unsigned int irq)
{
	(void)irq;
}

int arch_irq_is_enabled(unsigned int irq)
{
	(void)irq;
	return 0;
}

void arch_irq_disable(unsigned int irq)
{
	(void)irq;
}
