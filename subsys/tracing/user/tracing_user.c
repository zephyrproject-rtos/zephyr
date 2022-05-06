/*
 * Copyright (c) 2020 Lexmark International, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <tracing_user.h>
#include <kernel_internal.h>
#include <zephyr/kernel_structs.h>
#include <ksched.h>

static int nested_interrupts[CONFIG_MP_NUM_CPUS];

void __weak sys_trace_thread_switched_in_user(struct k_thread *thread) {}
void __weak sys_trace_thread_switched_out_user(struct k_thread *thread) {}
void __weak sys_trace_isr_enter_user(int nested_interrupts) {}
void __weak sys_trace_isr_exit_user(int nested_interrupts) {}
void __weak sys_trace_idle_user(void) {}

void sys_trace_k_thread_switched_in(void)
{
	int key = irq_lock();

	__ASSERT_NO_MSG(nested_interrupts[_current_cpu->id] == 0);
	/* Can't use k_current_get as thread base and z_tls_current might be incorrect */
	sys_trace_thread_switched_in_user(z_current_get());

	irq_unlock(key);
}

void sys_trace_k_thread_switched_out(void)
{
	int key = irq_lock();

	__ASSERT_NO_MSG(nested_interrupts[_current_cpu->id] == 0);
	/* Can't use k_current_get as thread base and z_tls_current might be incorrect */
	sys_trace_thread_switched_out_user(z_current_get());

	irq_unlock(key);
}

void sys_trace_isr_enter(void)
{
	int key = irq_lock();
	_cpu_t *curr_cpu = _current_cpu;

	sys_trace_isr_enter_user(nested_interrupts[curr_cpu->id]);
	nested_interrupts[curr_cpu->id]++;

	irq_unlock(key);
}

void sys_trace_isr_exit(void)
{
	int key = irq_lock();
	_cpu_t *curr_cpu = _current_cpu;

	nested_interrupts[curr_cpu->id]--;
	sys_trace_isr_exit_user(nested_interrupts[curr_cpu->id]);

	irq_unlock(key);
}

void sys_trace_idle(void)
{
	sys_trace_idle_user();
}
