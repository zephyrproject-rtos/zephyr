/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel_structs.h>
#include <logging/kernel_event_logger.h>
#include <misc/printk.h>
#include <misc/util.h>
#include <zephyr.h>

#include "SEGGER_SYSVIEW.h"
#include "SEGGER_RTT.h"

static u8_t printer_stack[1024];
static u8_t calc_stack[1024];
static u8_t sysview_stack[2048];

static struct k_thread printer_thread_data;
static struct k_thread calc_thread_data;
static struct k_thread sysview_thread_data;

static u32_t timestamp, interrupt;

extern SEGGER_RTT_CB _SEGGER_RTT;

u32_t sysview_get_timestamp(void)
{
	return timestamp;
}

u32_t sysview_get_interrupt(void)
{
	return interrupt;
}

static void publish_context_switch(u32_t *event_data)
{
#if defined(CONFIG_KERNEL_EVENT_LOGGER_CONTEXT_SWITCH)
	SEGGER_SYSVIEW_OnTaskStartExec(event_data[1]);
#endif
}

static void publish_interrupt(u32_t *event_data)
{
#if defined(CONFIG_KERNEL_EVENT_LOGGER_INTERRUPT)
	interrupt = event_data[1];

	/* FIXME: RecordEnter and RecordExit seem to be required to keep
	 * SystemView happy; however, there's currently no way to measure
	 * the time it takes for an ISR to execute.
	 */
	SEGGER_SYSVIEW_RecordEnterISR();
	SEGGER_SYSVIEW_RecordExitISR();
#endif
}

static void publish_sleep(u32_t *event_data)
{
#if defined(CONFIG_KERNEL_EVENT_LOGGER_SLEEP)
	SEGGER_SYSVIEW_OnIdle();
#endif
}

static void publish_task(u32_t *event_data)
{
#if defined(CONFIG_KERNEL_EVENT_LOGGER_THREAD)
	u32_t thread_id;

	thread_id = event_data[1];

	switch ((enum sys_k_event_logger_thread_event)event_data[2]) {
	case KERNEL_LOG_THREAD_EVENT_READYQ:
		SEGGER_SYSVIEW_OnTaskStartReady(thread_id);
		break;
	case KERNEL_LOG_THREAD_EVENT_PEND:
		SEGGER_SYSVIEW_OnTaskStopReady(thread_id, 3<<3);
		break;
	case KERNEL_LOG_THREAD_EVENT_EXIT:
		SEGGER_SYSVIEW_OnTaskTerminate(thread_id);
		break;
	}

	/* FIXME: Maybe we need a KERNEL_LOG_THREAD_EVENT_CREATE? */
#endif
}

static void send_system_desc(void)
{
	SEGGER_SYSVIEW_SendSysDesc("N=ZephyrSysViewSample");
	SEGGER_SYSVIEW_SendSysDesc("D=" CONFIG_BOARD " "
				   CONFIG_SOC " " CONFIG_ARCH);
	SEGGER_SYSVIEW_SendSysDesc("O=Zephyr");
}

static void sysview_api_send_task_list(void)
{
	struct k_thread *thr;

	for (thr = _kernel.threads; thr; thr = thr->next_thread) {
		char name[20];

		snprintk(name, sizeof(name), "T%xE%x", (uintptr_t)thr,
			 (uintptr_t)thr->entry);

		/* NOTE: struct k_thread is inside the stack on Zephyr 1.7.
		 * This is not guaranteed by the API, and is likely to change
		 * in the future.  Hence, StackBase/StackSize are not set here;
		 * these could be stored as part of the kernel event.
		 */
		SEGGER_SYSVIEW_SendTaskInfo(&(SEGGER_SYSVIEW_TASKINFO) {
			.TaskID = (u32_t)(uintptr_t)thr,
			.sName = name,
			.Prio = thr->base.prio,
		});
	}
}

static u32_t zephyr_to_sysview(int event_type)
{
	static const u32_t lut[] = {
		[KERNEL_EVENT_LOGGER_CONTEXT_SWITCH_EVENT_ID] =
			SYSVIEW_EVTMASK_TASK_START_EXEC,
		[KERNEL_EVENT_LOGGER_INTERRUPT_EVENT_ID] =
			SYSVIEW_EVTMASK_ISR_ENTER |
			SYSVIEW_EVTMASK_ISR_EXIT,
		[KERNEL_EVENT_LOGGER_SLEEP_EVENT_ID] =
			SYSVIEW_EVTMASK_IDLE,
		[KERNEL_EVENT_LOGGER_THREAD_EVENT_ID] =
			SYSVIEW_EVTMASK_TASK_CREATE |
			SYSVIEW_EVTMASK_TASK_START_READY |
			SYSVIEW_EVTMASK_TASK_STOP_READY |
			SYSVIEW_EVTMASK_TASK_STOP_EXEC,
	};

	return lut[event_type];
}

