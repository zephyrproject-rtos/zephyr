/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _TRACE_SYSVIEW_H
#define _TRACE_SYSVIEW_H
#include <kernel.h>
#include <init.h>

#include <SEGGER_SYSVIEW.h>
#include <Global.h>
#include "SEGGER_SYSVIEW_Zephyr.h"

void sys_trace_thread_switched_in(void);
void sys_trace_thread_switched_out(void);
void sys_trace_isr_enter(void);
void sys_trace_isr_exit(void);
void sys_trace_isr_exit_to_scheduler(void);
void sys_trace_idle(void);
void sys_trace_semaphore_init(struct k_sem *sem);
void sys_trace_semaphore_take(struct k_sem *sem);
void sys_trace_semaphore_give(struct k_sem *sem);
void sys_trace_mutex_init(struct k_mutex *mutex);
void sys_trace_mutex_lock(struct k_mutex *mutex);
void sys_trace_mutex_unlock(struct k_mutex *mutex);

#define sys_trace_thread_priority_set(thread)

static inline void sys_trace_thread_info(struct k_thread *thread)
{
	char name[20];

	snprintk(name, sizeof(name), "T%pE%p", thread, &thread->entry);

	SEGGER_SYSVIEW_TASKINFO Info;

	Info.TaskID = (uint32_t)(uintptr_t)thread;
	Info.sName = name;
	Info.Prio = thread->base.prio;
	Info.StackBase = thread->stack_info.size;
	Info.StackSize = thread->stack_info.start;
	SEGGER_SYSVIEW_SendTaskInfo(&Info);
}

#define sys_trace_thread_create(thread)				       \
	do {							       \
		SEGGER_SYSVIEW_OnTaskCreate((uint32_t)(uintptr_t)thread); \
		sys_trace_thread_info(thread);			       \
	} while (0)

#define sys_trace_thread_name_set(thread)

#define sys_trace_thread_abort(thread)

#define sys_trace_thread_suspend(thread)

#define sys_trace_thread_resume(thread)

#define sys_trace_thread_ready(thread) \
	SEGGER_SYSVIEW_OnTaskStartReady((uint32_t)(uintptr_t)thread)

#define sys_trace_thread_pend(thread) \
	SEGGER_SYSVIEW_OnTaskStopReady((uint32_t)(uintptr_t)thread, 3 << 3)

#define sys_trace_void(id) SEGGER_SYSVIEW_RecordVoid(id)

#define sys_trace_end_call(id) SEGGER_SYSVIEW_RecordEndCall(id)

#endif /* _TRACE_SYSVIEW_H */
