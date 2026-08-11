/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Unified PMU benchmark suite (kernel + cache + branch)
 *
 * Uses the Zephyr ztest benchmark framework (@c ZTEST_BENCHMARK*) with hardware
 * PMU counters (@c pmu_* API) for measurement. When @ref pmu_pmuv3_bootstrap
 * fails (typical on QEMU), individual benchmarks become no-ops.
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/benchmark.h>
#include <zephyr/arch/pmu.h>
#include <pmu_helpers.h>
#include <pmu_arm64_configs.h>
#include <zephyr/irq_offload.h>
#include <zephyr/random/random.h>
#include <zephyr/sys/util.h>

#ifdef CONFIG_SMP
BUILD_ASSERT(IS_ENABLED(CONFIG_SCHED_CPU_MASK),
	     "SMP build requires CONFIG_SCHED_CPU_MASK for CPU pinning; "
	     "PMU registers are per-CPU and measurements are invalid if the "
	     "test thread migrates between CPUs during a benchmark run.");
#endif

#define BENCH_DURATION_MS 1000U
#define HELPER_STACK_SIZE 2048
#define HELPER_PRIORITY   K_PRIO_PREEMPT(10)

#ifdef CONFIG_SMP
static int bench_cpu;
#endif

#define L1_CACHE_WORK_SIZE (16 * 1024)
#define L2_CACHE_WORK_SIZE (128 * 1024)
#define CACHE_LINE_SIZE    64

#define ARRAY_ELEMENTS    (L2_CACHE_WORK_SIZE / sizeof(uint64_t))
#define MEMORY_ITERATIONS 1000

static uint64_t __aligned(CACHE_LINE_SIZE) test_array[ARRAY_ELEMENTS];
static uint32_t __aligned(CACHE_LINE_SIZE) random_indices[MEMORY_ITERATIONS];

static struct pmu_counter_config counter_configs[PMU_MAX_COUNTERS];

#define MEASURE_START()                                                                            \
	pmu_counter_reset_all();                                                                   \
	pmu_cycle_reset();                                                                         \
	pmu_start()

#define MEASURE_STOP() pmu_stop()

struct pmu_bench_ctx {
	bool active;
	const char *name;
};

static void print_pm_result(const char *test_name, const struct pmu_counter_config *cfgs,
			    uint64_t cycles, uint64_t e0, uint64_t e1, uint64_t e2, uint64_t e3)
{
	printk("%-30s: %10llu cycles", test_name, (unsigned long long)cycles);
	if (e0 > 0U) {
		printk(", %s=%llu", pmu_event_name(cfgs[0].event),
		       (unsigned long long)e0);
	}
	if (e1 > 0U) {
		printk(", %s=%llu", pmu_event_name(cfgs[1].event),
		       (unsigned long long)e1);
	}
	if (e2 > 0U) {
		printk(", %s=%llu", pmu_event_name(cfgs[2].event),
		       (unsigned long long)e2);
	}
	if (e3 > 0U) {
		printk(", %s=%llu", pmu_event_name(cfgs[3].event),
		       (unsigned long long)e3);
	}
	printk("\n");
}

static void pmu_require_measurement(const char *tag, uint64_t cycles, uint64_t c0,
					      uint64_t c1, uint64_t c2, uint64_t c3)
{
	zassert_true(cycles > 0ULL, "%s: PMU cycle counter did not advance", tag);
	zassert_true((c0 | c1 | c2 | c3) != 0ULL, "%s: all PMU event counters are zero", tag);
}

static bool pmu_bench_begin(struct pmu_bench_ctx *ctx, const char *name,
			    const struct pmu_counter_config *cfgs, size_t n)
{
	ctx->active = false;
	ctx->name = name;

	if (!pmu_ready()) {
		return false;
	}

#ifdef CONFIG_SMP
	zassert_equal(arch_curr_cpu()->id, bench_cpu,
		      "%s: benchmark thread migrated off CPU %d", name, bench_cpu);
#endif

	if (!pmu_apply_counter_group(counter_configs, cfgs, n)) {
		return false;
	}

	MEASURE_START();
	ctx->active = true;
	return true;
}

static void pmu_bench_end(struct pmu_bench_ctx *ctx)
{
	uint64_t cycles;
	uint64_t c0;
	uint64_t c1;
	uint64_t c2;
	uint64_t c3;

	if (!ctx->active) {
		return;
	}

	MEASURE_STOP();

	cycles = pmu_cycle_count();
	c0 = pmu_counter_read(0);
	c1 = pmu_counter_read(1);
	c2 = pmu_counter_read(2);
	c3 = pmu_counter_read(3);

	pmu_require_measurement(ctx->name, cycles, c0, c1, c2, c3);
	print_pm_result(ctx->name, counter_configs, cycles, c0, c1, c2, c3);

	ctx->active = false;
}

