/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pmu_sampling_priv.h"

#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/barrier.h>
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
/*
 * The ring index is the free-running sample sequence modulo the capacity.
 * Only a power-of-two capacity keeps that mapping stable across the 32-bit
 * sequence wrap (otherwise UINT32_MAX % cap and 0 % cap can select the same
 * slot and alias consecutive samples).
 */
BUILD_ASSERT((CONFIG_PROFILING_PMU_SAMPLE_BUFFER_LEN &
	      (CONFIG_PROFILING_PMU_SAMPLE_BUFFER_LEN - 1)) == 0,
	     "CONFIG_PROFILING_PMU_SAMPLE_BUFFER_LEN must be a power of two");
BUILD_ASSERT(sizeof(struct pmu_sample_record) == 24);

/*
 * Shortest overflow period accepted by pmu_sampling_start(). A very small
 * period can re-overflow the counter on the instructions executed inside the
 * overflow ISR itself (preload, flag clear, return), re-asserting the
 * level-triggered PMU interrupt before the handler returns and starving the
 * rest of the system. This floor is a conservative software guard; a precise
 * minimum depends on the core and has not been characterised on hardware.
 */
#define PMU_SAMPLING_MIN_PERIOD_EVENTS 256U

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

/*
 * Serializes the full init/start/stop state transitions. These configure
 * shared PMU state (the use_cycle_counter selector, per-CPU work items, the
 * one-time work-queue creation) and must not interleave: two concurrent
 * starts could install different events on different CPUs, a restart could be
 * torn down by a still-running auto-stop handler, and two first-time init
 * callers could both create the static work queues. The mutex is recursive,
 * so pmu_sampling_start_inner() may call pmu_sampling_stop() while holding it.
 */
static K_MUTEX_DEFINE(pmu_lock);

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
 * Per-CPU work queues run at a preemptible priority where the configuration
 * provides one, otherwise at the highest cooperative priority. Priority 0 is
 * not a valid application-thread priority when CONFIG_NUM_PREEMPT_PRIORITIES
 * is 0 (it is reserved for the idle thread).
 */
#if CONFIG_NUM_PREEMPT_PRIORITIES > 0
#define PMU_SMP_WQ_PRIO K_PRIO_PREEMPT(0)
#else
#define PMU_SMP_WQ_PRIO K_PRIO_COOP(0)
#endif

/*
 * arch_num_cpus() counts every configured CPU, including zephyr,deferred-start
 * cores that may never come online. A work item dispatched to a queue pinned
 * to an offline CPU never runs, so a later flush would wait forever. Probe each
 * queue once at init and only dispatch to CPUs whose pinned thread actually
 * ran; CPUs brought online after init do not participate in sampling.
 */
static atomic_t cpu_online[CONFIG_MP_MAX_NUM_CPUS];

static bool pmu_sampling_cpu_online(unsigned int cpu)
{
	return smp_cpu_wq[cpu] != NULL && atomic_get(&cpu_online[cpu]) != 0;
}

static struct k_work cpu_probe_work[CONFIG_MP_MAX_NUM_CPUS];
static K_SEM_DEFINE(cpu_probe_sem, 0, CONFIG_MP_MAX_NUM_CPUS);

static void pmu_cpu_probe_work_fn(struct k_work *work)
{
	unsigned int cpu = arch_curr_cpu()->id;

	if (cpu < CONFIG_MP_MAX_NUM_CPUS) {
		atomic_set(&cpu_online[cpu], 1);
	}
	k_sem_give(&cpu_probe_sem);
}

/* Time a pinned high-priority probe is given to run before a CPU is treated
 * as offline.
 */
