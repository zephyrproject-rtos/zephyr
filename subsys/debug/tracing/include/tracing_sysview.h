/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _TRACE_SYSVIEW_H
#define _TRACE_SYSVIEW_H
#include <kernel.h>
#include <kernel_structs.h>
#include <init.h>

#include <systemview/SEGGER_SYSVIEW.h>
#include <systemview/Global.h>
#include "SEGGER_SYSVIEW_Zephyr.h"

#ifndef CONFIG_SMP
extern k_tid_t const _idle_thread;
#endif

static inline int is_idle_thread(struct k_thread *thread)
{
#ifdef CONFIG_SMP
	return thread->base.is_idle;
#else
	return thread == _idle_thread;
#endif
}


static inline void _sys_trace_thread_switched_in(void)
{
	struct k_thread *thread;

	thread = k_current_get();

	if (is_idle_thread(thread)) {
		SEGGER_SYSVIEW_OnIdle();
	} else {
		SEGGER_SYSVIEW_OnTaskStartExec((u32_t)(uintptr_t)thread);
	}
}

#define sys_trace_thread_switched_in() _sys_trace_thread_switched_in()

#define sys_trace_thread_switched_out() SEGGER_SYSVIEW_OnTaskStopExec()

#define sys_trace_isr_enter() SEGGER_SYSVIEW_RecordEnterISR()

#define sys_trace_isr_exit() SEGGER_SYSVIEW_RecordExitISR()

#define sys_trace_isr_exit_to_scheduler() \
	SEGGER_SYSVIEW_RecordExitISRToScheduler()

#define sys_trace_thread_priority_set(thread)

static inline void sys_trace_thread_info(struct k_thread *thread)
{
	char name[20];

	snprintk(name, sizeof(name), "T%xE%x", (uintptr_t)thread,
		 (uintptr_t)&thread->entry);

	SEGGER_SYSVIEW_TASKINFO Info;

	Info.TaskID = (u32_t)(uintptr_t)thread;
	Info.sName = name;
	Info.Prio = thread->base.prio;
	Info.StackBase = thread->stack_info.size;
	Info.StackSize = thread->stack_info.start;
	SEGGER_SYSVIEW_SendTaskInfo(&Info);
}

#define sys_trace_thread_create(thread)				       \
	do {							       \
		SEGGER_SYSVIEW_OnTaskCreate((u32_t)(uintptr_t)thread); \
		sys_trace_thread_info(thread);			       \
	} while (0)

#define sys_trace_thread_abort(thread)

#define sys_trace_thread_suspend(thread)

#define sys_trace_thread_resume(thread)

#define sys_trace_thread_ready(thread) \
	SEGGER_SYSVIEW_OnTaskStartReady((u32_t)(uintptr_t)thread)

#define sys_trace_thread_pend(thread) \
	SEGGER_SYSVIEW_OnTaskStopReady((u32_t)(uintptr_t)thread, 3 << 3)

#define sys_trace_void(id) SEGGER_SYSVIEW_RecordVoid(id)

#define sys_trace_idle() SEGGER_SYSVIEW_OnIdle()

#define sys_trace_end_call(id) SEGGER_SYSVIEW_RecordEndCall(id)

#endif /* _TRACE_SYSVIEW_H */