static void pmu_bench_suite_setup(void)
{
	pmu_info_t pmu_id;
	int ret;
	size_t i;

#ifdef CONFIG_SMP
	bench_cpu = arch_curr_cpu()->id;
	/*
	 * CPU mask APIs cannot pin a running thread (see test_threads_cpu_mask).
	 * Record the boot CPU and verify it in pmu_bench_begin(); pin helper
	 * threads to bench_cpu before k_thread_start().
	 */
#endif

	ret = pmu_init();

	printk("\n");
	printk("=================================================================\n");
	printk(" PMU benchmark suite (kernel + cache + branch)\n");
	printk("=================================================================\n");

	if (!pmu_pmuv3_bootstrap(ret)) {
		printk("This is common on QEMU - benchmarks will be skipped.\n");
		printk("=================================================================\n\n");
		return;
	}

	printk("PMU initialized: %u counters available\n", pmu_num_counters());

	pmu_get_info(&pmu_id);
	printk("CPU Implementer: 0x%02x, ID Code: 0x%02x\n", pmu_id.implementer, pmu_id.idcode);
	printk("CPU Frequency: %u MHz (runtime calibrated)\n", pmu_cpu_freq_mhz());

	for (i = 0; i < ARRAY_ELEMENTS; i++) {
		test_array[i] = i;
	}
	for (i = 0; i < MEMORY_ITERATIONS; i++) {
		random_indices[i] = sys_rand32_get() % ARRAY_ELEMENTS;
	}

	printk("Test buffers initialized\n");
	printk("=================================================================\n\n");
}

ZTEST_BENCHMARK_SUITE(pmu, pmu_bench_suite_setup, NULL);

/* ----- Context switch ----- */

static struct k_thread helper_thread;
static K_THREAD_STACK_DEFINE(helper_stack, HELPER_STACK_SIZE);
static struct k_sem sync_sem;
static struct pmu_bench_ctx ctx_switch_ctx;
static bool ctx_switch_helper_started;

static void yield_helper_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	k_sem_give(&sync_sem);

	while (true) {
		k_yield();
	}
}

static void ctx_switch_start_helper(void)
{
	k_tid_t tid;

	if (ctx_switch_helper_started) {
		return;
	}

	k_sem_init(&sync_sem, 0, 1);

	tid = k_thread_create(&helper_thread, helper_stack,
			      K_THREAD_STACK_SIZEOF(helper_stack), yield_helper_entry,
			      NULL, NULL, NULL, HELPER_PRIORITY, 0, K_FOREVER);
	zassert_not_null(tid, "failed to create context-switch helper thread");
#ifdef CONFIG_SMP
	zassert_ok(k_thread_cpu_pin(tid, bench_cpu), "helper must match bench CPU");
#endif
	k_thread_start(tid);

	zassert_ok(k_sem_take(&sync_sem, K_SECONDS(5)), "context-switch helper failed to start");

	ctx_switch_helper_started = true;
}

static void ctx_switch_setup(void)
{
	if (!pmu_bench_begin(&ctx_switch_ctx, "context_switch_yield",
			     pmu_bench_ctx_switch_counters,
			     ARRAY_SIZE(pmu_bench_ctx_switch_counters))) {
		return;
	}

	ctx_switch_start_helper();
}

static void ctx_switch_teardown(void)
{
	if (ctx_switch_ctx.active && ctx_switch_helper_started) {
		k_thread_abort(&helper_thread);
		ctx_switch_helper_started = false;
	}

	pmu_bench_end(&ctx_switch_ctx);
}

ZTEST_BENCHMARK_TIMED(pmu, context_switch_yield, BENCH_DURATION_MS,
		      ctx_switch_setup, ctx_switch_teardown)
{
	k_yield();
}

/* ----- irq offload ----- */

static volatile uint32_t isr_count;
static struct pmu_bench_ctx irq_offload_ctx;

static void minimal_isr(const void *arg)
{
	ARG_UNUSED(arg);
	isr_count++;
}

static void irq_offload_setup(void)
{
	if (!pmu_bench_begin(&irq_offload_ctx, "irq_offload_overhead",
			     pmu_bench_irq_offload_counters,
			     ARRAY_SIZE(pmu_bench_irq_offload_counters))) {
		return;
	}

	isr_count = 0;
}

static void irq_offload_teardown(void)
{
	if (irq_offload_ctx.active) {
		zassert_true(isr_count > 0U, "irq_offload ISR never ran");
	}

	pmu_bench_end(&irq_offload_ctx);
}

ZTEST_BENCHMARK_TIMED(pmu, irq_offload_overhead, BENCH_DURATION_MS,
		      irq_offload_setup, irq_offload_teardown)
{
	irq_offload(minimal_isr, NULL);
}

/* ----- k_uptime_get ----- */

static struct pmu_bench_ctx uptime_ctx;

static void uptime_setup(void)
{
	(void)pmu_bench_begin(&uptime_ctx, "k_uptime_get_kernel_mode",
			       pmu_bench_kernel_api_counters,
			       ARRAY_SIZE(pmu_bench_kernel_api_counters));
}

static void uptime_teardown(void)
{
	pmu_bench_end(&uptime_ctx);
}

ZTEST_BENCHMARK_TIMED(pmu, k_uptime_get_kernel_mode, BENCH_DURATION_MS,
		      uptime_setup, uptime_teardown)
{
	(void)k_uptime_get();
}

