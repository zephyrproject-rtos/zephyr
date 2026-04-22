/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/pmu.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/arch/arm64/lib_helpers.h>
#include <errno.h>

LOG_MODULE_REGISTER(pmu, CONFIG_ARM64_PMUV3_LOG_LEVEL);

/*
 * PMU logical state is per CPU: each core has its own PMU bank, and
 * initialization or calibration on one CPU must not short-circuit another.
 */
struct pmu_cpu_state {
	uint32_t num_counters;
	uint32_t implementer;
	uint32_t idcode;
	uint32_t cpu_freq_mhz;
	atomic_t initialized;
};

static struct pmu_cpu_state pmu_state_cpus[CONFIG_MP_MAX_NUM_CPUS];

static ALWAYS_INLINE struct pmu_cpu_state *pmu_curr_state(void)
{
	/*
	 * Use arch_curr_cpu()->id, not CPU_ID. CPU_ID expands to _current_cpu
	 * which asserts !z_smp_cpu_mobile(); preemptible threads (e.g. main)
	 * are "mobile" in SMP builds and would trip that assert even though
	 * reading the PMU for the CPU we are actually running on is correct.
	 */
	return &pmu_state_cpus[arch_curr_cpu()->id];
}

/* Event name lookup table */
struct event_info {
	uint32_t code;
	const char *name;
};

static const struct event_info event_names[] = {{PMU_EVT_SW_INCR, "SW_INCR"},
						{PMU_EVT_L1I_CACHE_REFILL, "L1I_CACHE_REFILL"},
						{PMU_EVT_L1I_TLB_REFILL, "L1I_TLB_REFILL"},
						{PMU_EVT_L1D_CACHE_REFILL, "L1D_CACHE_REFILL"},
						{PMU_EVT_L1D_CACHE, "L1D_CACHE"},
						{PMU_EVT_L1D_TLB_REFILL, "L1D_TLB_REFILL"},
						{PMU_EVT_INST_RETIRED, "INST_RETIRED"},
						{PMU_EVT_EXC_TAKEN, "EXC_TAKEN"},
						{PMU_EVT_EXC_RETURN, "EXC_RETURN"},
						{PMU_EVT_BR_MIS_PRED, "BR_MIS_PRED"},
						{PMU_EVT_CPU_CYCLES, "CPU_CYCLES"},
						{PMU_EVT_BR_PRED, "BR_PRED"},
						{PMU_EVT_MEM_ACCESS, "MEM_ACCESS"},
						{PMU_EVT_L1I_CACHE, "L1I_CACHE"},
						{PMU_EVT_L1D_CACHE_WB, "L1D_CACHE_WB"},
						{PMU_EVT_L2D_CACHE, "L2D_CACHE"},
						{PMU_EVT_L2D_CACHE_REFILL, "L2D_CACHE_REFILL"},
						{PMU_EVT_L2D_CACHE_WB, "L2D_CACHE_WB"},
						{PMU_EVT_BUS_ACCESS, "BUS_ACCESS"},
						{PMU_EVT_MEMORY_ERROR, "MEMORY_ERROR"},
						{PMU_EVT_INST_SPEC, "INST_SPEC"},
						{PMU_EVT_TTBR_WRITE_RETIRED, "TTBR_WRITE_RETIRED"},
						{PMU_EVT_BUS_CYCLES, "BUS_CYCLES"}};

const char *arch_pmu_event_name(uint32_t event)
{
	for (size_t i = 0; i < ARRAY_SIZE(event_names); i++) {
		if (event_names[i].code == event) {
			return event_names[i].name;
		}
	}
	return "UNKNOWN";
}

/*
 * Calibrate CPU clock frequency by comparing PMCCNTR_EL0 (CPU cycles)
 * against CNTVCT_EL0 (system timer ticks at CNTFRQ_EL0 Hz).
 * We run the cycle counter for a short interval and compute:
 *
 *   CPU_MHz = (pmu_delta * cntfrq) / (cnt_delta * 1,000,000)
 *
 * This provides a runtime measurement that works on any platform,
 * eliminating the need for a static Kconfig frequency setting.
 */
