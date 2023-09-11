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
#include <zephyr/irq.h>

#include <zephyr/drivers/interrupt_controller/riscv_clic.h>
#include <zephyr/drivers/interrupt_controller/riscv_plic.h>

#if defined(CONFIG_RISCV_HAS_CLIC)

void arch_irq_intc_enable(unsigned int irq, const struct device *dev)
{
	ARG_UNUSED(dev);
	iscv_clic_irq_enable(irq);
}

void arch_irq_enable(unsigned int irq)
{
	arch_irq_intc_enable(irq, NULL);
}

void arch_irq_intc_disable(unsigned int irq, const struct device *dev)
{
	ARG_UNUSED(dev);
	riscv_clic_irq_disable(irq);
}

void arch_irq_disable(unsigned int irq)
{
	arch_irq_intc_disable(irq, NULL);
}

int arch_irq_intc_is_enabled(unsigned int irq, const struct device *dev)
{
	ARG_UNUSED(dev);
	return riscv_clic_irq_is_enabled(irq);
}

int arch_irq_is_enabled(unsigned int irq)
{
	return arch_irq_intc_is_enabled(irq, NULL);
}

void z_riscv_irq_intc_priority_set(unsigned int irq, unsigned int prio,
				   uint32_t flags,, const struct device *dev)
{
	ARG_UNUSED(dev);
	riscv_clic_irq_priority_set(irq, prio, flags);
}

void z_riscv_irq_priority_set(unsigned int irq, unsigned int prio, uint32_t flags)
{
	z_riscv_irq_intc_priority_set(irq, prio, flags, NULL);
}

#else /* PLIC + HLINT/CLINT or HLINT/CLINT only */

void arch_irq_intc_enable(unsigned int irq, const struct device *dev)
{
	uint32_t mie;

#if defined(CONFIG_RISCV_HAS_PLIC)
	unsigned int level = irq_get_level(irq);

	if (level == 2) {
		irq = irq_from_level_2(irq);
		riscv_plic_irq_enable(irq, dev);
		return;
	}
#endif

	/*
	 * CSR mie register is updated using atomic instruction csrrs
	 * (atomic read and set bits in CSR register)
	 */
	mie = csr_read_set(mie, 1 << irq);
}

void arch_irq_enable(unsigned int irq)
{
	arch_irq_intc_enable(irq, NULL);
}

void arch_irq_intc_disable(unsigned int irq, const struct device *dev)
{
	uint32_t mie;

#if defined(CONFIG_RISCV_HAS_PLIC)
	unsigned int level = irq_get_level(irq);

	if (level == 2) {
		irq = irq_from_level_2(irq);
		riscv_plic_irq_disable(irq, dev);
		return;
	}
#endif

	/*
	 * Use atomic instruction csrrc to disable device interrupt in mie CSR.
	 * (atomic read and clear bits in CSR register)
	 */
	mie = csr_read_clear(mie, 1 << irq);
}

void arch_irq_disable(unsigned int irq)
{
	arch_irq_intc_disable(irq, NULL);
}

int arch_irq_intc_is_enabled(unsigned int irq, const struct device *dev)
{
	uint32_t mie;

#if defined(CONFIG_RISCV_HAS_PLIC)
	unsigned int level = irq_get_level(irq);

	if (level == 2) {
		irq = irq_from_level_2(irq);
		return riscv_plic_irq_is_enabled(irq, dev);
	}
#endif

	mie = csr_read(mie);

	return !!(mie & (1 << irq));
}

int arch_irq_is_enabled(unsigned int irq)
{
	return arch_irq_intc_is_enabled(irq, NULL);
}

#if defined(CONFIG_RISCV_HAS_PLIC)
void z_riscv_irq_intc_priority_set(unsigned int irq, unsigned int prio,
				   uint32_t flags, const struct device *dev)
{
	unsigned int level = irq_get_level(irq);

	if (level == 2) {
		irq = irq_from_level_2(irq);
		riscv_plic_intc_set_priority(irq, prio, dev);
	}
}

void z_riscv_irq_priority_set(unsigned int irq, unsigned int prio, uint32_t flags)
{
	z_riscv_irq_intc_priority_set(irq, prio, flags, NULL);
}
#endif /* CONFIG_RISCV_HAS_PLIC */
#endif /* CONFIG_RISCV_HAS_CLIC */

#if defined(CONFIG_RISCV_SOC_INTERRUPT_INIT)
__weak void soc_interrupt_init(void)
{
	/* ensure that all interrupts are disabled */
	(void)arch_irq_lock();

	csr_write(mie, 0);
	csr_write(mip, 0);
}
#endif
