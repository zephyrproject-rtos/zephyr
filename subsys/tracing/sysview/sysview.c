/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>
#include <kernel_structs.h>
#include <init.h>
#include <ksched.h>

#include <SEGGER_SYSVIEW.h>
#include <Global.h>
#include "SEGGER_SYSVIEW_Zephyr.h"

#if CONFIG_THREAD_MAX_NAME_LEN
#define THREAD_NAME_LEN CONFIG_THREAD_MAX_NAME_LEN
#else
#define THREAD_NAME_LEN 20
#endif

static uint32_t interrupt;

uint32_t sysview_get_timestamp(void)
{
	return k_cycle_get_32();
}

uint32_t sysview_get_interrupt(void)
{
	return interrupt;
}

void sys_trace_thread_switched_in(void)
{
	struct k_thread *thread;

	thread = k_current_get();

	if (z_is_idle_thread_object(thread)) {
		SEGGER_SYSVIEW_OnIdle();
	} else {
		SEGGER_SYSVIEW_OnTaskStartExec((uint32_t)(uintptr_t)thread);
	}
}

void sys_trace_thread_switched_out(void)
{
	SEGGER_SYSVIEW_OnTaskStopExec();
}

void sys_trace_isr_enter(void)
{
	SEGGER_SYSVIEW_RecordEnterISR();
}

void sys_trace_isr_exit(void)
{
	SEGGER_SYSVIEW_RecordExitISR();
}

void sys_trace_isr_exit_to_scheduler(void)
{
	SEGGER_SYSVIEW_RecordExitISRToScheduler();
}

void sys_trace_idle(void)
{
	SEGGER_SYSVIEW_OnIdle();
}


void sys_trace_semaphore_init(struct k_sem *sem)
{
	SEGGER_SYSVIEW_RecordU32(
		SYS_TRACE_ID_SEMA_INIT, (uint32_t)(uintptr_t)sem);
}

void sys_trace_semaphore_take(struct k_sem *sem)
{
	SEGGER_SYSVIEW_RecordU32(
		SYS_TRACE_ID_SEMA_TAKE, (uint32_t)(uintptr_t)sem);
}

void sys_trace_semaphore_give(struct k_sem *sem)
{
	SEGGER_SYSVIEW_RecordU32(
		SYS_TRACE_ID_SEMA_GIVE, (uint32_t)(uintptr_t)sem);
}

void sys_trace_mutex_init(struct k_mutex *mutex)
{
	SEGGER_SYSVIEW_RecordU32(
		SYS_TRACE_ID_MUTEX_INIT, (uint32_t)(uintptr_t)mutex);
}

void sys_trace_mutex_lock(struct k_mutex *mutex)
{
	SEGGER_SYSVIEW_RecordU32(
		SYS_TRACE_ID_MUTEX_LOCK, (uint32_t)(uintptr_t)mutex);
}

void sys_trace_mutex_unlock(struct k_mutex *mutex)
{
	SEGGER_SYSVIEW_RecordU32(
		SYS_TRACE_ID_MUTEX_UNLOCK, (uint32_t)(uintptr_t)mutex);
}

static void set_thread_name(char *name, struct k_thread *thread)
{
	const char *tname = k_thread_name_get(thread);

	if (tname != NULL && tname[0] != '\0') {
		memcpy(name, tname, THREAD_NAME_LEN);
		name[THREAD_NAME_LEN - 1] = '\0';
	} else {
		snprintk(name, THREAD_NAME_LEN, "T%pE%p",
		thread, &thread->entry);
	}
}

void sys_trace_thread_info(struct k_thread *thread)
{
	char name[THREAD_NAME_LEN];

	set_thread_name(name, thread);

	SEGGER_SYSVIEW_TASKINFO Info;

	Info.TaskID = (uint32_t)(uintptr_t)thread;
	Info.sName = name;
	Info.Prio = thread->base.prio;
	Info.StackBase = thread->stack_info.size;
	Info.StackSize = thread->stack_info.start;
	SEGGER_SYSVIEW_SendTaskInfo(&Info);
}



static void send_task_list_cb(void)
{
	struct k_thread *thread;

	for (thread = _kernel.threads; thread; thread = thread->next_thread) {
		char name[THREAD_NAME_LEN];

		if (z_is_idle_thread_object(thread)) {
			continue;
		}

		set_thread_name(name, thread);

		SEGGER_SYSVIEW_SendTaskInfo(&(SEGGER_SYSVIEW_TASKINFO) {
			.TaskID = (uint32_t)(uintptr_t)thread,
			.sName = name,
			.StackSize = thread->stack_info.size,
			.StackBase = thread->stack_info.start,
			.Prio = thread->base.prio,
		});
	}
}


static U64 get_time_cb(void)
{
	return (U64)k_cycle_get_32();
}


const SEGGER_SYSVIEW_OS_API SYSVIEW_X_OS_TraceAPI = {
	get_time_cb,
	send_task_list_cb,
};


static int sysview_init(const struct device *arg)
{
	ARG_UNUSED(arg);

	SEGGER_SYSVIEW_Conf();
	if (IS_ENABLED(CONFIG_SEGGER_SYSTEMVIEW_BOOT_ENABLE)) {
		SEGGER_SYSVIEW_Start();
	}
	return 0;
}


SYS_INIT(sysview_init, POST_KERNEL, 0);
