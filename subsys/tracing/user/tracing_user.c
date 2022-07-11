/*
 * Copyright (c) 2020 Lexmark International, Inc.
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <tracing_user.h>
#include <kernel_internal.h>
#include <zephyr/kernel_structs.h>
#include <ksched.h>

static int nested_interrupts[CONFIG_MP_NUM_CPUS];

void __weak sys_trace_thread_create_user(struct k_thread *thread) {}
void __weak sys_trace_thread_abort_user(struct k_thread *thread) {}
void __weak sys_trace_thread_suspend_user(struct k_thread *thread) {}
void __weak sys_trace_thread_resume_user(struct k_thread *thread) {}
void __weak sys_trace_thread_name_set_user(struct k_thread *thread) {}
void __weak sys_trace_thread_switched_in_user(struct k_thread *thread) {}
void __weak sys_trace_thread_switched_out_user(struct k_thread *thread) {}
void __weak sys_trace_thread_info_user(struct k_thread *thread) {}
void __weak sys_trace_thread_sched_ready_user(struct k_thread *thread) {}
void __weak sys_trace_thread_pend_user(struct k_thread *thread) {}
void __weak sys_trace_thread_priority_set_user(struct k_thread *thread, int prio) {}
void __weak sys_trace_isr_enter_user(int nested_interrupts) {}
void __weak sys_trace_isr_exit_user(int nested_interrupts) {}
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
/* FIXME: Limitation of the current x86 EFI cosnole implementation. */
#if !defined(CONFIG_X86_EFI_CONSOLE) && !defined(CONFIG_UART_CONSOLE)

	int key = irq_lock();

	__ASSERT_NO_MSG(nested_interrupts[_current_cpu->id] == 0);
	/* Can't use k_current_get as thread base and z_tls_current might be incorrect */
	sys_trace_thread_switched_in_user(z_current_get());

	irq_unlock(key);
#endif
}

void sys_trace_k_thread_switched_out(void)
{
#if !defined(CONFIG_X86_EFI_CONSOLE) && !defined(CONFIG_UART_CONSOLE)
	int key = irq_lock();

	__ASSERT_NO_MSG(nested_interrupts[_current_cpu->id] == 0);
	/* Can't use k_current_get as thread base and z_tls_current might be incorrect */
	sys_trace_thread_switched_out_user(z_current_get());

	irq_unlock(key);
#endif
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
