/*
 * Copyright (c) 2023 Intel corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file IRQ offload - x8664 implementation
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/irq_offload.h>
#include <kernel_arch_data.h>

#define NR_IRQ_VECTORS (IV_NR_VECTORS - IV_IRQS)  /* # vectors free for IRQs */

extern void (*x86_irq_funcs[NR_IRQ_VECTORS])(const void *arg);
extern const void *x86_irq_args[NR_IRQ_VECTORS];

static void (*irq_offload_funcs[CONFIG_MP_MAX_NUM_CPUS])(const void *arg);
static const void *irq_offload_args[CONFIG_MP_MAX_NUM_CPUS];

static void dispatcher(const void *arg)
{
	uint8_t cpu_id = _current_cpu->id;

	if (irq_offload_funcs[cpu_id] != NULL) {
		irq_offload_funcs[cpu_id](irq_offload_args[cpu_id]);
	}
}

void arch_irq_offload(irq_offload_routine_t routine, const void *parameter)
{
	int key = arch_irq_lock();
	uint8_t cpu_id = _current_cpu->id;

	irq_offload_funcs[cpu_id] = routine;
	irq_offload_args[cpu_id] = parameter;

	__asm__ volatile("int %0" : : "i" (CONFIG_IRQ_OFFLOAD_VECTOR)
			  : "memory");

	arch_irq_unlock(key);
}

void arch_irq_offload_init(void)
{
	x86_irq_funcs[CONFIG_IRQ_OFFLOAD_VECTOR - IV_IRQS] = dispatcher;
}
