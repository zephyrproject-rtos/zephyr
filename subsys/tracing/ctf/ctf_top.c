/*
 * Copyright (c) 2018 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <kernel_structs.h>
#include <kernel_internal.h>
#include <ctf_top.h>


static void _get_thread_name(struct k_thread *thread,
			     ctf_bounded_string_t *name)
{
	const char *tname = k_thread_name_get(thread);

	if (tname != NULL && tname[0] != '\0') {
		strncpy(name->buf, tname, sizeof(name->buf));
		/* strncpy may not always null-terminate */
		name->buf[sizeof(name->buf) - 1] = 0;
	}
}

void sys_trace_thread_switched_out(void)
{
	ctf_bounded_string_t name = { "unknown" };
	struct k_thread *thread;

	thread = k_current_get();
	_get_thread_name(thread, &name);

	ctf_top_thread_switched_out((uint32_t)(uintptr_t)thread, name);
}

void sys_trace_thread_switched_in(void)
{
	struct k_thread *thread;
	ctf_bounded_string_t name = { "unknown" };

	thread = k_current_get();
	_get_thread_name(thread, &name);

	ctf_top_thread_switched_in((uint32_t)(uintptr_t)thread, name);
}

void sys_trace_thread_priority_set(struct k_thread *thread)
{
	ctf_bounded_string_t name = { "unknown" };

	_get_thread_name(thread, &name);
	ctf_top_thread_priority_set((uint32_t)(uintptr_t)thread,
				    thread->base.prio, name);
}

void sys_trace_thread_create(struct k_thread *thread)
{
	ctf_bounded_string_t name = { "unknown" };

	_get_thread_name(thread, &name);
	ctf_top_thread_create(
		(uint32_t)(uintptr_t)thread,
		thread->base.prio,
		name
		);

#if defined(CONFIG_THREAD_STACK_INFO)
	ctf_top_thread_info(
		(uint32_t)(uintptr_t)thread,
		name,
		thread->stack_info.start,
		thread->stack_info.size
		);
#endif
}

void sys_trace_thread_abort(struct k_thread *thread)
{
	ctf_bounded_string_t name = { "unknown" };

	_get_thread_name(thread, &name);
	ctf_top_thread_abort((uint32_t)(uintptr_t)thread, name);
}

void sys_trace_thread_suspend(struct k_thread *thread)
{
	ctf_bounded_string_t name = { "unknown" };

	_get_thread_name(thread, &name);
	ctf_top_thread_suspend((uint32_t)(uintptr_t)thread, name);
}

void sys_trace_thread_resume(struct k_thread *thread)
{
	ctf_bounded_string_t name = { "unknown" };

	_get_thread_name(thread, &name);

	ctf_top_thread_resume((uint32_t)(uintptr_t)thread, name);
}

void sys_trace_thread_ready(struct k_thread *thread)
{
	ctf_bounded_string_t name = { "unknown" };

	_get_thread_name(thread, &name);

	ctf_top_thread_ready((uint32_t)(uintptr_t)thread, name);
}

void sys_trace_thread_pend(struct k_thread *thread)
{
	ctf_bounded_string_t name = { "unknown" };

	_get_thread_name(thread, &name);
	ctf_top_thread_pend((uint32_t)(uintptr_t)thread, name);
}

void sys_trace_thread_info(struct k_thread *thread)
{
#if defined(CONFIG_THREAD_STACK_INFO)
	ctf_bounded_string_t name = { "unknown" };

	_get_thread_name(thread, &name);
	ctf_top_thread_info(
		(uint32_t)(uintptr_t)thread,
		name,
		thread->stack_info.start,
		thread->stack_info.size
		);
#endif
}

void sys_trace_thread_name_set(struct k_thread *thread)
{
	ctf_bounded_string_t name = { "unknown" };

	_get_thread_name(thread, &name);
	ctf_top_thread_name_set(
		(uint32_t)(uintptr_t)thread,
		name
		);

}

void sys_trace_isr_enter(void)
{
	ctf_top_isr_enter();
}

void sys_trace_isr_exit(void)
{
	ctf_top_isr_exit();
}

void sys_trace_isr_exit_to_scheduler(void)
{
	ctf_top_isr_exit_to_scheduler();
}

void sys_trace_idle(void)
{
	ctf_top_idle();
}

void sys_trace_void(unsigned int id)
{
	ctf_top_void(id);
}

void sys_trace_semaphore_init(struct k_sem *sem)
{
	ctf_top_semaphore_init(
		(uint32_t)(uintptr_t)sem
		);
}

void sys_trace_semaphore_take(struct k_sem *sem)
{
	ctf_top_semaphore_take(
		(uint32_t)(uintptr_t)sem
		);
}

void sys_trace_semaphore_give(struct k_sem *sem)
{
	ctf_top_semaphore_give(
		(uint32_t)(uintptr_t)sem
		);
}

void sys_trace_mutex_init(struct k_mutex *mutex)
{
	ctf_top_mutex_init(
		(uint32_t)(uintptr_t)mutex
		);
}

void sys_trace_mutex_lock(struct k_mutex *mutex)
{
	ctf_top_mutex_lock(
		(uint32_t)(uintptr_t)mutex
		);
}

void sys_trace_mutex_unlock(struct k_mutex *mutex)
{
	ctf_top_mutex_unlock(
		(uint32_t)(uintptr_t)mutex
		);
}

void sys_trace_end_call(unsigned int id)
{
	ctf_top_end_call(id);
}
