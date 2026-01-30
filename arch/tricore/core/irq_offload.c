/*
 * Copyright (c) 2026 Infineon Technologies AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/irq_offload.h>
#include <zephyr/toolchain.h>
#include <zephyr/arch/tricore/syscall.h>

void arch_irq_offload(irq_offload_routine_t routine, const void *parameter)
{
	register void *a5 __asm__("a5") = routine;
	register void *a4 __asm__("a4") = (void *)parameter;

	__asm__ volatile("syscall " STRINGIFY(TRICORE_SYSCALL_IRQ_OFFLOAD)
					      :
					      : "r"(a5), "r"(a4)
					      : "memory");
}

void arch_irq_offload_init(void)
{
	/* No init needed: irq_offload is dispatched via syscall
	 * TRICORE_SYSCALL_IRQ_OFFLOAD; the trap/syscall vector is wired
	 * up by the architecture's boot-time vector setup.
	 */
}
