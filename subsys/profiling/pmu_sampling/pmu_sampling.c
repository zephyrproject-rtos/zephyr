/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pmu_sampling_priv.h"

#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/profiling/pmu_sampling.h>
#include <zephyr/arch/pmu.h>
#if defined(CONFIG_ARM64)
#include <zephyr/arch/arm64/pmuv3.h>
#endif
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

LOG_MODULE_REGISTER(pmu_sampling);

BUILD_ASSERT(CONFIG_PROFILING_PMU_SAMPLE_BUFFER_LEN > 0);
BUILD_ASSERT(sizeof(struct pmu_sample_record) == 24);

#define ZPERF_MAGIC "ZPERFV01"
#define ZPERF_FILE_HDR_SIZE 32
#define ZPERF_EVENT_BLK_SIZE 36
#define ZPERF_TOTAL_HDR (ZPERF_FILE_HDR_SIZE + ZPERF_EVENT_BLK_SIZE)

BUILD_ASSERT(sizeof(ZPERF_MAGIC) == 9);

static struct pmu_sample_record
	cpu_sample_buf[CONFIG_MP_MAX_NUM_CPUS][CONFIG_PROFILING_PMU_SAMPLE_BUFFER_LEN];

static atomic_t prod[CONFIG_MP_MAX_NUM_CPUS];
static atomic_t cons[CONFIG_MP_MAX_NUM_CPUS];
static atomic_t lost_cnt[CONFIG_MP_MAX_NUM_CPUS];

static atomic_t init_flag;
static atomic_t active_flag;
static uint32_t active_event;
static uint32_t active_period;