#define MUST_LOG(event) \
	(IS_ENABLED(CONFIG_ ## event) && \
	sys_k_must_log_event(event ## _EVENT_ID))

#define ENABLE_SYSVIEW_EVENT(event) \
	(MUST_LOG(event) ? zephyr_to_sysview(event ## _EVENT_ID) : 0)

static void sysview_setup(void)
{
	static const SEGGER_SYSVIEW_OS_API api = {
		.pfGetTime = NULL, /* sysview_get_timestamp() used instead */
		.pfSendTaskList = sysview_api_send_task_list,
	};
	u32_t evs;

	printk("RTT block address is %p\n", &_SEGGER_RTT);

	/* NOTE: SysView does not support dynamic frequency scaling */
	SEGGER_SYSVIEW_Init(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC,
			    CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC,
			    &api, send_system_desc);

#if defined(CONFIG_PHYS_RAM_ADDR)	/* x86 */
	SEGGER_SYSVIEW_SetRAMBase(CONFIG_PHYS_RAM_ADDR);
#elif defined(CONFIG_SRAM_BASE_ADDRESS)	/* arm, default */
	SEGGER_SYSVIEW_SetRAMBase(CONFIG_SRAM_BASE_ADDRESS);
#else
	/* Setting RAMBase is just an optimization: this value is subtracted
	 * from all pointers in order to save bandwidth.  It's not an error
	 * if a platform does not set this value.
	 */
#endif

	evs = SYSVIEW_EVTMASK_PRINT_FORMATTED |
		ENABLE_SYSVIEW_EVENT(KERNEL_EVENT_LOGGER_SLEEP) |
		ENABLE_SYSVIEW_EVENT(KERNEL_EVENT_LOGGER_CONTEXT_SWITCH) |
		ENABLE_SYSVIEW_EVENT(KERNEL_EVENT_LOGGER_INTERRUPT) |
		ENABLE_SYSVIEW_EVENT(KERNEL_EVENT_LOGGER_THREAD);
	SEGGER_SYSVIEW_EnableEvents(evs);
}

#undef ENABLE_SYSVIEW_EVENT
#undef MUST_LOG

static void sysview_thread(void)
{
	sysview_setup();

	for (;;) {
		u32_t event_data[4];
		u16_t event_id;
		u8_t dropped;
		u8_t event_data_size = (u8_t)ARRAY_SIZE(event_data);
		int ret;

		ret = sys_k_event_logger_get_wait(&event_id,
						  &dropped,
						  event_data,
						  &event_data_size);
		if (ret < 0) {
			continue;
		}

		timestamp = event_data[0];

		switch (event_id) {
		case KERNEL_EVENT_LOGGER_CONTEXT_SWITCH_EVENT_ID:
			publish_context_switch(event_data);
			break;
		case KERNEL_EVENT_LOGGER_INTERRUPT_EVENT_ID:
			publish_interrupt(event_data);
			break;
		case KERNEL_EVENT_LOGGER_SLEEP_EVENT_ID:
			publish_sleep(event_data);
			break;
		case KERNEL_EVENT_LOGGER_THREAD_EVENT_ID:
			publish_task(event_data);
			break;
		}
	}
}

static void printer_thread(void)
{
	for (;;) {
		SEGGER_SYSVIEW_Print("Printer thread says hello");
		k_sleep(MSEC_PER_SEC);
	}
}

static void calc_thread(void)
{
	int denom = 0;

	for (;;) {
		const int val = 0xbebacafe;

		denom = (denom + 1) % 16;

		if (denom == 0) {
			SEGGER_SYSVIEW_Warn("Not calculating: denom is 0");
		} else {
			SEGGER_SYSVIEW_PrintfHost("Calculated: %d",
						  val / denom);
		}

		k_sleep(MSEC_PER_SEC);
	}
}

void main(void)
{
	k_thread_create(&sysview_thread_data, sysview_stack,
			K_THREAD_STACK_SIZEOF(sysview_stack),
			(k_thread_entry_t)sysview_thread,
			NULL, NULL, NULL, K_PRIO_COOP(1), 0, 0);

	k_thread_create(&printer_thread_data, printer_stack,
			K_THREAD_STACK_SIZEOF(printer_stack),
			(k_thread_entry_t)printer_thread,
			NULL, NULL, NULL, K_PRIO_COOP(1), 0, 0);

	k_thread_create(&calc_thread_data, calc_stack,
			K_THREAD_STACK_SIZEOF(calc_stack),
			(k_thread_entry_t)calc_thread,
			NULL, NULL, NULL, K_PRIO_COOP(1), 0, 0);
}
