/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/riscv/csr.h>
#include <zephyr/irq.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util.h>
#include <soc.h>

extern void _isr_wrapper(void);

static void irq_priority_init(unsigned int irq)
{
	if (irq < 26) {
		return;
	}

	unsigned int group = (irq - 26) / 8;
	unsigned int shift = ((irq - 26) % 8) * 4;
	unsigned long mask = 0xfUL << shift;
	unsigned long priority = 1UL << shift;

#define SET_PRIORITY(csr) csr_write(csr, (csr_read(csr) & ~mask) | priority)
	switch (group) {
	case 0:
		SET_PRIORITY(HI3861_CSR_INTPRI0);
		break;
	case 1:
		SET_PRIORITY(HI3861_CSR_INTPRI1);
		break;
	case 2:
		SET_PRIORITY(HI3861_CSR_INTPRI2);
		break;
	case 3:
		SET_PRIORITY(HI3861_CSR_INTPRI3);
		break;
	case 4:
		SET_PRIORITY(HI3861_CSR_INTPRI4);
		break;
	}
#undef SET_PRIORITY
}

void arch_irq_enable(unsigned int irq)
{
	__ASSERT(irq < CONFIG_NUM_IRQS, "Invalid Hi3861 IRQ %u", irq);
	unsigned int key = irq_lock();

	irq_priority_init(irq);
	if (irq < 32) {
		csr_set(mie, BIT(irq));
	} else {
		csr_set(HI3861_CSR_LOCIE0, BIT(irq - 32));
	}
	irq_unlock(key);
}

void arch_irq_disable(unsigned int irq)
{
	__ASSERT(irq < CONFIG_NUM_IRQS, "Invalid Hi3861 IRQ %u", irq);
	if (irq < 32) {
		csr_clear(mie, BIT(irq));
	} else {
		csr_clear(HI3861_CSR_LOCIE0, BIT(irq - 32));
	}
}

int arch_irq_is_enabled(unsigned int irq)
{
	__ASSERT(irq < CONFIG_NUM_IRQS, "Invalid Hi3861 IRQ %u", irq);
	if (irq < 32) {
		return !!(csr_read(mie) & BIT(irq));
	}
	return !!(csr_read(HI3861_CSR_LOCIE0) & BIT(irq - 32));
}

void soc_interrupt_init(void)
{
	csr_write(mie, 0);
	csr_write(HI3861_CSR_LOCIE0, 0);
	csr_write(mtvec, (uintptr_t)_isr_wrapper);
}
