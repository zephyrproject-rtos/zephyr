/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <tracing_test.h>
#include <tracing/tracing_format.h>
tracing_data_t tracing_data;

void sys_trace_thread_switched_out(void)
{
	tracing_data.data = "sys_trace_thread_switched_out_t1";
	tracing_data.length = 50;
	tracing_format_raw_data("sys_trace_thread_switched_out_t2", 50);
	tracing_format_data(&tracing_data, 1);
	TRACING_STRING("%s %d\n", __func__, __LINE__);
}

void sys_trace_thread_switched_in(void)
{
	tracing_data.data = "sys_trace_thread_switched_in_t1";
	tracing_data.length = 50;
	tracing_format_raw_data("sys_trace_thread_switched_in_t2", 50);
	tracing_format_data(&tracing_data, 1);
	TRACING_STRING("%s %d\n", __func__, __LINE__);
}

void sys_trace_thread_priority_set(struct k_thread *thread)
{
	tracing_data.data = "sys_trace_thread_priority_set_t1";
	tracing_data.length = 50;
	tracing_format_raw_data("sys_trace_thread_priority_set_t2", 50);
	tracing_format_data(&tracing_data, 1);
	TRACING_STRING("%s %d\n", __func__, __LINE__);
}

void sys_trace_thread_create(struct k_thread *thread)
{
	tracing_data.data = "sys_trace_thread_create_t1";
	tracing_data.length = 50;
	tracing_format_raw_data("sys_trace_thread_create_t2", 50);
	tracing_format_data(&tracing_data, 1);
	TRACING_STRING("%s %d\n", __func__, __LINE__);
}

void sys_trace_thread_abort(struct k_thread *thread)
{
	tracing_data.data = "sys_trace_thread_abort_t1";
	tracing_data.length = 50;
	tracing_format_raw_data("sys_trace_thread_abort_t2", 50);
	tracing_format_data(&tracing_data, 1);
	TRACING_STRING("%s %d\n", __func__, __LINE__);
}

void sys_trace_thread_suspend(struct k_thread *thread)
{
	tracing_data.data = "sys_trace_thread_suspend_t1";
	tracing_data.length = 50;
	tracing_format_raw_data("sys_trace_thread_suspend_t2", 50);
	tracing_format_data(&tracing_data, 1);
	TRACING_STRING("%s %d\n", __func__, __LINE__);
}

void sys_trace_thread_resume(struct k_thread *thread)
{
	tracing_data.data = "sys_trace_thread_resume_t1";
	tracing_data.length = 50;
	tracing_format_raw_data("sys_trace_thread_resume_t2", 50);
	tracing_format_data(&tracing_data, 1);
	TRACING_STRING("%s %d\n", __func__, __LINE__);
}

void sys_trace_thread_ready(struct k_thread *thread)
{
	tracing_data.data = "sys_trace_thread_ready_t1";
	tracing_data.length = 50;
	tracing_format_raw_data("sys_trace_thread_ready_t2", 50);
	tracing_format_data(&tracing_data, 1);
	TRACING_STRING("%s %d\n", __func__, __LINE__);
}

void sys_trace_thread_pend(struct k_thread *thread)
{
	tracing_data.data = "sys_trace_thread_pend_t1";
	tracing_data.length = 50;
	tracing_format_raw_data("sys_trace_thread_pend_t2", 50);
	tracing_format_data(&tracing_data, 1);
	TRACING_STRING("%s %d\n", __func__, __LINE__);
}

void sys_trace_thread_info(struct k_thread *thread)
{
	tracing_data.data = "sys_trace_thread_info_t1";
	tracing_data.length = 50;
	tracing_format_raw_data("sys_trace_thread_info_t2", 50);
	tracing_format_data(&tracing_data, 1);
	TRACING_STRING("%s %d\n", __func__, __LINE__);
}

void sys_trace_thread_name_set(struct k_thread *thread)
{
	tracing_data.data = "sys_trace_thread_name_set_t1";
	tracing_data.length = 50;
	tracing_format_raw_data("sys_trace_thread_name_set_t2", 50);
	tracing_format_data(&tracing_data, 1);
	TRACING_STRING("%s %d\n", __func__, __LINE__);
}

