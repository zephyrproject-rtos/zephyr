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

void arch_irq_enable(unsigned int irq)
{
#if CONFIG_ITE_IT8XXX2_INTC
	if (irq > 0) {
		ite_intc_irq_enable(irq);
	}
#else
	uint32_t mie;

#if defined(CONFIG_RISCV_HAS_PLIC)
	if (irq > RISCV_MAX_GENERIC_IRQ) {
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
#endif /* CONFIG_ITE_IT8XXX2_INTC */
}

void arch_irq_disable(unsigned int irq)
{
#if CONFIG_ITE_IT8XXX2_INTC
	if (irq > 0) {
		ite_intc_irq_disable(irq);
	}
#else
	uint32_t mie;

#if defined(CONFIG_RISCV_HAS_PLIC)
	if (irq > RISCV_MAX_GENERIC_IRQ) {
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
#endif /* CONFIG_ITE_IT8XXX2_INTC */
};

int arch_irq_is_enabled(unsigned int irq)
{
	uint32_t mie;

#if defined(CONFIG_RISCV_HAS_PLIC)
	if (irq > RISCV_MAX_GENERIC_IRQ)
		return riscv_plic_irq_is_enabled(irq);
#endif

	__asm__ volatile ("csrr %0, mie" : "=r" (mie));

#if CONFIG_ITE_IT8XXX2_INTC
	return (mie && (ite_intc_irq_is_enable(irq)));
#else
	return !!(mie & (1 << irq));
#endif /* CONFIG_ITE_IT8XXX2_INTC */
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
