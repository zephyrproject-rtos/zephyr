/*
 * Copyright (c) 2024 Michael Hope
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT wch_pfic

#include <hal_ch32fun.h>

#include <zephyr/arch/cpu.h>
#include <zephyr/init.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#define SEVONPEND BIT(4)
#define WFITOWFE  BIT(3)

void arch_irq_enable(unsigned int irq)
{
	PFIC->IENR[irq / 32] = BIT(irq % 32);
}

void arch_irq_disable(unsigned int irq)
{
	PFIC->IRER[irq / 32] = BIT(irq % 32);
}

int arch_irq_is_enabled(unsigned int irq)
{
	return ((PFIC->ISR[irq >> 5] & BIT(irq & 0x1F)) != 0);
}

static int pfic_init(void)
{
	/* `wfi` is called with interrupts disabled. Configure the PFIC to wake up on any event,
	 * including any interrupt.
	 *
	 * On the dual-core CH32H41x (AMP) each core boots its own Zephyr image, so this runs once
	 * per core. The assignment deliberately writes SCTLR to a fixed, known configuration
	 * (rather than OR-ing into it); every core writes the same value, so the result is
	 * well-defined regardless of which core runs first or how many times it runs.
	 */
	PFIC->SCTLR = SEVONPEND | WFITOWFE;
	return 0;
}

SYS_INIT(pfic_init, PRE_KERNEL_1, CONFIG_INTC_INIT_PRIORITY);
