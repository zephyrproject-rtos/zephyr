/*
 * Copyright (c) 2018 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/kernel_structs.h>
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

void sys_trace_k_thread_switched_out(void)
{
	ctf_bounded_string_t name = { "unknown" };
	struct k_thread *thread;

	thread = k_current_get();
	_get_thread_name(thread, &name);

	ctf_top_thread_switched_out((uint32_t)(uintptr_t)thread, name);
}

void sys_trace_k_thread_switched_in(void)
{
	struct k_thread *thread;
	ctf_bounded_string_t name = { "unknown" };

	thread = k_current_get();
	_get_thread_name(thread, &name);

	ctf_top_thread_switched_in((uint32_t)(uintptr_t)thread, name);
}

void sys_trace_k_thread_priority_set(struct k_thread *thread)
{
	ctf_bounded_string_t name = { "unknown" };

	_get_thread_name(thread, &name);
	ctf_top_thread_priority_set((uint32_t)(uintptr_t)thread,
				    thread->base.prio, name);
}

void sys_trace_k_thread_create(struct k_thread *thread, size_t stack_size, int prio)
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

void sys_trace_k_thread_abort(struct k_thread *thread)
{
	ctf_bounded_string_t name = { "unknown" };

	_get_thread_name(thread, &name);
	ctf_top_thread_abort((uint32_t)(uintptr_t)thread, name);
}

void sys_trace_k_thread_suspend(struct k_thread *thread)
{
	ctf_bounded_string_t name = { "unknown" };

	_get_thread_name(thread, &name);
	ctf_top_thread_suspend((uint32_t)(uintptr_t)thread, name);
}

void sys_trace_k_thread_resume(struct k_thread *thread)
{
	ctf_bounded_string_t name = { "unknown" };

	_get_thread_name(thread, &name);

	ctf_top_thread_resume((uint32_t)(uintptr_t)thread, name);
}

void sys_trace_k_thread_ready(struct k_thread *thread)
{
	ctf_bounded_string_t name = { "unknown" };

	_get_thread_name(thread, &name);

	ctf_top_thread_ready((uint32_t)(uintptr_t)thread, name);
}

void sys_trace_k_thread_start(struct k_thread *thread)
{

}

void sys_trace_k_thread_pend(struct k_thread *thread)
{
	ctf_bounded_string_t name = { "unknown" };

	_get_thread_name(thread, &name);
	ctf_top_thread_pend((uint32_t)(uintptr_t)thread, name);
}

void sys_trace_k_thread_info(struct k_thread *thread)
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

void sys_trace_k_thread_name_set(struct k_thread *thread, int ret)
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

/* Semaphore */
void sys_trace_k_sem_init(struct k_sem *sem, int ret)
{
	ctf_top_semaphore_init(
		(uint32_t)(uintptr_t)sem,
		(int32_t)ret
		);
}

void sys_trace_k_sem_take_enter(struct k_sem *sem, k_timeout_t timeout)
{
	ctf_top_semaphore_take_enter(
		(uint32_t)(uintptr_t)sem,
		k_ticks_to_us_floor32((uint32_t)timeout.ticks)
		);
}


void sys_trace_k_sem_take_blocking(struct k_sem *sem, k_timeout_t timeout)
{
	ctf_top_semaphore_take_blocking(
		(uint32_t)(uintptr_t)sem,
		k_ticks_to_us_floor32((uint32_t)timeout.ticks)
		);
}

void sys_trace_k_sem_take_exit(struct k_sem *sem, k_timeout_t timeout, int ret)
{
	ctf_top_semaphore_take_exit(
		(uint32_t)(uintptr_t)sem,
		k_ticks_to_us_floor32((uint32_t)timeout.ticks),
		(uint32_t)ret
		);
}

void sys_trace_k_sem_reset(struct k_sem *sem)
{
	ctf_top_semaphore_reset(
		(uint32_t)(uintptr_t)sem
		);
}

void sys_trace_k_sem_give_enter(struct k_sem *sem)
{
	ctf_top_semaphore_give_enter(
		(uint32_t)(uintptr_t)sem
		);
}

void sys_trace_k_sem_give_exit(struct k_sem *sem)
{
	ctf_top_semaphore_give_exit(
		(uint32_t)(uintptr_t)sem
		);
}

/* Mutex */
void sys_trace_k_mutex_init(struct k_mutex *mutex, int ret)
{
	ctf_top_mutex_init(
		(uint32_t)(uintptr_t)mutex,
		(int32_t)ret
		);
}

void sys_trace_k_mutex_lock_enter(struct k_mutex *mutex, k_timeout_t timeout)
{
	ctf_top_mutex_lock_enter(
		(uint32_t)(uintptr_t)mutex,
		k_ticks_to_us_floor32((uint32_t)timeout.ticks)
		);
}

void sys_trace_k_mutex_lock_blocking(struct k_mutex *mutex, k_timeout_t timeout)
{
	ctf_top_mutex_lock_blocking(
		(uint32_t)(uintptr_t)mutex,
		k_ticks_to_us_floor32((uint32_t)timeout.ticks)
		);
}

void sys_trace_k_mutex_lock_exit(struct k_mutex *mutex, k_timeout_t timeout, int ret)
{
	ctf_top_mutex_lock_exit(
		(uint32_t)(uintptr_t)mutex,
		k_ticks_to_us_floor32((uint32_t)timeout.ticks),
		(int32_t)ret
		);
}

void sys_trace_k_mutex_unlock_enter(struct k_mutex *mutex)
{
	ctf_top_mutex_unlock_enter(
		(uint32_t)(uintptr_t)mutex
		);
}

void sys_trace_k_mutex_unlock_exit(struct k_mutex *mutex, int ret)
{
	ctf_top_mutex_unlock_exit(
		(uint32_t)(uintptr_t)mutex,
		(int32_t)ret
		);
}

/* Timer */
void sys_trace_k_timer_init(struct k_timer *timer)
{
	ctf_top_timer_init(
		(uint32_t)(uintptr_t)timer);
}

void sys_trace_k_timer_start(struct k_timer *timer, k_timeout_t duration,
			     k_timeout_t period)
{
	ctf_top_timer_start(
		(uint32_t)(uintptr_t)timer,
		k_ticks_to_us_floor32((uint32_t)duration.ticks),
		k_ticks_to_us_floor32((uint32_t)period.ticks)
		);
}

void sys_trace_k_timer_stop(struct k_timer *timer)
{
	ctf_top_timer_stop(
		(uint32_t)(uintptr_t)timer
		);
}

void sys_trace_k_timer_status_sync_enter(struct k_timer *timer)
{
	ctf_top_timer_status_sync_enter(
		(uint32_t)(uintptr_t)timer
		);
}

void sys_trace_k_timer_status_sync_blocking(struct k_timer *timer, k_timeout_t timeout)
{
	ctf_top_timer_status_sync_blocking(
		(uint32_t)(uintptr_t)timer,
		k_ticks_to_us_floor32((uint32_t)timeout.ticks)
		);
}

void sys_trace_k_timer_status_sync_exit(struct k_timer *timer, uint32_t result)
{
	ctf_top_timer_status_sync_exit(
		(uint32_t)(uintptr_t)timer,
		result
		);
}
