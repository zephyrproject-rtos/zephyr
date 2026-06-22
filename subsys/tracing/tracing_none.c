/*
 * Copyright (c) 2019 Intel corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/tracing/tracing.h>
#include <zephyr/debug/cpu_load.h>

__weak void sys_trace_isr_enter(void) {}

__weak void sys_trace_isr_exit(void) {}

__weak void sys_trace_isr_exit_to_scheduler(void) {}

__weak void sys_trace_idle(void)
{
	if (IS_ENABLED(CONFIG_CPU_LOAD)) {
		cpu_load_on_enter_idle();
	}
}

__weak void sys_trace_idle_exit(void)
{
	if (IS_ENABLED(CONFIG_CPU_LOAD)) {
		cpu_load_on_exit_idle();
	}
}
