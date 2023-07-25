/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/kernel_structs.h>
#include <zephyr/init.h>
#include <ksched.h>

#include <SEGGER_SYSVIEW.h>


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
	SEGGER_SYSVIEW_OnIdle();
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
