/*
 * Copyright (c) 2021-2024 Gerson Fernando Budke <nandojve@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief interrupt management code for riscv SOCs supporting the SiFive clic
 */
#include <zephyr/irq.h>
#include <soc.h>
#include <clic.h>

void riscv_clic_irq_enable(unsigned int irq)
{
	*(volatile uint8_t *)(CLIC_HART0_ADDR + CLIC_INTIE + irq) = 1;
}

void riscv_clic_irq_disable(unsigned int irq)
{
	*(volatile uint8_t *)(CLIC_HART0_ADDR + CLIC_INTIE + irq) = 0;
}

void riscv_clic_irq_priority_set(unsigned int irq, unsigned int prio, uint32_t flags)
{
	ARG_UNUSED(irq);
	ARG_UNUSED(prio);
	ARG_UNUSED(flags);
}

int riscv_clic_irq_is_enabled(unsigned int irq)
{
	return *(volatile uint8_t *)(CLIC_HART0_ADDR + CLIC_INTIE + irq);
}
