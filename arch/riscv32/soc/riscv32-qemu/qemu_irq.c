/*
 * Copyright (c) 2016 Jean-Paul Etienne <fractalclone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief riscv32-qemu interrupt management code
 */
#include <irq.h>
#include <soc.h>

void _arch_irq_enable(unsigned int irq)
{
	uint32_t mie;

	/*
	 * Since only internal Timer device has interrupt within in
	 * riscv32-qemu, use only mie CSR register to enable device interrupt.
	 * CSR mie register is updated using atomic instruction csrrs
	 * (atomic read and set bits in CSR register)
	 */
	__asm__ volatile ("csrrs %0, mie, %1\n"
			  : "=r" (mie)
			  : "r" (1 << irq));
}

void _arch_irq_disable(unsigned int irq)
{
	uint32_t mie;

	/*
	 * Use atomic instruction csrrc to disable device interrupt in mie CSR.
	 * (atomic read and clear bits in CSR register)
	 */
	__asm__ volatile ("csrrc %0, mie, %1\n"
			  : "=r" (mie)
			  : "r" (1 << irq));
};

int _arch_irq_is_enabled(unsigned int irq)
{
	uint32_t mie;

	__asm__ volatile ("csrr %0, mie" : "=r" (mie));

	return !!(mie & (1 << irq));
}

#if defined(CONFIG_RISCV_SOC_INTERRUPT_INIT)
void soc_interrupt_init(void)
{
	/* ensure that all interrupts are disabled */
	(void)irq_lock();

	__asm__ volatile ("csrwi mie, 0\n"
			  "csrwi sie, 0\n"
			  "csrwi mip, 0\n"
			  "csrwi sip, 0\n");
}
#endif
