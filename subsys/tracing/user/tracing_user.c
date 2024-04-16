/*
 * Copyright (c) 2020 Lexmark International, Inc.
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <tracing_user.h>
#include <zephyr/kernel.h>

void __weak sys_trace_thread_create_user(struct k_thread *thread) {}
void __weak sys_trace_thread_abort_user(struct k_thread *thread) {}
void __weak sys_trace_thread_suspend_user(struct k_thread *thread) {}
void __weak sys_trace_thread_resume_user(struct k_thread *thread) {}
void __weak sys_trace_thread_name_set_user(struct k_thread *thread) {}
void __weak sys_trace_thread_switched_in_user(void) {}
void __weak sys_trace_thread_switched_out_user(void) {}
void __weak sys_trace_thread_info_user(struct k_thread *thread) {}
void __weak sys_trace_thread_sched_ready_user(struct k_thread *thread) {}
void __weak sys_trace_thread_pend_user(struct k_thread *thread) {}
void __weak sys_trace_thread_priority_set_user(struct k_thread *thread, int prio) {}
void __weak sys_trace_isr_enter_user(void) {}
void __weak sys_trace_isr_exit_user(void) {}
void __weak sys_trace_idle_user(void) {}

void sys_trace_thread_create(struct k_thread *thread)
{
	sys_trace_thread_create_user(thread);
}

void sys_trace_thread_abort(struct k_thread *thread)
{
	sys_trace_thread_abort_user(thread);
}

void sys_trace_thread_suspend(struct k_thread *thread)
{
	sys_trace_thread_suspend_user(thread);
}

void sys_trace_thread_resume(struct k_thread *thread)
{
	sys_trace_thread_resume_user(thread);
}

void sys_trace_thread_name_set(struct k_thread *thread)
{
	sys_trace_thread_name_set_user(thread);
}

void sys_trace_k_thread_switched_in(void)
{
	sys_trace_thread_switched_in_user();
}

void sys_trace_k_thread_switched_out(void)
{
	sys_trace_thread_switched_out_user();
}

void sys_trace_thread_info(struct k_thread *thread)
{
	sys_trace_thread_info_user(thread);
}

void sys_trace_thread_sched_priority_set(struct k_thread *thread, int prio)
{
	sys_trace_thread_priority_set_user(thread, prio);
}

void sys_trace_thread_sched_ready(struct k_thread *thread)
{
	sys_trace_thread_sched_ready_user(thread);
}

void sys_trace_thread_pend(struct k_thread *thread)
{
	sys_trace_thread_pend_user(thread);
}

void sys_trace_isr_enter(void)
{
	sys_trace_isr_enter_user();
}

void sys_trace_isr_exit(void)
{
	sys_trace_isr_exit_user();
}

void sys_trace_idle(void)
{
	sys_trace_idle_user();
}
