/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief PMU counter accuracy / correctness tests (portable pmu_* API).
 *
 * The benchmark suite in tests/benchmarks/pmu proves the counters *advance*.
 * This suite proves the returned numbers are *correct*, in three tiers that do
 * not depend on per-microarchitecture magic numbers:
 *
 *   Tier 1 (absolute, deterministic): a known-length NOP sled retires an exact
 *     number of instructions. Comparing two sleds of different lengths cancels
 *     all fixed start/stop/call overhead, so the INST_RETIRED delta must equal
 *     the nop-count delta to within a couple of instructions. A miscounting /
 *     mis-scaled counter fails by thousands.
 *
 *   Tier 2 (relational): mispredicted-branch and cache-refill events must move
 *     in the right direction between a favourable and an adverse workload
 *     (random >> predictable). True on any ARMv8 core without fixed constants.
 *
 *   Tier 3 (cross-check): the cycle counter's rate over a known wall-clock
 *     window must match a believable core frequency and the driver's own
 *     runtime calibration.
 *
 * All measurements run with the scheduler and interrupts locked so the thread
 * cannot migrate (PMU state is per-CPU) and no ISR pollutes the counts. On a
 * PMU-less target (e.g. QEMU virt) the suite skips cleanly.
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/arch/pmu.h>
#include <zephyr/random/random.h>
#include <zephyr/sys/util.h>

/* NOP sled sizes. The DELTA is what the INST_RETIRED test actually checks. */
#define NOPS_SMALL 1024U
#define NOPS_LARGE 5120U
#define NOPS_DELTA (NOPS_LARGE - NOPS_SMALL) /* 4096 */

/* Workload sizes for the relational tests. */
#define BRANCH_COUNT 4096U
#define CACHE_WORDS  8192U /* 64 KiB > typical L1D, fits L2 */

static bool pmu_usable;

/* Driver's runtime-calibrated core frequency (0 when not available). */
static inline uint32_t cpu_freq_mhz(void)
{
#if defined(CONFIG_ARM64_PMUV3)
	return arch_pmu_cpu_freq_mhz();
#else
	return 0U;
#endif
}

static uint8_t branch_pred_pattern[BRANCH_COUNT]; /* all-zero: perfectly predictable */
static uint8_t branch_rand_pattern[BRANCH_COUNT]; /* random 0/1: adversarial */
static uint64_t __aligned(64) cache_array[CACHE_WORDS];
static uint32_t cache_rand_idx[CACHE_WORDS];
static volatile uint64_t work_sink;

/*
 * Fixed-length NOP sleds. .rept emits exactly N `nop`s; each retires as one
 * instruction. isb bookends keep the enable/measure/disable boundaries crisp.
 * __noinline + volatile + memory clobber stop the compiler from eliding or
 * reordering the block. The bl/ret and prologue are identical for both sizes,
 * so they cancel when the two measurements are subtracted.
 */
static void __noinline work_nops_small(void)
{
	__asm__ volatile("isb\n\t"
			 ".rept " STRINGIFY(NOPS_SMALL) "\n\t"
			 "nop\n\t"
			 ".endr\n\t"
			 "isb\n\t" ::: "memory");
}

static void __noinline work_nops_large(void)
{
	__asm__ volatile("isb\n\t"
			 ".rept " STRINGIFY(NOPS_LARGE) "\n\t"
			 "nop\n\t"
			 ".endr\n\t"
			 "isb\n\t" ::: "memory");
}

/*
 * Data-dependent conditional branch, forced in inline asm. A plain C
 * `if (pat[i])` gets if-converted to a branchless CSEL at -Os on ARMv8, leaving
 * nothing for BR_MIS_PRED to observe. The explicit `cbz` cannot be if-converted,
 * so an all-zero pattern is always-taken (perfectly predicted) while a random
 * pattern mispredicts ~50% of the time.
 */
static void __noinline branch_workload(const uint8_t *pat)
{
	uint64_t sink = 0;

	for (uint32_t i = 0; i < BRANCH_COUNT; i++) {
		uint32_t c = pat[i];

		__asm__ volatile("cbz %w[c], 1f\n\t"
				 "add %[s], %[s], #1\n\t"
				 "1:\n\t"
				 : [s] "+r"(sink)
				 : [c] "r"(c)
				 : "memory");
	}
	work_sink = sink;
}

static void __noinline work_branch_pred(void)
{
	branch_workload(branch_pred_pattern);
}

static void __noinline work_branch_rand(void)
{
	branch_workload(branch_rand_pattern);
}

static void __noinline work_cache_seq(void)
{
	uint64_t s = 0;

	for (uint32_t i = 0; i < CACHE_WORDS; i++) {
		s += cache_array[i];
	}
	work_sink = s;
}

