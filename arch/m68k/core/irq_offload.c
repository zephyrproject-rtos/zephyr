/*
 * Copyright (c) 2026 Dimitri Varpusvuori
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/irq_offload.h>

#include <ksched.h>

void arch_irq_offload(irq_offload_routine_t routine,
		const void *parameter)
{
	unsigned int key = arch_irq_lock();

	_current_cpu->nested++;

	routine(parameter);

	_current_cpu->nested--;

	arch_irq_unlock(key);

	z_reschedule_unlocked();
}

void arch_irq_offload_init(void)
{
}
