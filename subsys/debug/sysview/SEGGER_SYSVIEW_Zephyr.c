
#include <zephyr.h>
#include <kernel_structs.h>
#include <init.h>

#include <systemview/SEGGER_SYSVIEW.h>
#include <systemview/Global.h>
#include "SEGGER_SYSVIEW_Zephyr.h"

static u32_t interrupt;



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

void sys_trace_thread_switched_in(void)
{
	struct k_thread *thread;

	thread = k_current_get();

	if (is_idle_thread(thread)) {
		SEGGER_SYSVIEW_OnIdle();
	} else {
		SEGGER_SYSVIEW_OnTaskStartExec((u32_t)(uintptr_t)thread);
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

void sys_trace_thread_priority_get(struct k_thread *thread)
{
}

void sys_trace_thread_priority_set(struct k_thread *thread)
{
}

void sys_trace_thread_create(struct k_thread *thread)
{
	SEGGER_SYSVIEW_OnTaskCreate((u32_t)(uintptr_t)thread);
	sys_trace_thread_info(thread);
}

void sys_trace_thread_info(struct k_thread *thread)
{
	char name[20];

	if (thread->name == NULL) {
		snprintk(name, sizeof(name), "T%xE%x", (uintptr_t)thread,
			 (uintptr_t)&thread->entry);
	} else {
		snprintk(name, sizeof(name), "%s", thread->name);
	}

	SEGGER_SYSVIEW_TASKINFO Info;

	Info.TaskID = (u32_t)(uintptr_t)thread;
	Info.sName = name;
	Info.Prio = thread->base.prio;
	Info.StackBase = thread->stack_info.size;
	Info.StackSize = thread->stack_info.start;
	SEGGER_SYSVIEW_SendTaskInfo(&Info);
}

void sys_trace_thread_abort(struct k_thread *thread)
{
}

void sys_trace_thread_cancel(struct k_thread *thread)
{
}

void sys_trace_thread_suspend(struct k_thread *thread)
{
}

void sys_trace_thread_resume(struct k_thread *thread)
{
}

void sys_trace_thread_ready(struct k_thread *thread)
{
	SEGGER_SYSVIEW_OnTaskStartReady((u32_t)(uintptr_t)thread);
}

void sys_trace_thread_pend(struct k_thread *thread)
{
	SEGGER_SYSVIEW_OnTaskStopReady((u32_t)(uintptr_t)thread, 3 << 3);
}

void sys_trace_void(unsigned int id)
{
	SEGGER_SYSVIEW_RecordVoid(id);
}

void sys_trace_idle(void)
{
	SEGGER_SYSVIEW_OnIdle();
}

void sys_trace_end_call(unsigned int id)
{
	SEGGER_SYSVIEW_RecordEndCall(id);
}

u32_t sysview_get_timestamp(void)
{
	return k_cycle_get_32();
}

u32_t sysview_get_interrupt(void)
{
	return interrupt;
}

static void send_task_list_cb(void)
{
	struct k_thread *thread;

	for (thread = _kernel.threads; thread; thread = thread->next_thread) {
		char name[20];

		if (is_idle_thread(thread)) {
			continue;
		}

		if (thread->name == NULL) {
			snprintk(name, sizeof(name), "T%xE%x", (uintptr_t)thread,
				 (uintptr_t)&thread->entry);
		} else {
			snprintk(name, sizeof(name), "%s", thread->name);
		}
		SEGGER_SYSVIEW_SendTaskInfo(&(SEGGER_SYSVIEW_TASKINFO) {
			.TaskID = (u32_t)(uintptr_t)thread,
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


static int sysview_init(struct device *arg)
{
	ARG_UNUSED(arg);

	SEGGER_SYSVIEW_Conf();
	SEGGER_SYSVIEW_Start();
	return 0;
}


SYS_INIT(sysview_init, PRE_KERNEL_1, 0);
