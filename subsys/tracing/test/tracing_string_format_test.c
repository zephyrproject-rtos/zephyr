/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <tracing_test.h>
#include <tracing/tracing_format.h>

void sys_trace_k_thread_switched_out(void)
{
	struct k_thread *thread;

	thread = k_current_get();
	TRACING_STRING("%s: %p\n", __func__, thread);
}

void sys_trace_k_thread_switched_in(void)
{
	struct k_thread *thread;

	thread = k_current_get();
	TRACING_STRING("%s: %p\n", __func__, thread);
}

void sys_trace_k_thread_priority_set(struct k_thread *thread)
{
	TRACING_STRING("%s: %p\n", __func__, thread);
}

void sys_trace_k_thread_create(struct k_thread *thread, size_t stack_size,
			       int prio)
{
	TRACING_STRING("%s: %p\n", __func__, thread);
}

void sys_trace_k_thread_start(struct k_thread *thread)
{
	TRACING_STRING("%s: %p\n", __func__, thread);
}

void sys_trace_k_thread_abort(struct k_thread *thread)
{
	TRACING_STRING("%s: %p\n", __func__, thread);
}

void sys_trace_k_thread_suspend(struct k_thread *thread)
{
	TRACING_STRING("%s: %p\n", __func__, thread);
}

void sys_trace_k_thread_resume(struct k_thread *thread)
{
	TRACING_STRING("%s: %p\n", __func__, thread);
}

void sys_trace_k_thread_ready(struct k_thread *thread)
{
	TRACING_STRING("%s: %p\n", __func__, thread);
}

void sys_trace_k_thread_sched_ready(struct k_thread *thread)
{
	TRACING_STRING("%s: %p\n", __func__, thread);
}

void sys_trace_k_thread_sched_pend(struct k_thread *thread)
{
	TRACING_STRING("%s: %p\n", __func__, thread);
}

void sys_trace_k_thread_sched_abort(struct k_thread *thread)
{
	TRACING_STRING("%s: %p\n", __func__, thread);
}

void sys_trace_k_thread_sched_resume(struct k_thread *thread)
{
	TRACING_STRING("%s: %p\n", __func__, thread);
}

void sys_trace_k_thread_sched_suspend(struct k_thread *thread)
{
	TRACING_STRING("%s: %p\n", __func__, thread);
}

void sys_trace_k_thread_sleep_enter(k_timeout_t timeout)
{
	TRACING_STRING("%s\n", __func__);
}

void sys_trace_k_thread_sleep_exit(k_timeout_t timeout, int ret)
{
	TRACING_STRING("%s\n", __func__);
}

void sys_trace_k_thread_busy_wait_enter(uint32_t usec_to_wait)
{
	TRACING_STRING("%s\n", __func__);
}

void sys_trace_k_thread_busy_wait_exit(uint32_t usec_to_wait)
{
	TRACING_STRING("%s\n", __func__);
}

void sys_trace_k_thread_abort_enter(struct k_thread *thread)
{
	TRACING_STRING("%s: %p\n", __func__, thread);
}

void sys_trace_k_thread_abort_exit(struct k_thread *thread)
{
	TRACING_STRING("%s: %p\n", __func__, thread);
}

void sys_trace_k_thread_yield(void)
{
	TRACING_STRING("%s\n", __func__);
}

void sys_trace_thread_yield(struct k_thread *thread)
{
	TRACING_STRING("%s: %p\n", __func__, thread);
}

void sys_trace_k_thread_wakeup(struct k_thread *thread)
{
	TRACING_STRING("%s: %p\n", __func__, thread);
}

void sys_trace_k_thread_pend(struct k_thread *thread)
{
	TRACING_STRING("%s: %p\n", __func__, thread);
}

void sys_trace_k_thread_info(struct k_thread *thread)
{
	TRACING_STRING("%s: %p\n", __func__, thread);
}

void sys_trace_k_thread_name_set(struct k_thread *thread, int ret)
{
	TRACING_STRING("%s: %p\n", __func__, thread);
}

void sys_trace_k_thread_sched_lock(void)
{
	TRACING_STRING("%s\n", __func__);
}

void sys_port_trace_k_thread_sched_unlock(void)
{
	TRACING_STRING("%s\n", __func__);
}


void sys_trace_k_thread_join_blocking(struct k_thread *thread, k_timeout_t timeout)
{
	TRACING_STRING("%s %p, timeout: %u\n", __func__, thread, (uint32_t)timeout.ticks);
}

void sys_trace_k_thread_join_exit(struct k_thread *thread, k_timeout_t timeout, int ret)
{
	TRACING_STRING("%s %p, timeout: %u\n", __func__, thread, (uint32_t)timeout.ticks);
}

