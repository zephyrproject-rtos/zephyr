/*
 * Copyright (c) 2022 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/irq_offload.h>
#include <zephyr/arch/riscv/syscall.h>

void arch_irq_offload(irq_offload_routine_t routine, const void *parameter)
{
	arch_syscall_invoke2((uintptr_t)routine, (uintptr_t)parameter, RV_ECALL_IRQ_OFFLOAD);
}
