/*
 * Copyright (c) 2018,2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <kernel_internal.h>
#include <zephyr/spinlock.h>

bool z_spin_lock_valid(struct k_spinlock *l)
{
	uintptr_t thread_cpu = l->thread_cpu;

	if (thread_cpu != 0U) {
		if ((thread_cpu & 3U) == _current_cpu->id) {
			return false;
		}
	}
	return true;
}

bool z_spin_unlock_valid(struct k_spinlock *l)
{
	uintptr_t tcpu = l->thread_cpu;

	l->thread_cpu = 0;

	if (arch_is_in_isr() && _current->base.thread_state & _THREAD_DUMMY) {
		/* Edge case where an ISR aborted _current */
		return true;
	}
	if (tcpu != (_current_cpu->id | (uintptr_t)_current)) {
		return false;
	}
	return true;
}

void z_spin_lock_set_owner(struct k_spinlock *l)
{
	l->thread_cpu = _current_cpu->id | (uintptr_t)_current;
}

#ifdef CONFIG_KERNEL_COHERENCE
bool z_spin_lock_mem_coherent(struct k_spinlock *l)
{
	return arch_mem_coherent((void *)l);
}
#endif /* CONFIG_KERNEL_COHERENCE */
