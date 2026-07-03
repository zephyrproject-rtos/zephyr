/*
 * Copyright (c) 2026 Anuj Deshpande
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief C-DAC THEJAS32 SoC initialisation
 */

#include <zephyr/irq.h>
#include <zephyr/arch/riscv/csr.h>

void soc_interrupt_init(void)
{
	(void)arch_irq_lock();

	csr_write(mie, 0);
	csr_write(mip, 0);
}
