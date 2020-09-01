/*
 * Copyright (c) 2020 Lexmark International, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _TRACE_USER_H
#define _TRACE_USER_H
#include <kernel.h>
#include <kernel_structs.h>
#include <init.h>

#ifdef __cplusplus
extern "C" {
#endif

void user_sys_trace_thread_switched_in(struct k_thread *thread);
void user_sys_trace_thread_switched_out(struct k_thread *thread);
void user_sys_trace_isr_enter(void);
void user_sys_trace_isr_exit(void);
void user_sys_trace_idle(void);

void sys_trace_thread_switched_in(void);
void sys_trace_thread_switched_out(void);
void sys_trace_isr_enter(void);
void sys_trace_isr_exit(void);
void sys_trace_idle(void);

#define sys_trace_isr_exit_to_scheduler()

#define sys_trace_thread_priority_set(thread)
#define sys_trace_thread_info(thread)
#define sys_trace_thread_create(thread)
#define sys_trace_thread_abort(thread)
#define sys_trace_thread_suspend(thread)
#define sys_trace_thread_resume(thread)
#define sys_trace_thread_ready(thread)
#define sys_trace_thread_pend(thread)
#define sys_trace_thread_name_set(thread)

#define sys_trace_void(id)
#define sys_trace_end_call(id)

#ifdef __cplusplus
}
#endif

#endif /* _TRACE_USER_H */
