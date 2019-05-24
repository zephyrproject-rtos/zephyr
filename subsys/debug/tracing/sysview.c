/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>
#include <kernel_structs.h>
#include <init.h>

#include <SEGGER_SYSVIEW.h>
#include <Global.h>
#include "SEGGER_SYSVIEW_Zephyr.h"

static u32_t interrupt;

u32_t sysview_get_timestamp(void)
{
	return k_cycle_get_32();
}

u32_t sysview_get_interrupt(void)
{
	return interrupt;
}

void z_sys_trace_idle(void)
{
	sys_trace_idle();
}

void z_sys_trace_isr_enter(void)
{
	sys_trace_isr_enter();
}

void z_sys_trace_isr_exit(void)
{
	sys_trace_isr_exit();
}

void z_sys_trace_isr_exit_to_scheduler(void)
{
	sys_trace_isr_exit_to_scheduler();
}

void z_sys_trace_thread_switched_in(void)
{
	sys_trace_thread_switched_in();
}

void z_sys_trace_thread_switched_out(void)
{
	sys_trace_thread_switched_out();
}

static void send_task_list_cb(void)
{
	struct k_thread *thread;

	for (thread = _kernel.threads; thread; thread = thread->next_thread) {
		char name[20];
		const char *tname = k_thread_name_get(thread);

		if (is_idle_thread(thread)) {
			continue;
		}

		snprintk(name, sizeof(name), "T%xE%x", (uintptr_t)thread,
			 (uintptr_t)&thread->entry);
		SEGGER_SYSVIEW_SendTaskInfo(&(SEGGER_SYSVIEW_TASKINFO) {
			.TaskID = (u32_t)(uintptr_t)thread,
		      .sName = tname?tname:name,
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


static int sysview_init(struct device *arg)
{
	ARG_UNUSED(arg);

	SEGGER_SYSVIEW_Conf();
	if (IS_ENABLED(CONFIG_SEGGER_SYSTEMVIEW_BOOT_ENABLE)) {
		SEGGER_SYSVIEW_Start();
	}
	return 0;
}


SYS_INIT(sysview_init, POST_KERNEL, 0);
