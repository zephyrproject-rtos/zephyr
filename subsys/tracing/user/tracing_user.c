/*
 * Copyright (c) 2020 Lexmark International, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <tracing_user.h>
#include <kernel_internal.h>
#include <ksched.h>

static int nested_interrupts;

void __weak sys_trace_thread_switched_in_user(struct k_thread *thread) {}
void __weak sys_trace_thread_switched_out_user(struct k_thread *thread) {}
void __weak sys_trace_isr_enter_user(void) {}
void __weak sys_trace_isr_exit_user(void) {}
void __weak sys_trace_idle_user(void) {}

void sys_trace_k_thread_switched_in(void)
{
	int key = irq_lock();

	__ASSERT_NO_MSG(nested_interrupts == 0);

	sys_trace_thread_switched_in_user(k_current_get());

	irq_unlock(key);
}

void sys_trace_k_thread_switched_out(void)
{
	int key = irq_lock();

	__ASSERT_NO_MSG(nested_interrupts == 0);

	sys_trace_thread_switched_out_user(k_current_get());

	irq_unlock(key);
}

void sys_trace_isr_enter(void)
{
	int key = irq_lock();

	if (nested_interrupts == 0) {
		sys_trace_isr_enter_user();
	}
	nested_interrupts++;
	irq_unlock(key);
}

void sys_trace_isr_exit(void)
{
	int key = irq_lock();

	nested_interrupts--;
	if (nested_interrupts == 0) {
		sys_trace_isr_exit_user();
	}
	irq_unlock(key);
}

void sys_trace_idle(void)
{
	sys_trace_idle_user();
}
