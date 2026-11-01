/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/pm/pm.h>
#include <zephyr/sys/atomic.h>

/* CPUs currently inside pm_state_set(), one bit each */
atomic_t pm_cpus_in_state;

void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	unsigned int cpu = arch_curr_cpu()->id;

	ARG_UNUSED(state);
	ARG_UNUSED(substate_id);

	atomic_or(&pm_cpus_in_state, BIT(cpu));
	/* Interrupts stay locked: a pending one ends the wait without being taken. */
	__asm__ volatile("wfi" ::: "memory");
	atomic_and(&pm_cpus_in_state, ~BIT(cpu));
}

void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(state);
	ARG_UNUSED(substate_id);
}