static void pmu_calibrate_cpu_freq(void)
{
	uint64_t cntfrq = read_cntfrq_el0();
	uint64_t pmcr_save = read_pmcr_el0();
	uint64_t cnt_start, pmu_start, pmu_end, cnt_end;
	uint64_t wait_ticks, cnt_now;
	uint64_t pmu_delta, cnt_delta;

	/* Enable cycle counter, reset it, start counting */
	write_pmcr_el0(pmcr_save | PMCR_E | PMCR_C | PMCR_LC);
	barrier_isync_fence_full();
	write_pmcntenset_el0(BIT(31));

	cnt_start = read_cntvct_el0();
	pmu_start = read_pmccntr_el0();

	/*
	 * Spin for ~1 ms worth of system timer ticks.
	 * cntfrq / 1000 ticks = 1 ms.
	 * Bound the loop to prevent hanging on broken platforms where
	 * the virtual counter never advances.
	 */
	wait_ticks = cntfrq / 1000;

	uint32_t max_spins = 10000000U;

	do {
		cnt_now = read_cntvct_el0();
		if (--max_spins == 0) {
			LOG_ERR("CPU frequency calibration timed out "
				"(virtual counter stalled)");
			pmu_curr_state()->cpu_freq_mhz = 0;
			write_pmcr_el0(pmcr_save);
			barrier_isync_fence_full();
			write_pmcntenclr_el0(BIT(31));
			return;
		}
	} while ((cnt_now - cnt_start) < wait_ticks);

	pmu_end = read_pmccntr_el0();
	cnt_end = cnt_now;

	/* Restore PMCR (stop counting, clear enable bit) */
	write_pmcr_el0(pmcr_save);
	barrier_isync_fence_full();
	write_pmcntenclr_el0(BIT(31));

	pmu_delta = pmu_end - pmu_start;
	cnt_delta = cnt_end - cnt_start;

	if (cnt_delta > 0 && cntfrq > 0) {
		pmu_curr_state()->cpu_freq_mhz =
			(uint32_t)((pmu_delta * cntfrq) / (cnt_delta * 1000000ULL));
		LOG_INF("CPU frequency calibrated: %u MHz "
			"(pmu_delta=%llu, cnt_delta=%llu, cntfrq=%llu)",
			pmu_curr_state()->cpu_freq_mhz, pmu_delta, cnt_delta, cntfrq);
	} else {
		pmu_curr_state()->cpu_freq_mhz = 0;
		LOG_WRN("CPU frequency calibration failed "
			"(cnt_delta=%llu, cntfrq=%llu)",
			cnt_delta, cntfrq);
	}
}

