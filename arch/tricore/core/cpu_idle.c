/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/irq.h>
#include <zephyr/tracing/tracing.h>

void __weak arch_cpu_idle(void)
{
	sys_trace_idle();
	irq_unlock((1 << 15));

	__asm volatile("wait");
}

void __weak arch_cpu_atomic_idle(unsigned int key)
{
	/*
	 * ENABLE takes effect on the instruction after it, so a request that
	 * becomes pending before that point is serviced before WAIT could run
	 * and the caller then sleeps past the wake-up it was waiting for.
	 * Reading the pending state first does not close that window, so do
	 * not enter WAIT here.  arch_cpu_idle() still waits.
	 */
	irq_unlock(key);
}
