/*
 * Copyright (c) 2025 NVIDIA Corporation <jholdsworth@nvidia.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/irq_offload.h>
#include <zephyr/arch/openrisc/syscall.h>

void arch_irq_offload(irq_offload_routine_t routine, const void *parameter)
{
	arch_syscall_invoke2((uintptr_t)routine, (uintptr_t)parameter, OR_SYSCALL_IRQ_OFFLOAD);
}

void arch_irq_offload_init(void)
{
}