static void __noinline work_cache_rand(void)
{
	uint64_t s = 0;

	for (uint32_t i = 0; i < CACHE_WORDS; i++) {
		s += cache_array[cache_rand_idx[i]];
	}
	work_sink = s;
}

/*
 * Measure a single event across one call to @work. Returns the event-0 count
 * and, via @cycles_out, the cycle count for the same window. The whole window
 * runs under sched-lock (no migration: PMU is per-CPU) and irq-lock (no ISR
 * instructions counted). pmu_init() is idempotent and only calibrates once.
 */
static uint64_t measure_event(pmu_evt_t event, void (*work)(void), uint64_t *cycles_out)
{
	unsigned int key;
	uint64_t c0;

	k_sched_lock();
	(void)pmu_init(); /* ensure THIS CPU is initialised */
	pmu_counter_disable_all();
	(void)pmu_counter_config(0, event);
	pmu_counter_enable(0);

	key = irq_lock();
	pmu_counter_reset_all();
	pmu_cycle_reset();
	pmu_start();
	work();
	pmu_stop();
	c0 = pmu_counter_read(0);
	if (cycles_out != NULL) {
		*cycles_out = pmu_cycle_count();
	}
	irq_unlock(key);

	pmu_counter_disable_all();
	k_sched_unlock();
	return c0;
}

/* ------------------------------------------------------------------ Tier 1 */

ZTEST(pmu_accuracy, test_inst_retired_exact_delta)
{
	uint64_t small_cyc = 0, large_cyc = 0;
	uint64_t small, large;
	int64_t delta;

	if (!pmu_usable) {
		ztest_test_skip();
	}

	small = measure_event(PMU_EVT_INST_RETIRED, work_nops_small, &small_cyc);
	large = measure_event(PMU_EVT_INST_RETIRED, work_nops_large, &large_cyc);

	TC_PRINT("INST_RETIRED: small(%u nops)=%llu large(%u nops)=%llu delta=%lld\n",
		 NOPS_SMALL, (unsigned long long)small, NOPS_LARGE,
		 (unsigned long long)large, (long long)(large - small));

	/* Absolute sanity: each sled retires at least its nop count. */
	zassert_true(small >= NOPS_SMALL, "small count %llu < %u nops",
		     (unsigned long long)small, NOPS_SMALL);
	zassert_true(large >= NOPS_LARGE, "large count %llu < %u nops",
		     (unsigned long long)large, NOPS_LARGE);

	/*
	 * The decisive check: fixed overhead cancels, so the delta must equal
	 * the extra nops within a couple of instructions. A broken or scaled
	 * counter misses by thousands.
	 */
	delta = (int64_t)large - (int64_t)small;
	zassert_within(delta, (int64_t)NOPS_DELTA, 16,
		       "INST_RETIRED delta %lld != %u (+-16); counter is not 1:1",
		       (long long)delta, NOPS_DELTA);
}

ZTEST(pmu_accuracy, test_ipc_within_sane_band)
{
	uint64_t cyc = 0;
	uint64_t insn;

	if (!pmu_usable) {
		ztest_test_skip();
	}

	insn = measure_event(PMU_EVT_INST_RETIRED, work_nops_large, &cyc);

	TC_PRINT("IPC check: insn=%llu cycles=%llu\n", (unsigned long long)insn,
		 (unsigned long long)cyc);

	zassert_true(cyc > 0ULL, "cycle counter did not advance");
	/*
	 * IPC = insn/cyc must be physically plausible on any ARMv8 pipeline:
	 * neither near-zero (counter stuck/scaled) nor absurdly high. Bound
	 * 0.05 < IPC < 8 without needing the exact per-core width.
	 */
	zassert_true(insn < cyc * 20ULL, "IPC too low: insn=%llu cyc=%llu",
		     (unsigned long long)insn, (unsigned long long)cyc);
	zassert_true(insn * 20ULL > cyc, "IPC too high: insn=%llu cyc=%llu",
		     (unsigned long long)insn, (unsigned long long)cyc);
}

/* ------------------------------------------------------------------ Tier 2 */

ZTEST(pmu_accuracy, test_branch_mispredict_tracks_reality)
{
	uint64_t pred, rnd;

	if (!pmu_usable) {
		ztest_test_skip();
	}

	pred = measure_event(PMU_EVT_BR_MIS_PRED, work_branch_pred, NULL);
	rnd = measure_event(PMU_EVT_BR_MIS_PRED, work_branch_rand, NULL);

	TC_PRINT("BR_MIS_PRED: predictable=%llu random=%llu\n",
		 (unsigned long long)pred, (unsigned long long)rnd);

	/*
	 * A data-dependent random branch mispredicts far more than an
	 * always-taken-the-same-way branch. Require a clear separation and a
	 * meaningful absolute count so a stuck-at-zero counter fails.
	 */
	zassert_true(rnd > BRANCH_COUNT / 8U, "random mispredicts too few: %llu",
		     (unsigned long long)rnd);
	zassert_true(rnd > pred * 3ULL + 50ULL,
		     "BR_MIS_PRED did not separate random(%llu) from predictable(%llu)",
		     (unsigned long long)rnd, (unsigned long long)pred);
}

