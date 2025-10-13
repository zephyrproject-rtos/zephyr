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
#include <zephyr/irq_multilevel.h>

#include <zephyr/drivers/interrupt_controller/riscv_clic.h>
#include <zephyr/drivers/interrupt_controller/riscv_plic.h>
#include <zephyr/drivers/interrupt_controller/riscv_aia.h>

/* RISC-V architectural constant: local interrupts 0-15, external interrupts start at 16 */
#define RISCV_IRQ_EXT_OFFSET 16

#if defined(CONFIG_RISCV_HAS_CLIC)

void arch_irq_enable(unsigned int irq)
{
	riscv_clic_irq_enable(irq);
}

void arch_irq_disable(unsigned int irq)
{
	riscv_clic_irq_disable(irq);
}

int arch_irq_is_enabled(unsigned int irq)
{
	return riscv_clic_irq_is_enabled(irq);
}

void z_riscv_irq_priority_set(unsigned int irq, unsigned int prio, uint32_t flags)
{
	riscv_clic_irq_priority_set(irq, prio, flags);
}

void z_riscv_irq_vector_set(unsigned int irq)
{
#if defined(CONFIG_CLIC_SMCLICSHV_EXT)
	riscv_clic_irq_vector_set(irq);
#else
	ARG_UNUSED(irq);
#endif
}

#else /* PLIC + HLINT/CLINT or HLINT/CLINT only */

void arch_irq_enable(unsigned int irq)
{
	uint32_t mie;

#if defined(CONFIG_RISCV_HAS_PLIC)
	unsigned int level = irq_get_level(irq);

	if (level == 2) {
		riscv_plic_irq_enable(irq);
		return;
	}
#elif defined(CONFIG_RISCV_HAS_AIA)
	/*
	 * AIA uses raw EIID values, not multi-level encoding.
	 * IRQs < RISCV_IRQ_EXT_OFFSET are local interrupts (enabled via mie CSR),
	 * IRQs >= RISCV_IRQ_EXT_OFFSET are external interrupts (handled by AIA).
	 */
	if (irq >= RISCV_IRQ_EXT_OFFSET) {
		riscv_aia_irq_enable(irq);
		return;
	}
	/* Fall through to enable local interrupts in mie CSR */
#endif

	/*
	 * CSR mie register is updated using atomic instruction csrrs
	 * (atomic read and set bits in CSR register)
	 */
	mie = csr_read_set(mie, 1 << irq);
}

void arch_irq_disable(unsigned int irq)
{
	uint32_t mie;

#if defined(CONFIG_RISCV_HAS_PLIC)
	unsigned int level = irq_get_level(irq);

	if (level == 2) {
		riscv_plic_irq_disable(irq);
		return;
	}
#elif defined(CONFIG_RISCV_HAS_AIA)
	/* AIA: IRQs >= RISCV_IRQ_EXT_OFFSET are external, handled by AIA driver */
	if (irq >= RISCV_IRQ_EXT_OFFSET) {
		riscv_aia_irq_disable(irq);
		return;
	}
	/* Fall through for local interrupts */
#endif

	/*
	 * Use atomic instruction csrrc to disable device interrupt in mie CSR.
	 * (atomic read and clear bits in CSR register)
	 */
	mie = csr_read_clear(mie, 1 << irq);
}

int arch_irq_is_enabled(unsigned int irq)
{
	uint32_t mie;

#if defined(CONFIG_RISCV_HAS_PLIC)
	unsigned int level = irq_get_level(irq);

	if (level == 2) {
		return riscv_plic_irq_is_enabled(irq);
	}
#elif defined(CONFIG_RISCV_HAS_AIA)
	/* AIA: IRQs >= RISCV_IRQ_EXT_OFFSET are external, handled by AIA driver */
	if (irq >= RISCV_IRQ_EXT_OFFSET) {
		return riscv_aia_irq_is_enabled(irq);
	}
	/* Fall through for local interrupts */
#endif

	mie = csr_read(mie);

	return !!(mie & (1 << irq));
}

#if defined(CONFIG_RISCV_HAS_PLIC)
void z_riscv_irq_priority_set(unsigned int irq, unsigned int prio, uint32_t flags)
{
	unsigned int level = irq_get_level(irq);

	if (level == 2) {
		riscv_plic_set_priority(irq, prio);
	}
}
#elif defined(CONFIG_RISCV_HAS_AIA)
void z_riscv_irq_priority_set(unsigned int irq, unsigned int prio, uint32_t flags)
{
	ARG_UNUSED(flags);

	/* Local interrupts (< RISCV_IRQ_EXT_OFFSET) don't have configurable priority */
	if (irq < RISCV_IRQ_EXT_OFFSET) {
		return;
	}

	/* AIA priority is handled via IMSIC EITHRESHOLD or EIID ordering */
	riscv_aia_set_priority(irq, prio);
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
