/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/irq_offload.h>
#include <zephyr/toolchain.h>
#include <kernel_arch_data.h>

void arch_irq_offload(irq_offload_routine_t routine, const void *parameter)
{
	register irq_offload_routine_t r6 __asm__("r6") = routine;
	register const void *r7 __asm__("r7") = parameter;

	__asm__ volatile("trap " STRINGIFY(RH850_TRAP_IRQ_OFFLOAD) : : "r"(r6), "r"(r7) : "memory");
}

void arch_irq_offload_init(void)
{
}
