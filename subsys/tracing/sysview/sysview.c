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