static void pmu_autostop_work_fn(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(pmu_autostop_work, pmu_autostop_work_fn);

#if defined(CONFIG_SMP)
/*
 * Per-CPU work queues: one thread per secondary CPU, each pinned to its CPU
 * via CONFIG_SCHED_CPU_MASK. Created once at init time by
 * pmu_sampling_smp_queues_init(); NULL until then.
 */
#define PMU_SMP_STACK_SIZE 2048

static struct k_work_q smp_wq_storage[CONFIG_MP_MAX_NUM_CPUS];
static K_THREAD_STACK_ARRAY_DEFINE(smp_wq_stacks, CONFIG_MP_MAX_NUM_CPUS,
				   PMU_SMP_STACK_SIZE);
static struct k_work_q *smp_cpu_wq[CONFIG_MP_MAX_NUM_CPUS];

/*
 * Per-CPU work items used to program the PMU on each secondary CPU.
 * Each CPU's PMU register file is independent; counter config and IRQ
 * enable must run locally on that CPU.
 */
struct pmu_cpu_start_work {
	struct k_work work;
	uint32_t event;
	uint32_t period;
};

static struct pmu_cpu_start_work cpu_start_work[CONFIG_MP_MAX_NUM_CPUS];

static void pmu_cpu_start_work_fn(struct k_work *work)
{
	struct pmu_cpu_start_work *w =
		CONTAINER_OF(work, struct pmu_cpu_start_work, work);

	z_arm64_pmu_sampling_cpu_start(w->event, w->period);
}

static struct k_work cpu_stop_work[CONFIG_MP_MAX_NUM_CPUS];

static void pmu_cpu_stop_work_fn(struct k_work *work)
{
	ARG_UNUSED(work);
	z_arm64_pmu_sampling_cpu_stop();
}

static void pmu_sampling_smp_queues_init(void)
{
	for (unsigned int cpu = 0; cpu < (unsigned int)arch_num_cpus(); cpu++) {
		struct k_work_queue_config cfg = {
			.name = "pmu_smp",
			.no_yield = false,
		};
		struct k_work_q *wq = &smp_wq_storage[cpu];

		k_work_queue_init(wq);
		k_work_queue_start(wq, smp_wq_stacks[cpu],
				   K_THREAD_STACK_SIZEOF(smp_wq_stacks[cpu]),
				   K_PRIO_PREEMPT(0), &cfg);
		/* Pin the work queue thread to the target CPU. */
		k_thread_cpu_pin(k_work_queue_thread_get(wq), cpu);
		smp_cpu_wq[cpu] = wq;
	}
}
#endif /* CONFIG_SMP */

static void pmu_autostop_work_fn(struct k_work *work)
{
	ARG_UNUSED(work);
	pmu_sampling_stop();
}

static size_t total_stored_samples(void)
{
	size_t t = 0;

	for (unsigned int c = 0; c < CONFIG_MP_MAX_NUM_CPUS; c++) {
		uint32_t p = (uint32_t)atomic_get(&prod[c]);
		uint32_t co = (uint32_t)atomic_get(&cons[c]);

		t += (size_t)(p - co);
	}
	return t;
}

static size_t total_lost_samples(void)
{
	size_t t = 0;

	for (unsigned int c = 0; c < CONFIG_MP_MAX_NUM_CPUS; c++) {
		t += (size_t)atomic_get(&lost_cnt[c]);
	}
	return t;
}

void z_pmu_sampling_on_overflow(uintptr_t pc)
{
	unsigned int cpu;
	uint32_t cap = CONFIG_PROFILING_PMU_SAMPLE_BUFFER_LEN;
	atomic_val_t prev;
	uint32_t idx;
	struct pmu_sample_record *s;

	if (!atomic_get(&active_flag)) {
		return;
	}

	cpu = arch_curr_cpu()->id;
	__ASSERT_NO_MSG(cpu < CONFIG_MP_MAX_NUM_CPUS);

	prev = atomic_inc(&prod[cpu]);
	idx = (uint32_t)prev % cap;
	s = &cpu_sample_buf[cpu][idx];

	s->tstamp = k_cycle_get_64();
	s->pc = (uint64_t)pc;
	s->tid = (uint32_t)(uintptr_t)k_current_get();
	s->cpu = (uint16_t)cpu;
	s->event = (uint16_t)(active_event & 0xFFFFU);

	for (;;) {
		uint32_t cval = (uint32_t)atomic_get(&cons[cpu]);
		uint32_t pend = (uint32_t)(prev + 1U) - cval;

		if (pend <= cap) {
			break;
		}
		(void)atomic_inc(&cons[cpu]);
		(void)atomic_inc(&lost_cnt[cpu]);
	}
}

int pmu_sampling_period_from_hz(unsigned int sample_hz, uint32_t *period_events)
{
	uint32_t mhz;
	uint64_t pe;

	if (sample_hz == 0U || period_events == NULL) {
		return -EINVAL;
	}

	mhz = arch_pmu_cpu_freq_mhz();
	if (mhz == 0U) {
		return -ENOTSUP;
	}

	pe = ((uint64_t)mhz * 1000000ULL) / (uint64_t)sample_hz;
	if (pe == 0ULL) {
		pe = 1ULL;
	}
	if (pe > (uint64_t)(UINT32_MAX / 2U)) {
		return -ERANGE;
	}

	*period_events = (uint32_t)pe;
	return 0;
}

int pmu_sampling_init(void)
{
	int ret;

	if (atomic_get(&init_flag)) {
		return 0;
	}

	ret = pmu_init();
	if (ret != 0) {
		return ret;
	}

	if (pmu_num_counters() == 0U) {
		return -ENODEV;
	}

	ret = z_arm64_pmu_sampling_irq_init();
	if (ret != 0) {
		LOG_ERR("PMU sampling: IRQ setup failed (%d)", ret);
		return ret;
	}

#if defined(CONFIG_SMP)
	pmu_sampling_smp_queues_init();
#endif

	atomic_set(&init_flag, 1);
	return 0;
}

static int pmu_sampling_start_inner(uint32_t event, uint32_t period_events, int auto_stop_s)
{
	int ret;

	if (period_events == 0U || period_events > (UINT32_MAX / 2U)) {
		return -EINVAL;
	}

	unsigned int primary_cpu;

	ret = pmu_sampling_init();
	if (ret != 0) {
		return ret;
	}

	/*
	 * Stop the primary CPU's PMU directly — do NOT call pmu_sampling_stop()
	 * here because that would dispatch stop work items to ALL secondary CPUs
	 * (indices 1-3) via the pmu_smp work queues.  If the shell thread is
	 * running on CPU 1, 2, or 3, the stop work item for that CPU would run
	 * on the pmu_smp thread (also on that CPU) and clear PMINTENSET_EL1,
	 * disabling the PMU overflow interrupt just after the shell thread
	 * started it.  Result: only ~8 samples collected (the few overflows that
	 * occur before the stop work is processed) then silence.
	 *
	 * The full pmu_sampling_stop() with SMP dispatch is still correct for
	 * the auto-stop path (where all CPUs are actively sampling and all must
	 * be stopped).
	 */
	(void)k_work_cancel_delayable(&pmu_autostop_work);
	atomic_set(&active_flag, 0);
	z_arm64_pmu_sampling_cpu_stop();	/* stops primary CPU only */

	/*
	 * Pin the shell thread to the current CPU for the entire PMU setup
	 * sequence.  Without pinning, the thread can migrate between any two
	 * of the PMU register writes (every arch_irq_lock/unlock pair is a
	 * preemption point on SMP), causing counter config, PMINTENSET, and
	 * pmu_start() to execute on different CPUs.  The GIC GICR PPI enable
	 * (irq_enable) and all PMU hardware writes must land on the same CPU
	 * so that the overflow ISR is delivered there.
	 *
	 * k_thread_cpu_pin() works even if the thread blocks inside (e.g., for
	 * LOG_INF's shell mutex); the thread resumes on the pinned CPU.  Unpin
	 * after pmu_start() so normal scheduling resumes.
	 */
	primary_cpu = arch_curr_cpu()->id;
#if defined(CONFIG_SMP)
	k_thread_cpu_pin(k_current_get(), (int)primary_cpu);
#endif /* CONFIG_SMP */

	/* Re-ensure PMU is initialised on THIS pinned CPU. */
	ret = pmu_init();
	if (ret != 0) {
#if defined(CONFIG_SMP)
		k_thread_cpu_mask_enable_all(k_current_get());
#endif /* CONFIG_SMP */
		return ret;
	}

	z_arm64_pmu_sampling_set_period(period_events);

	active_event = event;
	active_period = period_events;
	pmu_sampling_clear();

	/* irq_enable on THIS pinned CPU — GICR PPI armed for primary_cpu. */
	(void)z_arm64_pmu_sampling_irq_init();

	/*
	 * Program and start the local (pinned) CPU's PMU. The backend selects
	 * the dedicated cycle counter for CPU_CYCLES and general-purpose
	 * counter 0 for all other events.
	 */
	z_arm64_pmu_sampling_arm_local(event, period_events);

	/* All PMU writes done on primary_cpu — unpin so shell can roam freely. */
#if defined(CONFIG_SMP)
	k_thread_cpu_mask_enable_all(k_current_get());
#endif /* CONFIG_SMP */

	atomic_set(&active_flag, 1);

#if defined(CONFIG_SMP)
	/*
	 * Dispatch start work to every secondary CPU except the primary.  The
	 * primary CPU's PMU was already configured above by the shell thread.
	 * Sending a stop work item to the primary's pmu_smp queue (which would
	 * happen if we called pmu_sampling_stop() earlier) and then a start
	 * item is the root cause of "8 samples then silence": the stop work
	 * clears PMINTENSET_EL1 on the primary CPU ~8 ms after the shell
	 * started it, killing the ISR.  Skipping the primary here prevents any
	 * pmu_smp work from touching the primary CPU's PMU registers.
	 */
	for (unsigned int cpu = 0; cpu < (unsigned int)arch_num_cpus(); cpu++) {
		struct pmu_cpu_start_work *w = &cpu_start_work[cpu];

		if (cpu == primary_cpu) {
			/* already configured locally; pmu_smp must not touch it */
			continue;
		}
		if (smp_cpu_wq[cpu] == NULL) {
			LOG_WRN("PMU: no work queue for CPU %u, skipping", cpu);
			continue;
		}
		k_work_init(&w->work, pmu_cpu_start_work_fn);
		w->event = event;
		w->period = period_events;
		k_work_submit_to_queue(smp_cpu_wq[cpu], &w->work);
	}
#endif /* CONFIG_SMP */

	if (auto_stop_s > 0) {
		(void)k_work_reschedule(&pmu_autostop_work, K_SECONDS(auto_stop_s));
	}

	return 0;
}

int pmu_sampling_start_cfg(const struct pmu_sampling_start_cfg *cfg)
{
	if (cfg == NULL) {
		return -EINVAL;
	}

	return pmu_sampling_start_inner(cfg->event, cfg->period_events, cfg->auto_stop_s);
}

int pmu_sampling_start(uint32_t event, uint32_t period_events)
{
	struct pmu_sampling_start_cfg cfg = {
		.event = event,
		.period_events = period_events,
		.auto_stop_s = -1,
	};

	return pmu_sampling_start_cfg(&cfg);
}

bool pmu_sampling_is_active(void)
{
	return atomic_get(&active_flag) != 0;
}

bool z_pmu_sampling_is_active(void)
{
	return atomic_get(&active_flag) != 0;
}

void pmu_sampling_stop(void)
{
	(void)k_work_cancel_delayable(&pmu_autostop_work);

	if (!atomic_get(&init_flag)) {
		return;
	}

	atomic_set(&active_flag, 0);

	/* Stop PMU on the calling CPU (CPU 0 / shell CPU). */
	z_arm64_pmu_sampling_cpu_stop();

#if defined(CONFIG_SMP)
	/*
	 * Stop PMU on each secondary CPU.  Each CPU has its own PMU register
	 * file; counter disable and IRQ disable must run locally on that CPU.
	 */
	for (unsigned int cpu = 0; cpu < (unsigned int)arch_num_cpus(); cpu++) {
		if (smp_cpu_wq[cpu] == NULL) {
			continue;
		}
		k_work_init(&cpu_stop_work[cpu], pmu_cpu_stop_work_fn);
		k_work_submit_to_queue(smp_cpu_wq[cpu], &cpu_stop_work[cpu]);
	}
#endif
}

void pmu_sampling_clear(void)
{
	for (unsigned int c = 0; c < CONFIG_MP_MAX_NUM_CPUS; c++) {
		atomic_set(&prod[c], 0);
		atomic_set(&cons[c], 0);
		atomic_set(&lost_cnt[c], 0);
	}
}

size_t pmu_sampling_count(void)
{
	return total_stored_samples();
}

void pmu_sampling_get_stats(struct pmu_sampling_stats *st)
{
	uint32_t cap = CONFIG_PROFILING_PMU_SAMPLE_BUFFER_LEN;
	size_t stored = 0;
	size_t lost_total = 0;
	unsigned int max_pct = 0;

	if (st == NULL) {
		return;
	}

	for (unsigned int c = 0; c < CONFIG_MP_MAX_NUM_CPUS; c++) {
		uint32_t p = (uint32_t)atomic_get(&prod[c]);
		uint32_t co = (uint32_t)atomic_get(&cons[c]);
		uint32_t n = p - co;

		stored += n;
		lost_total += (size_t)atomic_get(&lost_cnt[c]);

		if (cap > 0U) {
			unsigned int pct = (unsigned int)((100ULL * (uint64_t)n) / (uint64_t)cap);

			if (pct > max_pct) {
				max_pct = pct;
			}
		}
	}

	st->stored_samples = stored;
	st->lost_samples = lost_total;
	st->buffer_high_water_pct = max_pct;
}

size_t pmu_sampling_copy(struct pmu_sample_record *out, size_t max)
{
	size_t copied = 0;
	uint32_t cap = CONFIG_PROFILING_PMU_SAMPLE_BUFFER_LEN;

	if (out == NULL || max == 0U) {
		return 0U;
	}

	for (unsigned int c = 0; c < CONFIG_MP_MAX_NUM_CPUS && copied < max; c++) {
		uint32_t co = (uint32_t)atomic_get(&cons[c]);
		uint32_t p = (uint32_t)atomic_get(&prod[c]);
		uint32_t n = p - co;

		for (uint32_t k = 0; k < n && copied < max; k++) {
			out[copied++] = cpu_sample_buf[c][(co + k) % cap];
		}
	}

	return copied;
}

size_t pmu_sampling_export_zperf_size(void)
{
	return ZPERF_TOTAL_HDR + total_stored_samples() * sizeof(struct pmu_sample_record);
}

int pmu_sampling_export_zperf(void *out, size_t out_len)
{
	uint8_t *base = out;
	size_t ns = total_stored_samples();
	size_t need = ZPERF_TOTAL_HDR + ns * sizeof(struct pmu_sample_record);
	size_t lost = total_lost_samples();
	uint32_t cap = CONFIG_PROFILING_PMU_SAMPLE_BUFFER_LEN;
	const char *nm;
	size_t nmlen;
	uint8_t *wp;

	if (out == NULL) {
		return -EINVAL;
	}

	if (ns > (size_t)UINT32_MAX) {
		return -EFBIG;
	}

	if (out_len < need) {
		return -ENOSPC;
	}

	if (need > (size_t)INT_MAX) {
		return -EFBIG;
	}

	memcpy(base, ZPERF_MAGIC, 8);
	sys_put_le32(1, base + 8);
	sys_put_le32(ZPERF_TOTAL_HDR, base + 12);
	sys_put_le32((uint32_t)ns, base + 16);
	sys_put_le32((uint32_t)sizeof(struct pmu_sample_record), base + 20);
	sys_put_le32((uint32_t)MIN(lost, (size_t)UINT32_MAX), base + 24);
	sys_put_le32(0, base + 28);

	sys_put_le32(active_event, base + ZPERF_FILE_HDR_SIZE);
	memset(base + ZPERF_FILE_HDR_SIZE + 4, 0, 32);
	nm = arch_pmu_event_name(active_event);
	nmlen = strlen(nm);
	if (nmlen > 31U) {
		nmlen = 31U;
	}
	memcpy(base + ZPERF_FILE_HDR_SIZE + 4, nm, nmlen);

	wp = base + ZPERF_TOTAL_HDR;

	for (unsigned int c = 0; c < CONFIG_MP_MAX_NUM_CPUS; c++) {
		uint32_t co = (uint32_t)atomic_get(&cons[c]);
		uint32_t p = (uint32_t)atomic_get(&prod[c]);
		uint32_t n = p - co;

		for (uint32_t k = 0; k < n; k++) {
			memcpy(wp, &cpu_sample_buf[c][(co + k) % cap],
			       sizeof(struct pmu_sample_record));
			wp += sizeof(struct pmu_sample_record);
		}
	}

	return (int)(wp - base);
}
