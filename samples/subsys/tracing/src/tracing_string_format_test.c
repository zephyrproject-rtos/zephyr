/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <tracing_test.h>
#include <tracing/tracing_format.h>

void sys_trace_thread_switched_out(void)
{
	TRACING_STRING("%s %d\n", __func__, __LINE__);
}

void sys_trace_thread_switched_in(void)
{
	TRACING_STRING("%s %d\n", __func__, __LINE__);
}

void sys_trace_thread_priority_set(struct k_thread *thread)
{
	TRACING_STRING("%s %d\n", __func__, __LINE__);
}

void sys_trace_thread_create(struct k_thread *thread)
{
	TRACING_STRING("%s %d\n", __func__, __LINE__);
}

void sys_trace_thread_abort(struct k_thread *thread)
{
	TRACING_STRING("%s %d\n", __func__, __LINE__);
}

void sys_trace_thread_suspend(struct k_thread *thread)
{
	TRACING_STRING("%s %d\n", __func__, __LINE__);
}

void sys_trace_thread_resume(struct k_thread *thread)
{
	TRACING_STRING("%s %d\n", __func__, __LINE__);
}

void sys_trace_thread_ready(struct k_thread *thread)
{
	TRACING_STRING("%s %d\n", __func__, __LINE__);
}

void sys_trace_thread_pend(struct k_thread *thread)
{
	TRACING_STRING("%s %d\n", __func__, __LINE__);
}

void sys_trace_thread_info(struct k_thread *thread)
{
	TRACING_STRING("%s %d\n", __func__, __LINE__);
}

void sys_trace_thread_name_set(struct k_thread *thread)
{
	TRACING_STRING("%s %d\n", __func__, __LINE__);
}

void sys_trace_isr_enter(void)
{
	TRACING_STRING("%s %d\n", __func__, __LINE__);
}

void sys_trace_isr_exit(void)
{
	TRACING_STRING("%s %d\n", __func__, __LINE__);
}

void sys_trace_isr_exit_to_scheduler(void)
{
	TRACING_STRING("%s %d\n", __func__, __LINE__);
}

void sys_trace_idle(void)
{
	TRACING_STRING("%s %d\n", __func__, __LINE__);
}

void sys_trace_void(unsigned int id)
{
	TRACING_STRING("%s %d\n", __func__, __LINE__);
}

void sys_trace_end_call(unsigned int id)
{
	TRACING_STRING("%s %d\n", __func__, __LINE__);
}

void sys_trace_semaphore_init(struct k_sem *sem)
{
	TRACING_STRING("%s %d\n", __func__, __LINE__);
}

void sys_trace_semaphore_take(struct k_sem *sem)
{
	TRACING_STRING("%s %d\n", __func__, __LINE__);
}

void sys_trace_semaphore_give(struct k_sem *sem)
{
	TRACING_STRING("%s %d\n", __func__, __LINE__);
}

void sys_trace_mutex_init(struct k_mutex *mutex)
{
	TRACING_STRING("%s %d\n", __func__, __LINE__);
}

void sys_trace_mutex_lock(struct k_mutex *mutex)
{
	TRACING_STRING("%s %d\n", __func__, __LINE__);
}

void sys_trace_mutex_unlock(struct k_mutex *mutex)
{
	TRACING_STRING("%s %d\n", __func__, __LINE__);
}
