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
}

void arch_irq_enable(unsigned int irq)
{
	volatile uint32_t *int_enable_reg = (uint32_t *)DT_PROP(DT_NODELABEL(intc0), ie_offset);

	unsigned int reg_index = irq / (sizeof(uint32_t) << 3);
	unsigned int bit_pos = irq % (sizeof(uint32_t) << 3);
	/* Enable the interrupt by setting it's bit in interrupt enable register*/
	int_enable_reg[reg_index] |= (uint32_t)(1u << bit_pos);
}

int arch_irq_is_enabled(unsigned int irq)
{
	volatile uint32_t *int_enable_reg = (uint32_t *)DT_PROP(DT_NODELABEL(intc0), ie_offset);

	unsigned int reg_index = irq / (sizeof(uint32_t) << 3);
	unsigned int bit_pos = irq % (sizeof(uint32_t) << 3);

	return ((int_enable_reg[reg_index] >> bit_pos) & 0x1u);
}

void arch_irq_disable(unsigned int irq)
{
	volatile uint32_t *int_enable_reg = (uint32_t *)DT_PROP(DT_NODELABEL(intc0), ie_offset);

	unsigned int reg_index = irq / (sizeof(uint32_t) << 3);
	unsigned int bit_pos = irq % (sizeof(uint32_t) << 3);

	/* Disable the interrupt by clearing it's bit in interrupt enable register*/
	int_enable_reg[reg_index] &= (uint32_t)(~(1u << bit_pos));
}

bool arch_dspic_irq_isset(unsigned int irq)
{
	volatile uint32_t *int_ifs_reg = (uint32_t *)DT_PROP(DT_NODELABEL(intc0), if_offset);
	volatile int ret_ifs = false;
	unsigned int reg_index = irq / (sizeof(uint32_t) << 3);
	unsigned int bit_pos = irq % (sizeof(uint32_t) << 3);

	if ( (int_ifs_reg[reg_index] & (1u << bit_pos)) != 0u ) {
		ret_ifs = true;
	}
	return ret_ifs;
}

void z_dspic_enter_irq(int irq)
{
	volatile uint32_t *int_ifs_reg = (uint32_t *)DT_PROP(DT_NODELABEL(intc0), if_offset);

	unsigned int reg_index = (unsigned int)irq / (sizeof(uint32_t) << 3);
	unsigned int bit_pos = (unsigned int)irq % (sizeof(uint32_t) << 3);

	/* Enable the interrupt by setting it's bit in interrupt enable register*/
	int_ifs_reg[reg_index] |= (uint32_t)(1u << bit_pos);
}

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */
