/*
 * Copyright (c) 2020 Lexmark International, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <tracing_user.h>

static int nested_interrupts[CONFIG_MP_MAX_NUM_CPUS];

void sys_trace_thread_switched_in_user(void)
{
	unsigned int key = irq_lock();

	__ASSERT_NO_MSG(nested_interrupts[arch_curr_cpu()->id] == 0);
	/* Can't use k_current_get as thread base and z_tls_current might be incorrect */
	printk("%s: %p\n", __func__, k_sched_current_thread_query());

	irq_unlock(key);
}

void sys_trace_thread_switched_out_user(void)
{
	unsigned int key = irq_lock();

	__ASSERT_NO_MSG(nested_interrupts[arch_curr_cpu()->id] == 0);
	/* Can't use k_current_get as thread base and z_tls_current might be incorrect */
	printk("%s: %p\n", __func__, k_sched_current_thread_query());

	irq_unlock(key);
}

void sys_trace_isr_enter_user(void)
{
	unsigned int key = irq_lock();
	_cpu_t *curr_cpu = arch_curr_cpu();

	printk("%s: %d\n", __func__, nested_interrupts[curr_cpu->id]);
	nested_interrupts[curr_cpu->id]++;

	irq_unlock(key);
}

void sys_trace_isr_exit_user(void)
{
	unsigned int key = irq_lock();
	_cpu_t *curr_cpu = arch_curr_cpu();

	nested_interrupts[curr_cpu->id]--;
	printk("%s: %d\n", __func__, nested_interrupts[curr_cpu->id]);

	irq_unlock(key);
}

void sys_trace_idle_user(void)
{
	printk("%s\n", __func__);
}
