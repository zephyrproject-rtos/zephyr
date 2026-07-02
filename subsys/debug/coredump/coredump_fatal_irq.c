/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel_arch_interface.h>
#include <zephyr/kernel.h>
#include <zephyr/toolchain.h>

#ifdef CONFIG_DEBUG_COREDUMP_FATAL_UNLOCK_IRQS

__weak void arch_coredump_fatal_irq_unlock(unsigned int key, void *cookie)
{
	ARG_UNUSED(cookie);

	arch_irq_unlock(key);
}

__weak unsigned int arch_coredump_fatal_irq_lock(void *cookie)
{
	ARG_UNUSED(cookie);

	return arch_irq_lock();
}

#endif /* CONFIG_DEBUG_COREDUMP_FATAL_UNLOCK_IRQS */
