/*
 * Copyright (c) 2016 Jean-Paul Etienne <fractalclone@gmail.com>
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
 * @brief pulpino interrupt management code
 */
#include <irq.h>
#include <soc.h>

void _arch_irq_enable(unsigned int irq)
{
	int key;

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
	int key;

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
