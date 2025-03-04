/*
 * Copyright (c) 2025 NVIDIA Corporation <jholdsworth@nvidia.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/irq.h>

#include <zephyr/tracing/tracing.h>

#include <openrisc/openriscregs.h>

static ALWAYS_INLINE void openrisc_idle(unsigned int key)
{
	sys_trace_idle();

	/* unlock interrupts */
	irq_unlock(key);

	/* wait for interrupt */
	if (openrisc_read_spr(SPR_UPR) & SPR_UPR_PMP) {
		openrisc_write_spr(SPR_PMR, openrisc_read_spr(SPR_PMR) | SPR_PMR_DME);
	}
}

#ifndef CONFIG_ARCH_HAS_CUSTOM_CPU_IDLE
void arch_cpu_idle(void)
{
	openrisc_idle(1);
}
#endif

#ifndef CONFIG_ARCH_HAS_CUSTOM_CPU_ATOMIC_IDLE
void arch_cpu_atomic_idle(unsigned int key)
{
	openrisc_idle(key);
}
#endif
