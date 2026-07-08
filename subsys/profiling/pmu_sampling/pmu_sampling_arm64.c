/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pmu_sampling_priv.h"

#include <zephyr/devicetree.h>
#include <zephyr/profiling/pmu_sampling.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/arch/pmu.h>
#if defined(CONFIG_ARM64)
#include <zephyr/arch/arm64/pmuv3.h>
#endif
#include <zephyr/sys/atomic.h>

LOG_MODULE_DECLARE(pmu_sampling);

/*
 * reload_period is written once before sampling starts (single writer,
 * multiple readers in ISR). Volatile is sufficient; no atomic needed.
 */
static volatile uint32_t reload_period = 1000U;
static atomic_t irq_connected;

static void arm64_pmu_overflow_isr(const void *arg)
{
	uintptr_t pc;

	ARG_UNUSED(arg);

	__asm__ volatile("mrs %0, elr_el1" : "=r"(pc));

	z_pmu_sampling_on_overflow(pc);

	pmu_counter_clear_overflow(0);

	/* Reload the counter with -period so it overflows again after
	 * reload_period events, producing the next periodic sample.
	 */
	(void)pmu_counter_write32(0, (uint32_t)(-(uint32_t)reload_period));
}

#if DT_HAS_COMPAT_STATUS_OKAY(arm_armv8_pmu)
#define PMU_IRQ DT_IRQN(DT_COMPAT_GET_ANY_STATUS_OKAY(arm_armv8_pmu))
#elif defined(CONFIG_ARM64_PMU_OVERFLOW_IRQ) && (CONFIG_ARM64_PMU_OVERFLOW_IRQ >= 0)
#define PMU_IRQ CONFIG_ARM64_PMU_OVERFLOW_IRQ
#endif

int z_arm64_pmu_sampling_irq_init(void)
{
#if !defined(PMU_IRQ)
	LOG_ERR("PMU sampling: enable arm,armv8-pmu in DTS or set CONFIG_ARM64_PMU_OVERFLOW_IRQ");
	return -ENODEV;
#else
	if (atomic_cas(&irq_connected, 0, 1) == false) {
		/*
		 * IRQ_CONNECT runs once (it is a compile-time macro that fills
		 * the ISR table). irq_enable() must still be called on every
		 * CPU for PPIs, so fall through after the first call.
		 */
		irq_enable(PMU_IRQ);
		return 0;
	}

	IRQ_CONNECT(PMU_IRQ, IRQ_DEFAULT_PRIORITY, arm64_pmu_overflow_isr, NULL, 0);
	irq_enable(PMU_IRQ);

	LOG_INF("PMU overflow IRQ %d connected", PMU_IRQ);
	return 0;
#endif
}

void z_arm64_pmu_sampling_set_period(uint32_t period_events)
{
	reload_period = period_events;
}

void z_arm64_pmu_sampling_cpu_stop(void)
{
	pmu_counter_overflow_interrupt_set(0U, false);
	pmu_counter_disable(0U);
	pmu_stop();
}

#if defined(CONFIG_SMP)
void z_arm64_pmu_sampling_cpu_start(uint32_t event, uint32_t period_events)
{
	/*
	 * Called on each secondary CPU via IPI work item. Each CPU has its own
	 * PMU register file, so counter config and IRQ enable must run locally.
	 *
	 * Use arch_pmu_init_secondary() instead of pmu_init(): secondary CPUs
	 * do not need frequency calibration (period_events is pre-computed by
	 * the primary CPU) and must not emit LOG_INF output since this work
	 * item runs at high priority in a work-queue thread; with
	 * LOG_MODE_IMMEDIATE + SHELL_LOG_BACKEND the logging path can block on
	 * the shell transport mutex held by the lower-priority shell thread,
	 * causing a priority-inversion stall that hangs the shell.
	 */
	arch_pmu_init_secondary();
	pmu_stop();
	pmu_counter_disable_all();
	pmu_counter_overflow_interrupt_set(0, false);
	pmu_counter_reset_all();
	pmu_counter_clear_overflow(0);

	(void)pmu_counter_config(0U, event);
	(void)pmu_counter_write32(0U, (uint32_t)(-period_events));

	/* Enable PMU overflow PPI on this CPU. */
	(void)z_arm64_pmu_sampling_irq_init();

	pmu_counter_overflow_interrupt_set(0U, true);
	pmu_counter_enable(0U);
	pmu_start();
}
#else
void z_arm64_pmu_sampling_cpu_start(uint32_t event, uint32_t period_events)
{
	ARG_UNUSED(event);
	ARG_UNUSED(period_events);
}
#endif /* CONFIG_SMP */
