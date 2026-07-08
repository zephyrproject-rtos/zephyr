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
#include <zephyr/arch/arm64/lib_helpers.h>
#endif
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>

LOG_MODULE_DECLARE(pmu_sampling);

/* PMCNTENSET/PMINTENSET/PMOVS bit for the dedicated cycle counter. */
#define PMU_CYCLE_CNT_BIT BIT(31)

/*
 * reload_period is written once before sampling starts (single writer,
 * multiple readers in ISR). Volatile is sufficient; no atomic needed.
 */
static volatile uint32_t reload_period = 1000U;

/*
 * True when the active sampling source is the dedicated cycle counter
 * (PMCCNTR_EL0) rather than general-purpose event counter 0. Set on each CPU
 * by z_arm64_pmu_sampling_arm_local(); read in the overflow ISR and stop path.
 * Single writer per CPU before sampling starts; volatile is sufficient.
 */
static volatile bool use_cycle_counter;
static atomic_t irq_connected;

static ALWAYS_INLINE void pmu_write_pmintenset_el1(uint64_t v)
{
	__asm__ volatile("msr pmintenset_el1, %0" :: "r"(v) : "memory");
}

static ALWAYS_INLINE void pmu_write_pmintenclr_el1(uint64_t v)
{
	__asm__ volatile("msr pmintenclr_el1, %0" :: "r"(v) : "memory");
}

/*
 * Preload the cycle counter so it overflows after period_events cycles.
 * Writing (0 - period) works for both the 32-bit (PMCR.LC=0) and 64-bit
 * (PMCR.LC=1) cycle counter: in either width the counter wraps past its
 * maximum exactly period_events increments later.
 */
static ALWAYS_INLINE void pmu_cycle_preload(uint32_t period_events)
{
	write_pmccntr_el0((uint64_t)(0ULL - (uint64_t)period_events));
}

static void arm64_pmu_overflow_isr(const void *arg)
{
	uintptr_t pc;

	ARG_UNUSED(arg);

	__asm__ volatile("mrs %0, elr_el1" : "=r"(pc));

	z_pmu_sampling_on_overflow(pc);

	/* Reload with -period so the next sample lands after reload_period
	 * events, then clear the overflow flag for the active source.
	 */
	if (use_cycle_counter) {
		pmu_cycle_preload((uint32_t)reload_period);
		write_pmovsclr_el0(PMU_CYCLE_CNT_BIT);
	} else {
		pmu_counter_clear_overflow(0);
		(void)pmu_counter_write32(0, (uint32_t)(-(uint32_t)reload_period));
	}
}

/*
 * Program and start the local CPU's PMU for overflow sampling. The caller must
 * have pinned the current thread to this CPU (SMP) and initialised the PMU.
 */
void z_arm64_pmu_sampling_arm_local(uint32_t event, uint32_t period_events)
{
	use_cycle_counter = (event == PMU_EVT_CPU_CYCLES);

	pmu_stop();
	pmu_counter_disable_all();

	if (use_cycle_counter) {
		/*
		 * Drive sampling from the dedicated cycle counter. Some ARMv8-A
		 * cores do not increment a general-purpose counter for the
		 * CPU_CYCLES event, so counter 0 would never overflow.
		 */
		pmu_write_pmintenclr_el1(PMU_CYCLE_CNT_BIT);
		write_pmovsclr_el0(PMU_CYCLE_CNT_BIT);
		pmu_cycle_preload(period_events);
		pmu_write_pmintenset_el1(PMU_CYCLE_CNT_BIT);
		/* Cycle counter (bit 31) is enabled by pmu_start(). */
	} else {
		pmu_counter_overflow_interrupt_set(0U, false);
		pmu_counter_reset_all();
		pmu_counter_clear_overflow(0U);
		(void)pmu_counter_config(0U, event);
		(void)pmu_counter_write32(0U, (uint32_t)(-period_events));
		pmu_counter_overflow_interrupt_set(0U, true);
		pmu_counter_enable(0U);
	}

	pmu_start();
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
	if (use_cycle_counter) {
		pmu_write_pmintenclr_el1(PMU_CYCLE_CNT_BIT);
		write_pmcntenclr_el0(PMU_CYCLE_CNT_BIT);
	} else {
		pmu_counter_overflow_interrupt_set(0U, false);
		pmu_counter_disable(0U);
	}
	pmu_stop();
}

#if defined(CONFIG_SMP)
void z_arm64_pmu_sampling_cpu_start(uint32_t event, uint32_t period_events)
{
	/*
	 * Called on each secondary CPU via a per-CPU work item. Each CPU has its
	 * own PMU register file, so PMU init, counter config and IRQ enable must
	 * all run locally on the target CPU. pmu_init() is idempotent per CPU
	 * (each core keeps its own initialised flag), so calling it here safely
	 * initialises this secondary core's PMU if it has not been already.
	 */
	(void)pmu_init();

	/* Enable PMU overflow PPI on this CPU. */
	(void)z_arm64_pmu_sampling_irq_init();

	z_arm64_pmu_sampling_arm_local(event, period_events);
}
#else
void z_arm64_pmu_sampling_cpu_start(uint32_t event, uint32_t period_events)
{
	ARG_UNUSED(event);
	ARG_UNUSED(period_events);
}
#endif /* CONFIG_SMP */