void sys_trace_isr_enter(void)
{
	tracing_data.data = "sys_trace_isr_enter_t1";
	tracing_data.length = 50;
	tracing_format_raw_data("sys_trace_isr_enter_t2", 50);
	tracing_format_data(&tracing_data, 1);
	TRACING_STRING("%s %d\n", __func__, __LINE__);
}

void sys_trace_isr_exit(void)
{
	tracing_data.data = "sys_trace_isr_exit_t1";
	tracing_data.length = 50;
	tracing_format_raw_data("sys_trace_isr_exit_t2", 50);
	tracing_format_data(&tracing_data, 1);
	TRACING_STRING("%s %d\n", __func__, __LINE__);
}

void sys_trace_isr_exit_to_scheduler(void)
{
	tracing_data.data = "sys_trace_isr_exit_to_scheduler_t1";
	tracing_data.length = 50;
	tracing_format_raw_data("sys_trace_isr_exit_to_scheduler_t2", 50);
	tracing_format_data(&tracing_data, 1);
	TRACING_STRING("%s %d\n", __func__, __LINE__);
}

void sys_trace_idle(void)
{
	tracing_data.data = "sys_trace_idle_t1";
	tracing_data.length = 50;
	tracing_format_raw_data("sys_trace_idle_t2", 50);
	tracing_format_data(&tracing_data, 1);
	TRACING_STRING("%s %d\n", __func__, __LINE__);
}

void sys_trace_void(unsigned int id)
{
	tracing_data.data = "sys_trace_void_t1";
	tracing_data.length = 50;
	tracing_format_raw_data("sys_trace_void_t2", 50);
	tracing_format_data(&tracing_data, 1);
	TRACING_STRING("%s %d\n", __func__, __LINE__);
}

void sys_trace_end_call(unsigned int id)
{
	tracing_data.data = "sys_trace_end_call_t1";
	tracing_data.length = 50;
	tracing_format_raw_data("sys_trace_end_call_t2", 50);
	tracing_format_data(&tracing_data, 1);
	TRACING_STRING("%s %d\n", __func__, __LINE__);
}

void sys_trace_semaphore_init(struct k_sem *sem)
{
	tracing_data.data = "sys_trace_semaphore_init_t1";
	tracing_data.length = 50;
	tracing_format_raw_data("sys_trace_semaphore_init_t2", 50);
	tracing_format_data(&tracing_data, 1);
	TRACING_STRING("%s %d\n", __func__, __LINE__);
}

void sys_trace_semaphore_take(struct k_sem *sem)
{
	tracing_data.data = "sys_trace_semaphore_take_t1";
	tracing_data.length = 50;
	tracing_format_raw_data("sys_trace_semaphore_take_t2", 50);
	tracing_format_data(&tracing_data, 1);
	TRACING_STRING("%s %d\n", __func__, __LINE__);
}

void sys_trace_semaphore_give(struct k_sem *sem)
{
	tracing_data.data = "sys_trace_semaphore_give_t1";
	tracing_data.length = 50;
	tracing_format_raw_data("sys_trace_semaphore_give_t2", 50);
	tracing_format_data(&tracing_data, 1);
	TRACING_STRING("%s %d\n", __func__, __LINE__);
}

void sys_trace_mutex_init(struct k_mutex *mutex)
{
	tracing_data.data = "sys_trace_mutex_init_t1";
	tracing_data.length = 50;
	tracing_format_raw_data("sys_trace_mutex_init_t2", 50);
	tracing_format_data(&tracing_data, 1);
	TRACING_STRING("%s %d\n", __func__, __LINE__);
}

void sys_trace_mutex_lock(struct k_mutex *mutex)
{
	tracing_data.data = "sys_trace_mutex_lock_t1";
	tracing_data.length = 50;
	tracing_format_raw_data("sys_trace_mutex_lock_t2", 50);
	tracing_format_data(&tracing_data, 1);
	TRACING_STRING("%s %d\n", __func__, __LINE__);
}

void sys_trace_mutex_unlock(struct k_mutex *mutex)
{
	tracing_data.data = "sys_trace_mutex_unlock_t1";
	tracing_data.length = 50;
	tracing_format_raw_data("sys_trace_mutex_unlock_t2", 50);
	tracing_format_data(&tracing_data, 1);
	TRACING_STRING("%s %d\n", __func__, __LINE__);
}
