/*
 * Copyright (c) 2018 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <kernel_structs.h>
#include <init.h>

#include <ctf_middle.h>
#include "ctf_top.h"


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

void sys_trace_thread_switched_out(void)
{
	struct k_thread *thread = k_current_get();

	ctf_middle_thread_switched_out((u32_t)(uintptr_t)thread);
}

void sys_trace_thread_switched_in(void)
{
	struct k_thread *thread = k_current_get();

	ctf_middle_thread_switched_in((u32_t)(uintptr_t)thread);
}

void sys_trace_thread_priority_set(struct k_thread *thread)
{
	ctf_middle_thread_priority_set((u32_t)(uintptr_t)thread,
				       thread->base.prio);
}

void sys_trace_thread_create(struct k_thread *thread)
{
	ctf_bounded_string_t name = { "Unnamed thread" };

#if defined(CONFIG_THREAD_NAME)
	if (thread->name != NULL) {
		strncpy(name.buf, thread->name, sizeof(name.buf));
		/* strncpy may not always null-terminate */
		name.buf[sizeof(name.buf) - 1] = 0;
	}
#endif

	ctf_middle_thread_create(
		(u32_t)(uintptr_t)thread,
		thread->base.prio,
		name
		);

#if defined(CONFIG_THREAD_STACK_INFO)
	ctf_middle_thread_info(
		(u32_t)(uintptr_t)thread,
		thread->stack_info.size,
		thread->stack_info.start
		);
#endif
}

void sys_trace_thread_abort(struct k_thread *thread)
{
	ctf_middle_thread_abort((u32_t)(uintptr_t)thread);
}

void sys_trace_thread_suspend(struct k_thread *thread)
{
	ctf_middle_thread_suspend((u32_t)(uintptr_t)thread);
}

void sys_trace_thread_resume(struct k_thread *thread)
{
	ctf_middle_thread_resume((u32_t)(uintptr_t)thread);
}

void sys_trace_thread_ready(struct k_thread *thread)
{
	ctf_middle_thread_ready((u32_t)(uintptr_t)thread);
}

void sys_trace_thread_pend(struct k_thread *thread)
{
	ctf_middle_thread_pend((u32_t)(uintptr_t)thread);
}

void sys_trace_thread_info(struct k_thread *thread)
{
#if defined(CONFIG_THREAD_STACK_INFO)
	ctf_middle_thread_info(
		(u32_t)(uintptr_t)thread,
		thread->stack_info.size,
		thread->stack_info.start
		);
#endif
}

void sys_trace_isr_enter(void)
{
	ctf_middle_isr_enter();
}

void sys_trace_isr_exit(void)
{
	ctf_middle_isr_exit();
}

void sys_trace_isr_exit_to_scheduler(void)
{
	ctf_middle_isr_exit_to_scheduler();
}

void sys_trace_idle(void)
{
	ctf_middle_idle();
}

void sys_trace_void(unsigned int id)
{
	ctf_middle_void(id);
}

void sys_trace_end_call(unsigned int id)
{
	ctf_middle_end_call(id);
}


void z_sys_trace_thread_switched_out(void)
{
	sys_trace_thread_switched_out();
}
void z_sys_trace_thread_switched_in(void)
{
	sys_trace_thread_switched_in();
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
void z_sys_trace_idle(void)
{
	sys_trace_idle();
}


static int ctf_top_init(struct device *arg)
{
	ARG_UNUSED(arg);

	ctf_bottom_configure();
	ctf_bottom_start();
	return 0;
}

SYS_INIT(ctf_top_init, PRE_KERNEL_1, 0);