/* ----- L1 / L2 cache ----- */

static uint64_t cache_sum;
static struct pmu_bench_ctx l1_seq_ctx;

static void l1_seq_setup(void)
{
	cache_sum = 0;

	if (!pmu_bench_begin(&l1_seq_ctx, "l1_dcache_sequential", pmu_perf_cache_counters,
			     ARRAY_SIZE(pmu_perf_cache_counters))) {
		return;
	}
}

static void l1_seq_teardown(void)
{
	if (l1_seq_ctx.active) {
		zassert_not_equal(cache_sum, 0ULL, "L1 sequential sum should not be zero");
	}

	pmu_bench_end(&l1_seq_ctx);
}

ZTEST_BENCHMARK_TIMED(pmu, l1_dcache_sequential, BENCH_DURATION_MS,
		      l1_seq_setup, l1_seq_teardown)
{
	for (size_t i = 0; i < L1_CACHE_WORK_SIZE / sizeof(uint64_t); i++) {
		cache_sum += test_array[i];
	}
}

static struct pmu_bench_ctx l1_rand_ctx;
static uint32_t l1_rand_idx;

static void l1_rand_setup(void)
{
	cache_sum = 0;
	l1_rand_idx = 0;

	if (!pmu_bench_begin(&l1_rand_ctx, "l1_dcache_random", pmu_perf_cache_counters,
			     ARRAY_SIZE(pmu_perf_cache_counters))) {
		return;
	}
}

static void l1_rand_teardown(void)
{
	if (l1_rand_ctx.active) {
		zassert_not_equal(cache_sum, 0ULL, "L1 random sum should not be zero");
	}

	pmu_bench_end(&l1_rand_ctx);
}

ZTEST_BENCHMARK_TIMED(pmu, l1_dcache_random, BENCH_DURATION_MS,
		      l1_rand_setup, l1_rand_teardown)
{
	uint32_t idx = random_indices[l1_rand_idx % MEMORY_ITERATIONS] %
		       (L1_CACHE_WORK_SIZE / sizeof(uint64_t));

	l1_rand_idx++;
	cache_sum += test_array[idx];
}

static struct pmu_bench_ctx l2_seq_ctx;

static void l2_seq_setup(void)
{
	cache_sum = 0;

	if (!pmu_bench_begin(&l2_seq_ctx, "l2_cache_sequential", pmu_perf_cache_counters,
			     ARRAY_SIZE(pmu_perf_cache_counters))) {
		return;
	}
}

static void l2_seq_teardown(void)
{
	if (l2_seq_ctx.active) {
		zassert_not_equal(cache_sum, 0ULL, "L2 sequential sum should not be zero");
	}

	pmu_bench_end(&l2_seq_ctx);
}

ZTEST_BENCHMARK_TIMED(pmu, l2_cache_sequential, BENCH_DURATION_MS,
		      l2_seq_setup, l2_seq_teardown)
{
	for (size_t i = 0; i < ARRAY_ELEMENTS; i++) {
		cache_sum += test_array[i];
	}
}

/* ----- Branch prediction ----- */

static struct pmu_bench_ctx branch_pred_ctx;
static uint32_t branch_pred_i;

static void branch_pred_setup(void)
{
	cache_sum = 0;
	branch_pred_i = 0;

	if (!pmu_bench_begin(&branch_pred_ctx, "branch_predictable", pmu_perf_branch_counters,
			     ARRAY_SIZE(pmu_perf_branch_counters))) {
		return;
	}
}

static void branch_pred_teardown(void)
{
	if (branch_pred_ctx.active) {
		zassert_not_equal(cache_sum, 0ULL, "Branch predictable sum should not be zero");
	}

	pmu_bench_end(&branch_pred_ctx);
}

ZTEST_BENCHMARK_TIMED(pmu, branch_predictable, BENCH_DURATION_MS,
		      branch_pred_setup, branch_pred_teardown)
{
	if (branch_pred_i % 2U == 0U) {
		cache_sum += branch_pred_i;
	} else {
		cache_sum += branch_pred_i * 2U;
	}

	branch_pred_i++;
}

static struct pmu_bench_ctx branch_rand_ctx;
static uint32_t branch_rand_i;

static void branch_rand_setup(void)
{
	cache_sum = 0;
	branch_rand_i = 0;

	if (!pmu_bench_begin(&branch_rand_ctx, "branch_random", pmu_perf_branch_counters,
			     ARRAY_SIZE(pmu_perf_branch_counters))) {
		return;
	}
}

static void branch_rand_teardown(void)
{
	if (branch_rand_ctx.active) {
		zassert_not_equal(cache_sum, 0ULL, "Branch random sum should not be zero");
	}

	pmu_bench_end(&branch_rand_ctx);
}

ZTEST_BENCHMARK_TIMED(pmu, branch_random, BENCH_DURATION_MS,
		      branch_rand_setup, branch_rand_teardown)
{
	uint32_t rnd = random_indices[branch_rand_i % MEMORY_ITERATIONS];

	if (rnd & 1U) {
		cache_sum += rnd;
	} else {
		cache_sum += rnd * 2U;
	}

	branch_rand_i++;
}
