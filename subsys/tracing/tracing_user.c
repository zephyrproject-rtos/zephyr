/*
 * Copyright (c) 2020 Lexmark International, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <tracing_user.h>
#include <sys/printk.h>
#include <kernel_internal.h>
#include <ksched.h>

static int nested_interrupts;

void __weak user_sys_trace_thread_switched_in(struct k_thread *thread) {}
void __weak user_sys_trace_thread_switched_out(struct k_thread *thread) {}
void __weak user_sys_trace_isr_enter(void) {}
void __weak user_sys_trace_isr_exit(void) {}
void __weak user_sys_trace_idle(void) {}

void sys_trace_thread_switched_in(void)
{
	int key = irq_lock();

	__ASSERT_NO_MSG(nested_interrupts == 0);

	user_sys_trace_thread_switched_in(k_current_get());

	irq_unlock(key);
}

void sys_trace_thread_switched_out(void)
{
	int key = irq_lock();

	__ASSERT_NO_MSG(nested_interrupts == 0);

	user_sys_trace_thread_switched_out(k_current_get());

	irq_unlock(key);
}

void sys_trace_isr_enter(void)
{
	int key = irq_lock();

	if (nested_interrupts == 0) {
		user_sys_trace_isr_enter();
	}
	nested_interrupts++;
	irq_unlock(key);
}

void sys_trace_isr_exit(void)
{
	int key = irq_lock();

	nested_interrupts--;
	if (nested_interrupts == 0) {
		user_sys_trace_isr_exit();
	}
	irq_unlock(key);
}

void sys_trace_idle(void)
{
	user_sys_trace_idle();
}