#define PMU_SMP_PROBE_TIMEOUT K_MSEC(100)

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

		k_tid_t tid;
		int perr;

		k_work_queue_init(wq);
		k_work_queue_start(wq, smp_wq_stacks[cpu],
				   K_THREAD_STACK_SIZEOF(smp_wq_stacks[cpu]),
				   PMU_SMP_WQ_PRIO, &cfg);
		/*
		 * Pin the work queue thread to its target CPU. The kernel
		 * rejects affinity changes for a runnable thread, and the queue
		 * thread is already started here, so suspend it across the pin.
		 */
		tid = k_work_queue_thread_get(wq);
		k_thread_suspend(tid);
		perr = k_thread_cpu_pin(tid, cpu);
		k_thread_resume(tid);
		if (perr != 0) {
			LOG_ERR("PMU: failed to pin work queue to CPU %u (%d)", cpu, perr);
		}
		smp_cpu_wq[cpu] = wq;

		/*
		 * Initialise the per-CPU start/stop work items once. They must
		 * never be re-initialised while queued or running, so start and
		 * stop only set parameters and (re)submit them.
		 */
		k_work_init(&cpu_start_work[cpu].work, pmu_cpu_start_work_fn);
		k_work_init(&cpu_stop_work[cpu], pmu_cpu_stop_work_fn);
		k_work_init(&cpu_probe_work[cpu], pmu_cpu_probe_work_fn);
	}

	/*
	 * Probe which CPUs are actually online: a queue pinned to an offline
	 * (deferred-start) CPU never runs its probe, so cpu_online stays 0 and
	 * start/stop skip it instead of flushing work that can never complete.
	 */
	for (unsigned int cpu = 0; cpu < (unsigned int)arch_num_cpus(); cpu++) {
		if (smp_cpu_wq[cpu] == NULL) {
			continue;
		}
		k_work_submit_to_queue(smp_cpu_wq[cpu], &cpu_probe_work[cpu]);
	}
	for (unsigned int cpu = 0; cpu < (unsigned int)arch_num_cpus(); cpu++) {
		if (smp_cpu_wq[cpu] == NULL) {
			continue;
		}
		if (k_sem_take(&cpu_probe_sem, PMU_SMP_PROBE_TIMEOUT) != 0) {
			LOG_WRN("PMU: CPU appears offline; excluded from sampling");
		}
	}
}
#endif /* CONFIG_SMP */

/*
 * Tear down an active capture. Must be called with pmu_lock held. Does nothing
 * when no capture is active, so an inactive stop cannot clear the global
 * PMCR_EL0.E enable and freeze a measurement another PMU user set up through
 * the plain counter API.
 */
static void pmu_sampling_teardown_locked(void)
{
	if (!atomic_get(&active_flag)) {
		return;
	}

	atomic_set(&active_flag, 0);

	/* Stop PMU on the calling CPU (CPU 0 / shell CPU). */
	z_arm64_pmu_sampling_cpu_stop();

#if defined(CONFIG_SMP)
	struct k_work_sync sync;

	/*
	 * Stop PMU on each online secondary CPU. Each CPU has its own PMU
	 * register file; counter disable and IRQ disable must run locally on
	 * that CPU. Skipping offline (deferred-start) CPUs avoids flushing work
	 * on a pinned queue thread that can never run.
	 */
	for (unsigned int cpu = 0; cpu < (unsigned int)arch_num_cpus(); cpu++) {
		if (!pmu_sampling_cpu_online(cpu)) {
			continue;
		}
		k_work_submit_to_queue(smp_cpu_wq[cpu], &cpu_stop_work[cpu]);
	}

	/*
	 * Wait for every dispatched stop to finish before returning, so a
	 * subsequent start cannot race a still-pending stop (which would clear
	 * the overflow interrupt just after the new session armed it).
	 */
	for (unsigned int cpu = 0; cpu < (unsigned int)arch_num_cpus(); cpu++) {
		if (!pmu_sampling_cpu_online(cpu)) {
			continue;
		}
		(void)k_work_flush(&cpu_stop_work[cpu], &sync);
	}
#endif
}

static void pmu_autostop_work_fn(struct k_work *work)
{
	ARG_UNUSED(work);

	/*
	 * Runs from the system work queue, not from any caller of start/stop.
	 * Take the lock and tear down directly; do not call pmu_sampling_stop(),
	 * which cancel-syncs this very handler and would deadlock.
	 */
	k_mutex_lock(&pmu_lock, K_FOREVER);
	pmu_sampling_teardown_locked();
	k_mutex_unlock(&pmu_lock);
}

/*
 * Snapshot one CPU ring: the oldest-unread index and the number of pending
 * samples. Read the consumer index before the producer index so a concurrent
 * overflow can only advance prod (and, on a full ring, cons) afterwards,
 * keeping prod - cons non-negative. The count is clamped to the capacity,
 * which the ring never exceeds, to absorb a drop that races the two reads.
 */
