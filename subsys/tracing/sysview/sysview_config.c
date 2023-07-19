/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <SEGGER_SYSVIEW.h>
#include <ksched.h>

extern const SEGGER_SYSVIEW_OS_API SYSVIEW_X_OS_TraceAPI;

#if CONFIG_THREAD_MAX_NAME_LEN
#define THREAD_NAME_LEN CONFIG_THREAD_MAX_NAME_LEN
#else
#define THREAD_NAME_LEN 20
#endif


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


static void cbSendSystemDesc(void)
{
	SEGGER_SYSVIEW_SendSysDesc("N=" CONFIG_SEGGER_SYSVIEW_APP_NAME);
	SEGGER_SYSVIEW_SendSysDesc("D=" CONFIG_BOARD " "
				   CONFIG_SOC_SERIES " " CONFIG_ARCH);
	SEGGER_SYSVIEW_SendSysDesc("O=Zephyr");
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



void SEGGER_SYSVIEW_Conf(void)
{
	SEGGER_SYSVIEW_Init(sys_clock_hw_cycles_per_sec(),
			    sys_clock_hw_cycles_per_sec(),
			    &SYSVIEW_X_OS_TraceAPI, cbSendSystemDesc);
#if DT_NODE_HAS_STATUS(DT_CHOSEN(zephyr_sram), okay)
	SEGGER_SYSVIEW_SetRAMBase(DT_REG_ADDR(DT_CHOSEN(zephyr_sram)));
#else
	/* Setting RAMBase is just an optimization: this value is subtracted
	 * from all pointers in order to save bandwidth.  It's not an error
	 * if a platform does not set this value.
	 */
#endif
}
