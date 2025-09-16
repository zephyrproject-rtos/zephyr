/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <kernel_internal.h>
#include <zephyr/kernel_structs.h>
#include <zephyr/init.h>
#include <zephyr/debug/cpu_load.h>

#include <SEGGER_SYSVIEW.h>

#define NAMED_EVENT_MAXSTR 20 /* Maximum string length supported by named event */

static uint32_t interrupt;

uint32_t sysview_get_timestamp(void)
{
	return k_cycle_get_32();
}

uint32_t sysview_get_interrupt(void)
{
#ifdef CONFIG_CPU_CORTEX_M
	interrupt = ((SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk) >>
		     SCB_ICSR_VECTACTIVE_Pos);
#endif
	return interrupt;
}

void sys_trace_k_thread_switched_in(void)
{
	struct k_thread *thread;

	thread = k_current_get();

	if (z_is_idle_thread_object(thread)) {
		SEGGER_SYSVIEW_OnIdle();
	} else {
		SEGGER_SYSVIEW_OnTaskStartExec((uint32_t)(uintptr_t)thread);
	}
}

void sys_trace_k_thread_switched_out(void)
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
#ifdef CONFIG_TRACING_IDLE
	SEGGER_SYSVIEW_OnIdle();
#endif

	if (IS_ENABLED(CONFIG_CPU_LOAD)) {
		cpu_load_on_enter_idle();
	}
}

void sys_trace_idle_exit(void)
{
	if (IS_ENABLED(CONFIG_CPU_LOAD)) {
		cpu_load_on_exit_idle();
	}
}

void sys_trace_named_event(const char *name, uint32_t arg0, uint32_t arg1)
{
	/* Based on SEGGER provided code for user defined packets */
	uint8_t a_packet[SEGGER_SYSVIEW_INFO_SIZE + 2 *
		SEGGER_SYSVIEW_QUANTA_U32 + NAMED_EVENT_MAXSTR + 1];
	uint8_t *payload;

	payload = SEGGER_SYSVIEW_PREPARE_PACKET(a_packet);
	payload = SEGGER_SYSVIEW_EncodeString(payload, name, NAMED_EVENT_MAXSTR);
	payload = SEGGER_SYSVIEW_EncodeU32(payload, arg0);
	payload = SEGGER_SYSVIEW_EncodeU32(payload, arg1);
	SEGGER_SYSVIEW_SendPacket(a_packet, payload, TID_NAMED_EVENT);
}

static int sysview_init(void)
{

	SEGGER_SYSVIEW_Conf();
	if (IS_ENABLED(CONFIG_SEGGER_SYSTEMVIEW_BOOT_ENABLE)) {
		SEGGER_SYSVIEW_Start();
	}
	return 0;
}


SYS_INIT(sysview_init, POST_KERNEL, 0);
