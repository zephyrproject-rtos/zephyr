/*
 * Copyright (c) 2023 Intel corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file IRQ offload - x8664 implementation
 */

#include <zephyr/kernel.h>
#include <zephyr/irq_offload.h>
#include <kernel_arch_data.h>

#define NR_IRQ_VECTORS (IV_NR_VECTORS - IV_IRQS)  /* # vectors free for IRQs */

extern void (*x86_irq_funcs[NR_IRQ_VECTORS])(const void *arg);
extern const void *x86_irq_args[NR_IRQ_VECTORS];


void arch_irq_offload(irq_offload_routine_t routine, const void *parameter)
{
	x86_irq_funcs[CONFIG_IRQ_OFFLOAD_VECTOR - IV_IRQS] = routine;
	x86_irq_args[CONFIG_IRQ_OFFLOAD_VECTOR - IV_IRQS] = parameter;
	__asm__ volatile("int %0" : : "i" (CONFIG_IRQ_OFFLOAD_VECTOR)
			  : "memory");
	x86_irq_funcs[CONFIG_IRQ_OFFLOAD_VECTOR - IV_IRQS] = NULL;
}
