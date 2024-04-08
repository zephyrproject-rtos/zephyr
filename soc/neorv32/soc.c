/*
 * Copyright (c) 2021 Henrik Brix Andersen <henrik@brixandersen.dk>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/irq.h>
#include <zephyr/irq_nextlevel.h>
#include <soc.h>

#if defined(CONFIG_RISCV_SOC_INTERRUPT_INIT)

static const struct device *xirq;

void arch_irq_enable(unsigned int irq)
{
	if (IS_ENABLED(CONFIG_MULTI_LEVEL_INTERRUPTS) && irq_get_level(irq) != 1U) {
		irq_enable_next_level(xirq, irq);
		return;
	}
	csr_set(mie, 1 << irq);
}

void arch_irq_disable(unsigned int irq)
{
	if (IS_ENABLED(CONFIG_MULTI_LEVEL_INTERRUPTS) && irq_get_level(irq) != 1U) {
		irq_disable_next_level(xirq, irq);
		return;
	}
	csr_clear(mie, 1 << irq);
}

int arch_irq_is_enabled(unsigned int irq)
{
	uint32_t mie;

	if (IS_ENABLED(CONFIG_MULTI_LEVEL_INTERRUPTS) && irq_get_level(irq) != 1U) {
		return irq_line_is_enabled_next_level(xirq, irq);
	}

	mie = csr_read(mie);

	return !!(mie & (1 << irq));
}

/*
 * SoC-level interrupt initialization. Clear any pending interrupts or
 * events, and find the XIRQ device if necessary.
 *
 * This gets called as almost the first thing z_cstart() does, so it
 * will happen before any calls to the _arch_irq_xxx() routines above.
 */
void soc_interrupt_init(void)
{
	/* ensure that all interrupts are disabled */
	(void)arch_irq_lock();

	csr_write(mie, 0);

	if (IS_ENABLED(CONFIG_MULTI_LEVEL_INTERRUPTS)) {
		xirq = DEVICE_DT_GET(DT_INST(0, neorv32_xirq));
	}
}

#endif
