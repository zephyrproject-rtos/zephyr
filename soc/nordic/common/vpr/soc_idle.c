/*
 * Copyright (C) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/irq.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/tracing/tracing.h>
#include <nrfx.h>

/* Optionally apply errata workaround for waking up from hibernation */
static ALWAYS_INLINE void hibernate_dummy_write(void)
{
	if (!IS_ENABLED(CONFIG_NORDIC_VPR_HIBERNATE)) {
		return;
	}

	if (NRF_ERRATA_DYNAMIC_CHECK(54H, 111) || NRF_ERRATA_DYNAMIC_CHECK(54L, 18)) {
		*(volatile int *)0x5004C804 = 1;
	}
}

/*
 * Due to a HW issue, VPR requires MSTATUS.MIE to be enabled when entering sleep.
 * Otherwise it would not wake up.
 */
void arch_cpu_idle(void)
{
#if defined(CONFIG_TRACING)
	sys_trace_idle();
#endif

	if (NRF_ERRATA_DYNAMIC_CHECK(54H, 93)) {
		barrier_dsync_fence_full();
		irq_unlock(MSTATUS_IEN);
	}

	__asm__ volatile("wfi");
	hibernate_dummy_write();
	if (!NRF_ERRATA_DYNAMIC_CHECK(54H, 93)) {
#if defined(CONFIG_TRACING)
		/* Makes sense only if sleeping with interrupts locked. */
		sys_trace_idle_exit();
#endif
		irq_unlock(MSTATUS_IEN);
	}
}

void arch_cpu_atomic_idle(unsigned int key)
{
#if defined(CONFIG_TRACING)
	sys_trace_idle();
#endif
	if (NRF_ERRATA_DYNAMIC_CHECK(54H, 93)) {
		barrier_dsync_fence_full();
		irq_unlock(MSTATUS_IEN);
	}
	__asm__ volatile("wfi");
	hibernate_dummy_write();

	if (NRF_ERRATA_DYNAMIC_CHECK(54H, 93)) {
		/* Disable interrupts if needed. */
		__asm__ volatile ("csrc mstatus, %0"
			  :
			  : "r" (~key & MSTATUS_IEN)
			  : "memory");
	} else {
#if defined(CONFIG_TRACING)
		/* Makes sense only if sleeping with interrupts locked. */
		sys_trace_idle_exit();
#endif
		irq_unlock(key);
	}
}
