/*
 * Copyright (c) 2024 Infineon Technologies AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "soc.h"
#include <zephyr/irq.h>
#include <zephyr/tracing/tracing.h>
#include <zephyr/arch/common/sys_io.h>
#include <zephyr/arch/tricore/arch_inlines.h>

#define SCU_BASE 0xF0036000

void __weak arch_cpu_idle(void)
{
	sys_trace_idle();
	irq_unlock((1 << 15));
	/* Set sleep */
	aurix_cpu_endinit_enable(false);
	sys_write32(0x1, SCU_BASE + 0xC8 + arch_proc_id() * 0x4);
	aurix_cpu_endinit_enable(true);
	/* Wait for write to propagate */
	__asm volatile(
		"	dsync\n"
		"	isync\n"
		::: "memory"
	);
}

void __weak arch_cpu_atomic_idle(unsigned int key)
{
	sys_trace_idle();
	irq_unlock(key);
	/* Set sleep */
	aurix_cpu_endinit_enable(false);
	sys_write32(0x1, SCU_BASE + 0xC8 + arch_proc_id() * 0x4);
	aurix_cpu_endinit_enable(true);
	/* Wait for write to propagate */
	__asm volatile(
		"	dsync\n"
		"	isync\n"
		::: "memory"
	);
}
