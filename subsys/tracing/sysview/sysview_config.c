/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <kernel_internal.h>
#include <SEGGER_SYSVIEW.h>
#ifdef CONFIG_SYMTAB
#include <zephyr/debug/symtab.h>
#include <zephyr/sw_isr_table.h>
#endif

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
				   CONFIG_SOC_FAMILY " " CONFIG_ARCH);
	SEGGER_SYSVIEW_SendSysDesc("O=Zephyr");

#ifdef CONFIG_BOARD_QUALIFIERS
	SEGGER_SYSVIEW_SendSysDesc("C=" CONFIG_BOARD_QUALIFIERS);
#endif

#ifdef CONFIG_SYMTAB
	char isr_desc[SEGGER_SYSVIEW_MAX_STRING_LEN];

	for (int idx = 0; idx < IRQ_TABLE_SIZE; idx++) {
		const struct _isr_table_entry *entry = &_sw_isr_table[idx];

		if ((entry->isr == z_irq_spurious) || (entry->isr == NULL)) {
			continue;
		}
		const char *name = symtab_find_symbol_name((uintptr_t)entry->isr, NULL);

		snprintf(isr_desc, SEGGER_SYSVIEW_MAX_STRING_LEN, "I#%d=%s", idx + 16, name);

		SEGGER_SYSVIEW_SendSysDesc(isr_desc);
	}
#endif
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
#if DT_NODE_HAS_STATUS_OKAY(DT_CHOSEN(zephyr_sram))
	SEGGER_SYSVIEW_SetRAMBase(DT_REG_ADDR(DT_CHOSEN(zephyr_sram)));
#else
	/* Setting RAMBase is just an optimization: this value is subtracted
	 * from all pointers in order to save bandwidth.  It's not an error
	 * if a platform does not set this value.
	 */
#endif
}
