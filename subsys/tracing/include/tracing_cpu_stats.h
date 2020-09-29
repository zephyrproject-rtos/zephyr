/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _TRACE_CPU_STATS_H
#define _TRACE_CPU_STATS_H
#include <kernel.h>
#include <kernel_structs.h>
#include <init.h>

#ifdef __cplusplus
extern "C" {
#endif

struct cpu_stats {
	uint64_t idle;
	uint64_t non_idle;
	uint64_t sched;
};

void sys_trace_thread_switched_in(void);
void sys_trace_thread_switched_out(void);
void sys_trace_isr_enter(void);
void sys_trace_isr_exit(void);
void sys_trace_idle(void);

void cpu_stats_get_ns(struct cpu_stats *cpu_stats_ns);
uint32_t cpu_stats_non_idle_and_sched_get_percent(void);
void cpu_stats_reset_counters(void);

#define sys_trace_isr_exit_to_scheduler()

#define sys_trace_thread_priority_set(thread)
#define sys_trace_thread_info(thread)
#define sys_trace_thread_create(thread)
#define sys_trace_thread_abort(thread)
#define sys_trace_thread_suspend(thread)
#define sys_trace_thread_resume(thread)
#define sys_trace_thread_ready(thread)
#define sys_trace_thread_pend(thread)
#define sys_trace_thread_name_set(thread)

#define sys_trace_void(id)
#define sys_trace_end_call(id)
#define sys_trace_semaphore_init(sem)
#define sys_trace_semaphore_take(sem)
#define sys_trace_semaphore_give(sem)
#define sys_trace_mutex_init(mutex)
#define sys_trace_mutex_lock(mutex)
#define sys_trace_mutex_unlock(mutex)

#ifdef __cplusplus
}
#endif

#endif /* _TRACE_CPU_STATS_H */
