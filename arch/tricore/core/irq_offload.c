/*
 * Copyright (c) 2024 Infineon Technologies AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/irq_offload.h>
#include <ksched.h>

void arch_irq_offload_init(void)
{
}

void arch_irq_offload(irq_offload_routine_t routine, const void *parameter)
{
	unsigned int key;

	key = irq_lock();
	arch_curr_cpu()->nested++;

	routine(parameter);

	arch_curr_cpu()->nested--;
	irq_unlock(key);

#if defined(CONFIG_MULTITHREADING)
	/* If the offloaded routine caused a scheduling change
	 * (e.g. thread abort), trigger reschedule via the normal
	 * SYSCALL context switch path.
	 */
	key = irq_lock();
	struct k_thread *curr = _current;
	struct k_thread *next = z_get_next_switch_handle(curr);

	if (next != NULL) {
		arch_switch(next, &curr->switch_handle);
	}
	irq_unlock(key);
#endif
}
