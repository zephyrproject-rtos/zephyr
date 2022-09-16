/*
 * Copyright (c) 2022 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/irq_offload.h>
#include <zsr.h>

#define CURR_CPU (IS_ENABLED(CONFIG_SMP) ? arch_curr_cpu()->id : 0)

static struct {
	irq_offload_routine_t fn;
	const void *arg;
} offload_params[CONFIG_MP_NUM_CPUS];

static void irq_offload_isr(const void *param)
{
	ARG_UNUSED(param);
	offload_params[CURR_CPU].fn(offload_params[CURR_CPU].arg);
}

void arch_irq_offload(irq_offload_routine_t routine, const void *parameter)
{
	IRQ_CONNECT(ZSR_IRQ_OFFLOAD_INT, 0, irq_offload_isr, NULL, 0);

	unsigned int intenable, key = arch_irq_lock();

	offload_params[CURR_CPU].fn = routine;
	offload_params[CURR_CPU].arg = parameter;

	__asm__ volatile("rsr %0, INTENABLE" : "=r"(intenable));
	intenable |= BIT(ZSR_IRQ_OFFLOAD_INT);
	__asm__ volatile("wsr %0, INTENABLE; wsr %0, INTSET; rsync"
			 :: "r"(intenable), "r"(BIT(ZSR_IRQ_OFFLOAD_INT)));
	arch_irq_unlock(key);
}
