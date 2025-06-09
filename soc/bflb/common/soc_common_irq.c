/*
 * Copyright (c) 2021-2025 Gerson Fernando Budke <nandojve@gmail.com>
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

/* clang-format off */

static void clic_irq_enable(unsigned int irq)
{
	*(volatile uint8_t *)(CLIC_HART0_ADDR + CLIC_INTIE + irq) = 1;
}

static void clic_irq_disable(unsigned int irq)
{
	*(volatile uint8_t *)(CLIC_HART0_ADDR + CLIC_INTIE + irq) = 0;
}

void arch_irq_enable(unsigned int irq)
{
	uint32_t mie;

	if (irq == SOC_IRQ_MSOFT) {

		/* Read mstatus & set machine software interrupt enable in mie */

		__asm__ volatile("csrrs %0, mie, %1"
				 : "=r"(mie)
				 : "r"(BIT(RISCV_MACHINE_SOFT_IRQ)));

	} else if (irq == SOC_IRQ_MTIMER) {
		*(volatile uint8_t *)CLIC_TIMER_ENABLE_ADDRESS = 1;

		/* Read mstatus & set machine timer interrupt enable in mie */
		__asm__ volatile("csrrs %0, mie, %1"
				 : "=r"(mie)
				 : "r"(BIT(RISCV_MACHINE_TIMER_IRQ)
				     | BIT(RISCV_MACHINE_EXT_IRQ)));
	} else {
		clic_irq_enable(irq - SOC_IRQ_ASYNC);
	}
}

void arch_irq_disable(unsigned int irq)
{
	uint32_t mie;

	if (irq == SOC_IRQ_MSOFT) {

		/* Read mstatus & set machine software interrupt enable in mie */

		__asm__ volatile("csrrc %0, mie, %1"
				 : "=r"(mie)
				 : "r"(BIT(RISCV_MACHINE_SOFT_IRQ)));

	} else if (irq == SOC_IRQ_MTIMER) {
		*(volatile uint8_t *)CLIC_TIMER_ENABLE_ADDRESS = 0;

		/* Read mstatus & set machine timer interrupt enable in mie */
		__asm__ volatile("csrrc %0, mie, %1"
				 : "=r"(mie)
				 : "r"(BIT(RISCV_MACHINE_TIMER_IRQ)
				     | BIT(RISCV_MACHINE_EXT_IRQ)));
	} else {
		clic_irq_disable(irq - SOC_IRQ_ASYNC);
	}
}

void arch_irq_priority_set(unsigned int irq, unsigned int prio)
{
	ARG_UNUSED(irq);
	ARG_UNUSED(prio);
}

int arch_irq_is_enabled(unsigned int irq)
{
	uint32_t mie;

	/* Enable MEIE (machine external interrupt enable) */
	__asm__ volatile("csrrs %0, mie, %1"
			 : "=r"(mie)
			 : "r"(BIT(RISCV_MACHINE_EXT_IRQ)));

	/* Read mstatus & set machine interrupt enable (MIE) in mstatus */
	__asm__ volatile("csrrs %0, mstatus, %1"
			 : "=r"(mie)
			 : "r"(MSTATUS_MIE));

	return !!(mie & SOC_MIE_MSIE);
}

/* clang-format on */
