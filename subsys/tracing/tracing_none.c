/*
 * Copyright (c) 2019 Intel corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/tracing/tracing.h>
#include <zephyr/sys/cpu_load.h>

__weak void sys_trace_isr_enter(void)
{
	if (IS_ENABLED(CONFIG_CPU_LOAD_BACKEND_IDLE_HOOK)) {
		/* On architectures that wait for the interrupt with interrupts
		 * enabled, this is what actually closes the idle window: the
		 * idle-exit hook would run too late. It is a no-op when the CPU
		 * was not idle.
		 */
		cpu_load_on_exit_idle();
	}
}

__weak void sys_trace_isr_exit(void) {}

__weak void sys_trace_isr_exit_to_scheduler(void) {}

__weak void sys_trace_idle(void)
{
	if (IS_ENABLED(CONFIG_CPU_LOAD_BACKEND_IDLE_HOOK)) {
		cpu_load_on_enter_idle();
	}
}

__weak void sys_trace_idle_exit(void)
{
	if (IS_ENABLED(CONFIG_CPU_LOAD_BACKEND_IDLE_HOOK)) {
		cpu_load_on_exit_idle();
	}
}
