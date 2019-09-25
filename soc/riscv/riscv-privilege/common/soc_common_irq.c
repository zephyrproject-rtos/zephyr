/*
 * Copyright (c) 2017 Jean-Paul Etienne <fractalclone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief interrupt management code for riscv SOCs supporting the riscv
	  privileged architecture specification
 */
#include <irq.h>

/**
 * @brief Get an IRQ's level
 * @param irq The IRQ number in the Zephyr irq.h numbering system
 * @return IRQ level, either 1 or 2
 */
static inline unsigned int _irq_level(unsigned int irq)
{
	return ((irq >> 8) & 0xff) == 0U ? 1 : 2;
}

static inline unsigned int _level2_irq(unsigned int irq)
{
	return (irq >> 8) - 1;
}

void z_arch_irq_enable(unsigned int irq)
{
	u32_t mie;

#if defined(CONFIG_RISCV_HAS_PLIC)
	unsigned int level = _irq_level(irq);

	if (level == 2) {
		irq = _level2_irq(irq);
		riscv_plic_irq_enable(irq);
		return;
	}
#endif

	/*
	 * CSR mie register is updated using atomic instruction csrrs
	 * (atomic read and set bits in CSR register)
	 */
	__asm__ volatile ("csrrs %0, mie, %1\n"
			  : "=r" (mie)
			  : "r" (1 << irq));
}

void z_arch_irq_disable(unsigned int irq)
{
	u32_t mie;

#if defined(CONFIG_RISCV_HAS_PLIC)
	unsigned int level = _irq_level(irq);

	if (level == 2) {
		irq = _level2_irq(irq);
		riscv_plic_irq_disable(irq);
		return;
	}
#endif

	/*
	 * Use atomic instruction csrrc to disable device interrupt in mie CSR.
	 * (atomic read and clear bits in CSR register)
	 */
	__asm__ volatile ("csrrc %0, mie, %1\n"
			  : "=r" (mie)
			  : "r" (1 << irq));
};

void z_arch_irq_priority_set(unsigned int irq, unsigned int prio)
{
#if defined(CONFIG_RISCV_HAS_PLIC)
	unsigned int level = _irq_level(irq);

	if (level == 2) {
		irq = _level2_irq(irq);
		riscv_plic_set_priority(irq, prio);
	}
#endif

	return ;
}

int z_arch_irq_is_enabled(unsigned int irq)
{
	u32_t mie;

#if defined(CONFIG_RISCV_HAS_PLIC)
	unsigned int level = _irq_level(irq);

	if (level == 2) {
		irq = _level2_irq(irq);
		return riscv_plic_irq_is_enabled(irq);
	}
#endif

	__asm__ volatile ("csrr %0, mie" : "=r" (mie));

	return !!(mie & (1 << irq));
}

#if defined(CONFIG_RISCV_SOC_INTERRUPT_INIT)
void soc_interrupt_init(void)
{
	/* ensure that all interrupts are disabled */
	(void)irq_lock();

	__asm__ volatile ("csrwi mie, 0\n"
			  "csrwi mip, 0\n");
}
#endif
