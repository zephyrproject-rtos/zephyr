/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _TRACE_TEST_H
#define _TRACE_TEST_H

#include <kernel.h>
#include <kernel_structs.h>

#ifdef __cplusplus
extern "C" {
#endif

void sys_trace_thread_switched_out(void);
void sys_trace_thread_switched_in(void);
void sys_trace_thread_priority_set(struct k_thread *thread);
void sys_trace_thread_create(struct k_thread *thread);
void sys_trace_thread_abort(struct k_thread *thread);
void sys_trace_thread_suspend(struct k_thread *thread);
void sys_trace_thread_resume(struct k_thread *thread);
void sys_trace_thread_ready(struct k_thread *thread);
void sys_trace_thread_pend(struct k_thread *thread);
void sys_trace_thread_info(struct k_thread *thread);
void sys_trace_thread_name_set(struct k_thread *thread);
void sys_trace_isr_enter(void);
void sys_trace_isr_exit(void);
void sys_trace_isr_exit_to_scheduler(void);
void sys_trace_idle(void);
void sys_trace_void(unsigned int id);
void sys_trace_end_call(unsigned int id);
void sys_trace_semaphore_init(struct k_sem *sem);
void sys_trace_semaphore_take(struct k_sem *sem);
void sys_trace_semaphore_give(struct k_sem *sem);
void sys_trace_mutex_init(struct k_mutex *mutex);
void sys_trace_mutex_lock(struct k_mutex *mutex);
void sys_trace_mutex_unlock(struct k_mutex *mutex);
#ifdef __cplusplus
}
#endif

#endif