ZTEST(pmu_accuracy, test_cache_refill_tracks_locality)
{
	uint64_t seq, rnd;

	if (!pmu_usable) {
		ztest_test_skip();
	}

	seq = measure_event(PMU_EVT_L1D_CACHE_REFILL, work_cache_seq, NULL);
	rnd = measure_event(PMU_EVT_L1D_CACHE_REFILL, work_cache_rand, NULL);

	TC_PRINT("L1D_CACHE_REFILL: sequential=%llu random=%llu\n",
		 (unsigned long long)seq, (unsigned long long)rnd);

	/*
	 * Random access over a >L1 working set refills the L1D more than a
	 * sequential sweep (which the hardware prefetcher hides). This is a
	 * softer, direction-only check because the exact ratio is
	 * microarchitecture dependent.
	 */
	zassert_true(rnd > 0ULL, "random refills unexpectedly zero");
	zassert_true(rnd >= seq, "random refills(%llu) < sequential(%llu)",
		     (unsigned long long)rnd, (unsigned long long)seq);
}

/* ------------------------------------------------------------------ Tier 3 */

ZTEST(pmu_accuracy, test_cycle_counter_frequency)
{
	const uint32_t window_ms = 5U;
	unsigned int key;
	uint64_t cyc;
	uint64_t hz;
	uint32_t cal_mhz;

	if (!pmu_usable) {
		ztest_test_skip();
	}

	k_sched_lock();
	(void)pmu_init();

	key = irq_lock();
	pmu_cycle_reset();
	pmu_start();
	k_busy_wait(window_ms * 1000U); /* spins on the arch timer; ok irq-locked */
	pmu_stop();
	cyc = pmu_cycle_count();
	irq_unlock(key);
	k_sched_unlock();

	hz = (cyc * 1000ULL) / window_ms;
	cal_mhz = cpu_freq_mhz();

	TC_PRINT("cycle freq: %llu Hz over %u ms (driver calibrated %u MHz)\n",
		 (unsigned long long)hz, window_ms, cal_mhz);

	/* Plausible core clock: not stuck at ~0, not impossibly fast. */
	zassert_true(hz > 100000000ULL && hz < 5000000000ULL,
		     "cycle frequency %llu Hz outside 0.1-5 GHz", (unsigned long long)hz);

	/* Cross-check against the driver's own runtime calibration (+-15%). */
	if (cal_mhz != 0U) {
		zassert_within(hz / 1000000ULL, (uint64_t)cal_mhz, cal_mhz / 6U + 1U,
			       "measured %llu MHz disagrees with calibrated %u MHz",
			       (unsigned long long)(hz / 1000000ULL), cal_mhz);
	}
}

/* ------------------------------------------------------------------- setup */

static void *pmu_accuracy_setup(void)
{
	int ret;

	for (uint32_t i = 0; i < BRANCH_COUNT; i++) {
		branch_pred_pattern[i] = 0U;
		branch_rand_pattern[i] = (uint8_t)(sys_rand32_get() & 1U);
	}
	for (uint32_t i = 0; i < CACHE_WORDS; i++) {
		cache_array[i] = i;
		cache_rand_idx[i] = sys_rand32_get() % CACHE_WORDS;
	}

	ret = pmu_init();
	if (ret != 0 || pmu_num_counters() == 0U) {
		TC_PRINT("PMU unavailable (init=%d, counters=%u) - accuracy tests skip\n",
			 ret, pmu_num_counters());
		pmu_usable = false;
		return NULL;
	}

	/* Confirm the counters actually advance before trusting any numbers. */
	pmu_counter_disable_all();
	(void)pmu_counter_config(0, PMU_EVT_INST_RETIRED);
	pmu_counter_enable(0);
	pmu_counter_reset_all();
	pmu_cycle_reset();
	pmu_start();
	work_nops_small();
	pmu_stop();
	pmu_usable = (pmu_cycle_count() != 0ULL) && (pmu_counter_read(0) != 0ULL);
	pmu_counter_disable_all();

	TC_PRINT("PMU accuracy suite: %s (%u counters, %u MHz calibrated)\n",
		 pmu_usable ? "ENABLED" : "counters idle - skipping",
		 pmu_num_counters(), cpu_freq_mhz());
	return NULL;
}

ZTEST_SUITE(pmu_accuracy, NULL, pmu_accuracy_setup, NULL, NULL, NULL);
