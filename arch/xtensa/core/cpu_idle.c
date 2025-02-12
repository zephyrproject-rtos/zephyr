/*
 * Copyright (c) 2016 Cadence Design Systems, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/tracing/tracing.h>

#ifndef CONFIG_ARCH_CPU_IDLE_CUSTOM
void arch_cpu_idle(void)
{
	sys_trace_idle();
	__asm__ volatile ("waiti 0");
}
#endif

void arch_cpu_atomic_idle(unsigned int key)
{
	sys_trace_idle();
	__asm__ volatile ("waiti 0\n\t"
			  "wsr.ps %0\n\t"
			  "rsync" :: "a"(key));
}
