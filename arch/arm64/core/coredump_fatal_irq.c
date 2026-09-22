/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel_arch_interface.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/kernel.h>

#ifdef CONFIG_DEBUG_COREDUMP_FATAL_UNLOCK_IRQS

void arch_coredump_fatal_irq_unlock(unsigned int key, void *cookie)
{
	unsigned int daif;

	ARG_UNUSED(key);

	__ASSERT_NO_MSG(cookie != NULL);

	daif = read_daif();
	*(unsigned int *)cookie = daif;
	write_daif(daif & ~DAIF_IRQ_BIT);
}

unsigned int arch_coredump_fatal_irq_lock(void *cookie)
{
	__ASSERT_NO_MSG(cookie != NULL);

	write_daif(*(unsigned int *)cookie);

	return arch_irq_lock();
}

#endif /* CONFIG_DEBUG_COREDUMP_FATAL_UNLOCK_IRQS */
