/*
 * Copyright (c) 2018,2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/irq_offload.h>
#include <zephyr/init.h>

#ifndef CONFIG_IRQ_OFFLOAD_NESTED
/* Per-CPU semaphores prevent irq_offload() from nesting on the same CPU. */
static struct k_sem offload_sem[CONFIG_MP_MAX_NUM_CPUS];

static int irq_offload_sem_init(void)
{
	for (int i = 0; i < CONFIG_MP_MAX_NUM_CPUS; i++) {
		k_sem_init(&offload_sem[i], 1, 1);
	}
	return 0;
}
SYS_INIT(irq_offload_sem_init, PRE_KERNEL_1, 0);

/*
 * Release the calling CPU's offload semaphore when irq_offload() cannot
 * return normally to release it.
 */
void irq_offload_sem_give(void)
{
	unsigned int id = arch_curr_cpu()->id;

	k_sem_give(&offload_sem[id]);
}
#endif /* !CONFIG_IRQ_OFFLOAD_NESTED */

void irq_offload(irq_offload_routine_t routine, const void *parameter)
{
#ifdef CONFIG_IRQ_OFFLOAD_NESTED
	arch_irq_offload(routine, parameter);
#else
	unsigned int id = arch_curr_cpu()->id;

	k_sem_take(&offload_sem[id], K_FOREVER);
	arch_irq_offload(routine, parameter);
	k_sem_give(&offload_sem[id]);
#endif /* CONFIG_IRQ_OFFLOAD_NESTED */
}
