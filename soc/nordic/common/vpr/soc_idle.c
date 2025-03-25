/*
 * Copyright (C) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/irq.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/tracing/tracing.h>
#include "hal/nrf_vpr.h"
/*
 * Due to a HW issue, VPR requires MSTATUS.MIE to be enabled when entering sleep.
 * Otherwise it would not wake up.
 */
void arch_cpu_idle(void)
{
	sys_trace_idle();
	barrier_dsync_fence_full();
	irq_unlock(MSTATUS_IEN);
	__asm__ volatile("wfi");

	/* MLTPAN-18 workaround */
#if defined(CONFIG_RISCV_CORE_NORDIC_VPR_USE_MLTPAN_18)
	nrf_vpr_cpurun_set(NRF_VPR, true);
#endif
}

void arch_cpu_atomic_idle(unsigned int key)
{
	sys_trace_idle();
	barrier_dsync_fence_full();
	irq_unlock(MSTATUS_IEN);
	__asm__ volatile("wfi");

	/* MLTPAN-18 workaround */
#if defined(CONFIG_RISCV_CORE_NORDIC_VPR_USE_MLTPAN_18)
	nrf_vpr_cpurun_set(NRF_VPR, true);
#endif

	/* Disable interrupts if needed. */
	__asm__ volatile ("csrc mstatus, %0"
			  :
			  : "r" (~key & MSTATUS_IEN)
			  : "memory");
}
