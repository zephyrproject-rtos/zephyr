/*
 * Copyright (c) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Note that SweRV SoC has an external interrupt controller by design.
 * The corresponding driver is enabled via CONFIG_SWERV_PIC.
 *
 * For debugging purpose and for running on simulator without support
 * for that controller, we can disable the driver to avoid invalid
 * memory access and use the internal interrupt controller for basic
 * functions.
 */

#include <stdint.h>

void arch_irq_enable(unsigned int irq)
{
	uint32_t mie;

	__asm__ volatile ("csrrs %0, mie, %1\n"
			  : "=r" (mie)
			  : "r" (1 << irq));
}

void arch_irq_disable(unsigned int irq)
{
	uint32_t mie;

	__asm__ volatile ("csrrc %0, mie, %1\n"
			  : "=r" (mie)
			  : "r" (1 << irq));
};

int arch_irq_is_enabled(unsigned int irq)
{
	uint32_t mie;

	__asm__ volatile ("csrr %0, mie" : "=r" (mie));

	return !!(mie & (1 << irq));
}
