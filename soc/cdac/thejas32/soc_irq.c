/*
 * Copyright (c) 2017 Jean-Paul Etienne <fractalclone@gmail.com>
 * Copyright (c) 2026 Anuj Deshpande
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Interrupt management for the C-DAC THEJAS32 SoC
 *
 * Level-1 interrupts are the standard RISC-V machine-level interrupts and are
 * controlled through the mie CSR. Level-2 interrupts are the SoC peripheral
 * sources aggregated by the THEJAS32 interrupt controller and are routed to
 * its driver.
 */

#include <zephyr/irq.h>
#include <zephyr/irq_multilevel.h>
#include <zephyr/arch/riscv/csr.h>
#include <zephyr/drivers/interrupt_controller/intc_thejas32.h>

void arch_irq_enable(unsigned int irq)
{
	if (irq_get_level(irq) == 2) {
		thejas32_intc_irq_enable(irq);
		return;
	}

	csr_set(mie, BIT(irq));
}

void arch_irq_disable(unsigned int irq)
{
	if (irq_get_level(irq) == 2) {
		thejas32_intc_irq_disable(irq);
		return;
	}

	csr_clear(mie, BIT(irq));
}

int arch_irq_is_enabled(unsigned int irq)
{
	if (irq_get_level(irq) == 2) {
		return thejas32_intc_irq_is_enabled(irq);
	}

	return (csr_read(mie) & BIT(irq)) != 0;
}