int arch_pmu_init(void)
{
	uint64_t pmcr;
	uint64_t id_aa64dfr0;
	uint8_t pmu_ver;
	struct pmu_cpu_state *st = pmu_curr_state();

	if (atomic_get(&st->initialized)) {
		return st->num_counters > 0U ? 0 : -ENOTSUP;
	}

	/* Probe ID_AA64DFR0_EL1: PMU version field 0 means no PMU at this EL */
	id_aa64dfr0 = read_id_aa64dfr0_el1();
	pmu_ver = (id_aa64dfr0 >> 8) & 0xF;

	if (pmu_ver == 0 || pmu_ver == 0xF) {
		LOG_ERR("PMU not available: ID_AA64DFR0_EL1.PMUver=0x%x "
			"(not implemented or not visible at this exception level; "
			"models may omit PMU, or EL2/firmware may trap accesses)",
			pmu_ver);
		st->num_counters = 0;
		atomic_set(&st->initialized, 1);
		return -ENOTSUP;
	}

	LOG_DBG("PMU version: 0x%x (PMUv3)", pmu_ver);

	pmcr = read_pmcr_el0();

	st->num_counters = (pmcr >> PMCR_N_SHIFT) & PMCR_N_MASK;
	st->implementer = (pmcr >> PMCR_IMP_SHIFT) & PMCR_IMP_MASK;
	st->idcode = (pmcr >> PMCR_IDCODE_SHIFT) & PMCR_IDCODE_MASK;

	if (st->num_counters == 0) {
		LOG_ERR("PMU reports zero event counters (PMCR_EL0=0x%llx); "
			"may be disabled by firmware/hypervisor",
			pmcr);
		atomic_set(&st->initialized, 1);
		return -ENOTSUP;
	}

	/*
	 * Reset all counters and enable the 64-bit cycle counter.
	 * ISB required after PMCR write for reset to take effect (ARM DDI0487).
	 */
	write_pmcr_el0(PMCR_P | PMCR_C | PMCR_LC);
	barrier_isync_fence_full();

	/* Clear all overflow flags */
	write_pmovsclr_el0(0xFFFFFFFFUL);

	/* Disable all counters initially */
	write_pmcntenclr_el0(0xFFFFFFFFUL);

	/* Enable user-mode access if configured */
#ifdef CONFIG_ARM64_PMUV3_USER_ACCESS
	write_pmuserenr_el0(PMUSERENR_EN | PMUSERENR_SW | PMUSERENR_CR | PMUSERENR_ER);
#else
	write_pmuserenr_el0(0x00UL);
#endif

	/* Calibrate CPU clock frequency from PMU cycle counter vs system timer */
	pmu_calibrate_cpu_freq();

	/*
	 * Publish after all per-CPU fields are committed so other code on this
	 * CPU can safely read counter count and calibration.
	 */
	atomic_set(&st->initialized, 1);

	LOG_INF("PMU initialized: %u counters, implementer=0x%02x, id=0x%02x", st->num_counters,
		st->implementer, st->idcode);

	return 0;
}

uint32_t arch_pmu_num_counters(void)
{
	return pmu_curr_state()->num_counters;
}

uint32_t arch_pmu_cpu_freq_mhz(void)
{
	return pmu_curr_state()->cpu_freq_mhz;
}

void arch_pmu_get_info(pmu_info_t *info)
{
	struct pmu_cpu_state *st = pmu_curr_state();

	if (info == NULL) {
		return;
	}

	info->implementer = st->implementer;
	info->idcode = st->idcode;
}

int arch_pmu_counter_config(uint32_t counter, pmu_evt_t event)
{
	unsigned int key;

	if (!atomic_get(&pmu_curr_state()->initialized)) {
		return -ENODEV;
	}

	if (counter >= pmu_curr_state()->num_counters) {
		return -EINVAL;
	}

	/*
	 * The PMSELR → PMXEVTYPER sequence must be atomic w.r.t. interrupts.
	 * An interrupt handler that modifies PMSELR between the select and the
	 * type write would corrupt the configuration.
	 */
	key = arch_irq_lock();
	write_pmselr_el0(counter);
	barrier_isync_fence_full();
	write_pmxevtyper_el0((uint64_t)(uint32_t)event);
	barrier_isync_fence_full();
	arch_irq_unlock(key);

	LOG_DBG("Counter %u configured for event 0x%02x (%s)", counter, event,
		arch_pmu_event_name(event));

	return 0;
}

void arch_pmu_counter_enable(uint32_t counter)
{
	if (counter < pmu_curr_state()->num_counters) {
		write_pmcntenset_el0(BIT(counter));
	}
}

void arch_pmu_counter_disable(uint32_t counter)
{
	if (counter < pmu_curr_state()->num_counters) {
		write_pmcntenclr_el0(BIT(counter));
	}
}

