/*
 * Copyright (c) 2024 Michael Hope
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT wch_pfic

#include <ch32fun.h>

#include <zephyr/arch/cpu.h>
#include <zephyr/init.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>

#define SEVONPEND (1 << 4)
#define WFITOWFE  (1 << 3)

void arch_irq_enable(unsigned int irq)
{
	PFIC->IENR[irq / 32] = 1 << (irq % 32);
}

void arch_irq_disable(unsigned int irq)
{
	PFIC->IRER[irq / 32] |= 1 << (irq % 32);
}

int arch_irq_is_enabled(unsigned int irq)
{
	return ((PFIC->ISR[irq >> 5] & (1 << (irq & 0x1F))) != 0);
}

static int pfic_init(void)
{
	/* `wfi` is called with interrupts disabled. Configure the PFIC to wake up on any event,
	 * including any interrupt.
	 */
	PFIC->SCTLR = SEVONPEND | WFITOWFE;
	return 0;
}

SYS_INIT(pfic_init, PRE_KERNEL_1, CONFIG_INTC_INIT_PRIORITY);
