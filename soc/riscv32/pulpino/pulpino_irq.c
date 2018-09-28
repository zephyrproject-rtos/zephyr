/*
 * Copyright (c) 2016 Jean-Paul Etienne <fractalclone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief pulpino interrupt management code
 */
#include <irq.h>
#include <soc.h>

void _arch_irq_enable(unsigned int irq)
{
	unsigned int key;

	key = irq_lock();
	/*
	 * enable both IRQ and Event
	 * Event will allow system to wakeup upon an interrupt,
	 * if CPU was set to sleep
	 */
	PULP_IER |= (1 << irq);
	PULP_EER |= (1 << irq);
	irq_unlock(key);
};

void _arch_irq_disable(unsigned int irq)
{
	unsigned int key;

	key = irq_lock();
	PULP_IER &= ~(1 << irq);
	PULP_EER &= ~(1 << irq);
	irq_unlock(key);
};

int _arch_irq_is_enabled(unsigned int irq)
{
	return !!(PULP_IER & (1 << irq));
}

#if defined(CONFIG_RISCV_SOC_INTERRUPT_INIT)
void soc_interrupt_init(void)
{
	/* ensure that all interrupts are disabled */
	(void)irq_lock();
	PULP_IER = 0;
	PULP_EER = 0;
}
#endif