void sys_trace_isr_enter(void)
{
	TRACING_STRING("%s\n", __func__);
}

void sys_trace_isr_exit(void)
{
	TRACING_STRING("%s\n", __func__);
}

void sys_trace_isr_exit_to_scheduler(void)
{
	TRACING_STRING("%s\n", __func__);
}

void sys_trace_idle(void)
{
	TRACING_STRING("%s\n", __func__);
}

void sys_trace_k_condvar_broadcast_enter(struct k_condvar *condvar)
{
	TRACING_STRING("%s: %p\n", __func__, condvar);
}

void sys_trace_k_condvar_broadcast_exit(struct k_condvar *condvar, int ret)
{
	TRACING_STRING("%s: %p\n", __func__, condvar);
}

void sys_trace_k_condvar_init(struct k_condvar *condvar, int ret)
{
	TRACING_STRING("%s: %p\n", __func__, condvar);
}

void sys_trace_k_condvar_signal_enter(struct k_condvar *condvar)
{
	TRACING_STRING("%s: %p\n", __func__, condvar);
}

void sys_trace_k_condvar_signal_blocking(struct k_condvar *condvar)
{
	TRACING_STRING("%s: %p\n", __func__, condvar);
}

void sys_trace_k_condvar_signal_exit(struct k_condvar *condvar, int ret)
{
	TRACING_STRING("%s: %p\n", __func__, condvar);
}

void sys_trace_k_condvar_wait_enter(struct k_condvar *condvar, struct k_mutex *mutex,
				    k_timeout_t timeout)
{
	TRACING_STRING("%s: %p\n", __func__, condvar);
}

void sys_trace_k_condvar_wait_exit(struct k_condvar *condvar, struct k_mutex *mutex,
				   k_timeout_t timeout, int ret)
{
	TRACING_STRING("%s: %p\n", __func__, condvar);
}


void sys_trace_k_sem_init(struct k_sem *sem, int ret)
{
	TRACING_STRING("%s: %p\n", __func__, sem);
}
void sys_trace_k_sem_give_enter(struct k_sem *sem)
{
	TRACING_STRING("%s: %p\n", __func__, sem);
}

void sys_trace_k_sem_take_enter(struct k_sem *sem, k_timeout_t timeout)
{
	TRACING_STRING("%s: %p, timeout: %u\n", __func__, sem, (uint32_t)timeout.ticks);
}

void sys_trace_k_sem_take_exit(struct k_sem *sem, k_timeout_t timeout, int ret)
{
	TRACING_STRING("%s: %p, timeout: %u\n", __func__, sem, (uint32_t)timeout.ticks);
}

void sys_trace_k_sem_take_blocking(struct k_sem *sem, k_timeout_t timeout)
{
	TRACING_STRING("%s: %p, timeout: %u\n", __func__, sem, (uint32_t)timeout.ticks);
}

void sys_trace_k_mutex_init(struct k_mutex *mutex, int ret)
{
	TRACING_STRING("%s: %p, returns %d\n", __func__, mutex, ret);
}

void sys_trace_k_mutex_lock_enter(struct k_mutex *mutex, k_timeout_t timeout)
{
	TRACING_STRING("%s: %p, timeout: %u\n", __func__, mutex, (uint32_t)timeout.ticks);
}

void sys_trace_k_mutex_lock_exit(struct k_mutex *mutex, k_timeout_t timeout, int ret)
{
	TRACING_STRING("%s: %p, timeout: %u, returns: %d\n", __func__, mutex,
			(uint32_t)timeout.ticks, ret);
}

void sys_trace_k_mutex_lock_blocking(struct k_mutex *mutex, k_timeout_t timeout)
{
	TRACING_STRING("%s: %p, timeout: %u\n", __func__, mutex, (uint32_t)timeout.ticks);
}

void sys_trace_k_mutex_unlock_enter(struct k_mutex *mutex)
{
	TRACING_STRING("%s: %p\n", __func__, mutex);
}


void sys_trace_k_mutex_unlock_exit(struct k_mutex *mutex, int ret)
{
	TRACING_STRING("%s: %p, return: %d\n", __func__, mutex, ret);
}

void sys_trace_k_thread_sched_set_priority(struct k_thread *thread, int prio)
{
	TRACING_STRING("%s: %p, priority: %d\n", __func__, thread, prio);
}

void sys_trace_k_timer_start(struct k_timer *timer, k_timeout_t duration,
			     k_timeout_t period)
{
	TRACING_STRING("%s: %p, duration: %d, period: %d\n", __func__, timer, duration, period);
}

void sys_trace_k_timer_init(struct k_timer *timer, k_timer_expiry_t expiry_fn,
			    k_timer_expiry_t stop_fn)
{
	TRACING_STRING("%s: %p\n", __func__, timer);
}
