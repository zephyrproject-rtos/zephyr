/*
 * Copyright (c) 2020 Lexmark International, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <tracing_user.h>

void sys_trace_thread_switched_in_user(struct k_thread *thread)
{
	printk("%s: %p\n", __func__, thread);
}

void sys_trace_thread_switched_out_user(struct k_thread *thread)
{
	printk("%s: %p\n", __func__, thread);
}

void sys_trace_isr_enter_user(int nested_interrupts)
{
	printk("%s: %d\n", __func__, nested_interrupts);
}

void sys_trace_isr_exit_user(int nested_interrupts)
{
	printk("%s: %d\n", __func__, nested_interrupts);
}

void sys_trace_idle_user(void)
{
	printk("%s\n", __func__);
}