void arch_pmu_counter_enable_all(void)
{
	uint32_t mask = (1U << pmu_curr_state()->num_counters) - 1;

	/* Enable all event counters + cycle counter (bit 31) */
	write_pmcntenset_el0(mask | BIT(31));
}

void arch_pmu_counter_disable_all(void)
{
	write_pmcntenclr_el0(0xFFFFFFFFUL);
}

uint64_t arch_pmu_counter_read(uint32_t counter)
{
	unsigned int key;
	uint64_t value;

	if (!atomic_get(&pmu_curr_state()->initialized) ||
	    counter >= pmu_curr_state()->num_counters) {
		return 0;
	}

	/*
	 * The PMSELR → PMXEVCNTR read sequence must be atomic w.r.t. interrupts
	 * to prevent an interrupt from changing PMSELR between the select and
	 * the read, which would return the wrong counter's value.
	 */
	key = arch_irq_lock();
	write_pmselr_el0(counter);
	barrier_isync_fence_full();
	value = read_pmxevcntr_el0();
	arch_irq_unlock(key);

	return value;
}

void arch_pmu_counter_reset(uint32_t counter)
{
	unsigned int key;

	if (!atomic_get(&pmu_curr_state()->initialized) ||
	    counter >= pmu_curr_state()->num_counters) {
		return;
	}

	/* Select + write sequence must be atomic w.r.t. interrupts */
	key = arch_irq_lock();
	write_pmselr_el0(counter);
	barrier_isync_fence_full();
	write_pmxevcntr_el0(0UL);
	barrier_isync_fence_full();
	arch_irq_unlock(key);
}

void arch_pmu_counter_reset_all(void)
{
	unsigned int key;
	uint64_t pmcr;

	/*
	 * Read-modify-write of PMCR_EL0 must be atomic w.r.t. interrupts to
	 * prevent an ISR from changing PMCR between the read and write.
	 */
	key = arch_irq_lock();
	pmcr = read_pmcr_el0();
	write_pmcr_el0(pmcr | PMCR_P);
	barrier_isync_fence_full();
	arch_irq_unlock(key);
}

uint64_t arch_pmu_cycle_count(void)
{
	return read_pmccntr_el0();
}

void arch_pmu_cycle_reset(void)
{
	unsigned int key;
	uint64_t pmcr;

	key = arch_irq_lock();
	pmcr = read_pmcr_el0();
	write_pmcr_el0(pmcr | PMCR_C);
	barrier_isync_fence_full();
	arch_irq_unlock(key);
}

void arch_pmu_start(void)
{
	unsigned int key;
	uint64_t pmcr;

	key = arch_irq_lock();
	pmcr = read_pmcr_el0();
	write_pmcr_el0(pmcr | PMCR_E);
	barrier_isync_fence_full();
	arch_irq_unlock(key);

	/* Ensure cycle counter is enabled (PMCNTENSET is write-set-only) */
	write_pmcntenset_el0(BIT(31));
}

void arch_pmu_stop(void)
{
	unsigned int key;
	uint64_t pmcr;

	key = arch_irq_lock();
	pmcr = read_pmcr_el0();
	write_pmcr_el0(pmcr & ~PMCR_E);
	barrier_isync_fence_full();
	arch_irq_unlock(key);
}

bool arch_pmu_counter_overflow(uint32_t counter)
{
	uint64_t ovsr;

	if (!atomic_get(&pmu_curr_state()->initialized) ||
	    counter >= pmu_curr_state()->num_counters) {
		return false;
	}

	ovsr = read_pmovsclr_el0();
	return (ovsr & BIT(counter)) != 0;
}

void arch_pmu_counter_clear_overflow(uint32_t counter)
{
	if (!atomic_get(&pmu_curr_state()->initialized) ||
	    counter >= pmu_curr_state()->num_counters) {
		return;
	}

	/* Write 1 to clear overflow bit */
	write_pmovsclr_el0(BIT(counter));
}
