/*
 * Copyright (c) 2017 Jean-Paul Etienne <fractalclone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief interrupt management code for riscv SOCs supporting the riscv
	  privileged architecture specification
 */
#include <zephyr/irq.h>

void arch_irq_enable(unsigned int irq)
{
	if (IS_ENABLED(CONFIG_HAS_ITE_INTC)) {
		ite_intc_irq_enable(irq);
	}
}

void arch_irq_disable(unsigned int irq)
{
	if (IS_ENABLED(CONFIG_HAS_ITE_INTC)) {
		ite_intc_irq_disable(irq);
	}
};

int arch_irq_is_enabled(unsigned int irq)
{
	/*
	 * Return true from arch_irq_is_enabled() when external interrupt-enable
	 * bit, and SOC's IER are both true.
	 */
	if (IS_ENABLED(CONFIG_HAS_ITE_INTC)) {
		return ((csr_read(mie) & BIT(IRQ_M_EXT)) &&
			ite_intc_irq_is_enable(irq));
	} else {
		return 0;
	}
}