static uint32_t ring_snapshot(unsigned int c, uint32_t *co_out)
{
	uint32_t co = (uint32_t)atomic_get(&cons[c]);
	uint32_t p = (uint32_t)atomic_get(&prod[c]);
	uint32_t n = p - co;

	*co_out = co;
	return MIN(n, (uint32_t)CONFIG_PROFILING_PMU_SAMPLE_BUFFER_LEN);
}

static size_t total_stored_samples(void)
{
	size_t t = 0;

	for (unsigned int c = 0; c < CONFIG_MP_MAX_NUM_CPUS; c++) {
		uint32_t co;

		t += (size_t)ring_snapshot(c, &co);
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
	uint32_t p, co;
	struct pmu_sample_record *s;

	if (!atomic_get(&active_flag)) {
		return;
	}

	cpu = arch_curr_cpu()->id;
	__ASSERT_NO_MSG(cpu < CONFIG_MP_MAX_NUM_CPUS);

	/*
	 * Single producer per CPU: the overflow ISR cannot nest on its own CPU,
	 * so prod[cpu] is advanced only here. Consumers run on other CPUs and
	 * only read prod[cpu] while advancing cons[cpu].
	 */
	p = (uint32_t)atomic_get(&prod[cpu]);
	co = (uint32_t)atomic_get(&cons[cpu]);

	/*
	 * Buffer full: drop the oldest sample. Advance the consumer index and
	 * the lost count BEFORE overwriting the slot, so a reader never copies
	 * the record currently being rewritten.
	 */
	if ((p - co) >= cap) {
		(void)atomic_inc(&cons[cpu]);
		(void)atomic_inc(&lost_cnt[cpu]);
	}

	s = &cpu_sample_buf[cpu][p % cap];

	s->tstamp = k_cycle_get_64();
	s->pc = (uint64_t)pc;
	s->tid = (uint32_t)(uintptr_t)k_current_get();
	s->cpu = (uint16_t)cpu;
	s->event = (uint16_t)(active_event & 0xFFFFU);

	/*
	 * Publish only after the record is fully written. The fence pairs with
	 * the consumer's read of prod[cpu], so a consumer that observes the new
	 * index also observes a complete record.
	 */
	barrier_dmem_fence_full();
	atomic_set(&prod[cpu], (atomic_val_t)(p + 1U));
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

	k_mutex_lock(&pmu_lock, K_FOREVER);

	/* Re-check under the lock: another caller may have initialised while we
	 * were blocked, so the one-time work-queue creation runs exactly once.
	 */
	if (atomic_get(&init_flag)) {
		k_mutex_unlock(&pmu_lock);
		return 0;
	}

	ret = pmu_init();
	if (ret != 0) {
		goto out;
	}

	if (pmu_num_counters() == 0U) {
		ret = -ENODEV;
		goto out;
	}

	ret = z_arm64_pmu_sampling_irq_init();
	if (ret != 0) {
		LOG_ERR("PMU sampling: IRQ setup failed (%d)", ret);
		goto out;
	}

#if defined(CONFIG_SMP)
	pmu_sampling_smp_queues_init();
#endif

	atomic_set(&init_flag, 1);
out:
	k_mutex_unlock(&pmu_lock);
	return ret;
}

static int pmu_sampling_start_inner(uint32_t event, uint32_t period_events, int auto_stop_s)
{
	int ret;

	if (period_events < PMU_SAMPLING_MIN_PERIOD_EVENTS ||
	    period_events > (UINT32_MAX / 2U)) {
		return -EINVAL;
	}

	ret = pmu_sampling_init();
	if (ret != 0) {
		return ret;
	}

	/*
	 * Flush any auto-stop handler that may already be running before taking
	 * the lock: the handler also takes pmu_lock, so cancel-syncing it while
	 * holding the lock would deadlock. After this returns no auto-stop is
	 * pending or running, so it cannot tear down the session armed below.
	 */
	struct k_work_sync cancel_sync;

	(void)k_work_cancel_delayable_sync(&pmu_autostop_work, &cancel_sync);

	/*
	 * Hold the lock across the whole start transition so two concurrent
	 * starts cannot install different events on different CPUs and a
	 * restart cannot race a stop.
	 */
	k_mutex_lock(&pmu_lock, K_FOREVER);

	/* Tear down any previous session before arming the new one. */
	pmu_sampling_teardown_locked();

	z_arm64_pmu_sampling_set_period(period_events);
	active_event = event;
	active_period = period_events;
	pmu_sampling_clear();

	atomic_set(&active_flag, 1);

#if defined(CONFIG_SMP)
	/*
	 * Configure every CPU's PMU on that CPU's own pinned work-queue thread,
	 * then wait for completion. Running on the pinned thread guarantees the
	 * counter config, GIC PPI enable and pmu_start() all execute on the
	 * target CPU and cannot migrate between register writes. The primary is
	 * included: its per-CPU PMU state is already initialised (so the arch
	 * secondary init is a no-op there), and routing it through the same
	 * flushed path removes the earlier shell-thread pinning that the kernel
	 * silently rejected for a running thread.
	 */
	struct k_work_sync sync;

	for (unsigned int cpu = 0; cpu < (unsigned int)arch_num_cpus(); cpu++) {
		struct pmu_cpu_start_work *w = &cpu_start_work[cpu];

		if (!pmu_sampling_cpu_online(cpu)) {
			continue;
		}
		w->event = event;
		w->period = period_events;
		k_work_submit_to_queue(smp_cpu_wq[cpu], &w->work);
	}
	for (unsigned int cpu = 0; cpu < (unsigned int)arch_num_cpus(); cpu++) {
		if (!pmu_sampling_cpu_online(cpu)) {
			continue;
		}
		(void)k_work_flush(&cpu_start_work[cpu].work, &sync);
	}
#else
	/* Uniprocessor: configure the local PMU directly. */
	(void)z_arm64_pmu_sampling_irq_init();
	z_arm64_pmu_sampling_arm_local(event, period_events);
#endif /* CONFIG_SMP */

	if (auto_stop_s > 0) {
		(void)k_work_reschedule(&pmu_autostop_work, K_SECONDS(auto_stop_s));
	}

	k_mutex_unlock(&pmu_lock);
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
	struct k_work_sync cancel_sync;

	/*
	 * Flush any running/pending auto-stop handler before taking the lock.
	 * This is only ever called from an application or shell thread, never
	 * from the handler itself, so the cancel-sync cannot deadlock.
	 */
	(void)k_work_cancel_delayable_sync(&pmu_autostop_work, &cancel_sync);

	if (!atomic_get(&init_flag)) {
		return;
	}

	k_mutex_lock(&pmu_lock, K_FOREVER);
	pmu_sampling_teardown_locked();
	k_mutex_unlock(&pmu_lock);
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
		uint32_t co;
		uint32_t n = ring_snapshot(c, &co);

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
		uint32_t co;
		uint32_t n = ring_snapshot(c, &co);

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
	uint32_t cap = CONFIG_PROFILING_PMU_SAMPLE_BUFFER_LEN;
	uint32_t snap_co[CONFIG_MP_MAX_NUM_CPUS];
	uint32_t snap_n[CONFIG_MP_MAX_NUM_CPUS];
	size_t ns = 0;
	size_t need;
	size_t lost = total_lost_samples();
	const char *nm;
	size_t nmlen;
	uint8_t *wp;

	if (out == NULL) {
		return -EINVAL;
	}

	/*
	 * Snapshot every CPU's producer/consumer indices once; the capacity
	 * check, header counts and copy loop all use this snapshot. Reading
	 * prod before cons bounds snap_n at the then-current pending count, so a
	 * sample arriving mid-export can only make the copy write fewer records
	 * than reserved, never past the computed size. For a complete capture
	 * the caller should stop sampling first.
	 */
	for (unsigned int c = 0; c < CONFIG_MP_MAX_NUM_CPUS; c++) {
		uint32_t co;

		snap_n[c] = ring_snapshot(c, &co);
		snap_co[c] = co;
		ns += snap_n[c];
	}

	need = ZPERF_TOTAL_HDR + ns * sizeof(struct pmu_sample_record);

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
		uint32_t co = snap_co[c];
		uint32_t n = snap_n[c];

		for (uint32_t k = 0; k < n; k++) {
			memcpy(wp, &cpu_sample_buf[c][(co + k) % cap],
			       sizeof(struct pmu_sample_record));
			wp += sizeof(struct pmu_sample_record);
		}
	}

	return (int)(wp - base);
}
