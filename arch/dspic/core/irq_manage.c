/*
 * Copyright (c) 2025, Microchip Technology Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <kswap.h>

#ifndef _ASMLANGUAGE
#include <xc.h>

#ifdef __cplusplus
extern "C" {
#endif

void z_irq_spurious(const void *unused)
{
	ARG_UNUSED(unused);
	while (1) {
	}
	return;
}

void arch_irq_enable(unsigned int irq)
{
	volatile uint32_t *int_enable_reg[] = {&IEC0, &IEC1, &IEC2, &IEC3, &IEC4,
					       &IEC5, &IEC6, &IEC7, &IEC8};

	unsigned int reg_index = irq / (sizeof(uint32_t) << 3);
	unsigned int bit_pos = irq % (sizeof(uint32_t) << 3);

	/* Enable the interrupt by setting it's bit in interrupt enable register*/
	*int_enable_reg[reg_index] |= (uint32_t)(1u << bit_pos);

	return;
}

int arch_irq_is_enabled(unsigned int irq)
{
	volatile uint32_t *int_enable_reg[] = {&IEC0, &IEC1, &IEC2, &IEC3, &IEC4,
					       &IEC5, &IEC6, &IEC7, &IEC8};

	unsigned int reg_index = irq / (sizeof(uint32_t) << 3);
	unsigned int bit_pos = irq % (sizeof(uint32_t) << 3);

	return ((*int_enable_reg[reg_index] >> bit_pos) & 0x1u);
}

void arch_irq_disable(unsigned int irq)
{
	volatile uint32_t *int_enable_reg[] = {&IEC0, &IEC1, &IEC2, &IEC3, &IEC4,
					       &IEC5, &IEC6, &IEC7, &IEC8};

	unsigned int reg_index = irq / (sizeof(uint32_t) << 3);
	unsigned int bit_pos = irq % (sizeof(uint32_t) << 3);

	/* Disable the interrupt by clearing it's bit in interrupt enable register*/
	*int_enable_reg[reg_index] &= (uint32_t)(~(1u << bit_pos));

	return;
}

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */
