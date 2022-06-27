/*
 * Copyright (c) 2021 Henrik Brix Andersen <henrik@brixandersen.dk>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/irq.h>
#include <soc.h>

#if defined(CONFIG_RISCV_SOC_INTERRUPT_INIT)
void soc_interrupt_init(void)
{
	(void)arch_irq_lock();

	__asm__ volatile ("csrwi mie, 0\n");
}
#endif
