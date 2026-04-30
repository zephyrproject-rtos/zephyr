/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Kernel Operations PMU Benchmark Suite
 *
 * Uses the Zephyr @c pmu_* API (hardware event counters) to measure the cost of
 * fundamental kernel operations.  Unlike the latency_measure benchmark (which uses
 * timing_timestamp_get() for wall-clock cycles only), this suite instruments
 * operations with PMU events to provide richer architectural insight:
 * instructions retired, exceptions taken, cache refills, and branch
 * mispredictions alongside raw cycle counts.
 *
 * Test categories:
 * 1. Context switch (k_yield ping-pong)
 * 2. irq_offload overhead (synchronous ISR path)
 * 3. Thread suspend / resume
 * 4. Thread create / abort
 * 5. k_uptime_get() in kernel context (supervisor fast path, not user syscall)
 * 6. k_sem_give / k_sem_take in kernel context
 * Counter event tables for AArch64 are in @c pmu_benchmark_arm64_configs.h.
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/pmu.h>
#include <pmu_benchmark_helpers.h>
#include <pmu_benchmark_arm64_configs.h>
#include <zephyr/irq_offload.h>
#include <zephyr/sys/util.h>

#ifdef CONFIG_SMP
BUILD_ASSERT(IS_ENABLED(CONFIG_SCHED_CPU_MASK),
	     "SMP build requires CONFIG_SCHED_CPU_MASK for CPU pinning; "
	     "PMU registers are per-CPU and measurements are invalid if the "
	     "test thread migrates between CPUs during a benchmark run.");
#endif

/* Iteration counts */
#define NUM_ITERATIONS           1000
#define THREAD_CREATE_ITERATIONS 500 /* Thread create/abort is heavyweight */

/* Thread configuration */
#define HELPER_STACK_SIZE 1024
#define BENCH_PRIORITY    K_PRIO_COOP(7)

/** CPU used for all PMU work; helpers are pinned to match the benchmark thread. */
#ifdef CONFIG_SMP
static int bench_cpu;
#endif

/* Helper macros — same pattern as cpu_performance */
#define MEASURE_START()                                                                            \
	pmu_counter_reset_all();                                                                   \
	pmu_cycle_reset();                                                                         \
	pmu_start()

#define MEASURE_STOP() pmu_stop()

static void print_pm_result(const char *test_name, const struct pmu_counter_config *cfgs,
			    uint64_t cycles, uint64_t e0, uint64_t e1, uint64_t e2, uint64_t e3)
{
	TC_PRINT("%-30s: %10llu cycles", test_name, (unsigned long long)cycles);
	if (e0 > 0U) {
		TC_PRINT(", %s=%llu", pmu_benchmark_event_name(cfgs[0].event),
			 (unsigned long long)e0);
	}
	if (e1 > 0U) {
		TC_PRINT(", %s=%llu", pmu_benchmark_event_name(cfgs[1].event),
			 (unsigned long long)e1);
	}
	if (e2 > 0U) {
		TC_PRINT(", %s=%llu", pmu_benchmark_event_name(cfgs[2].event),
			 (unsigned long long)e2);
	}
	if (e3 > 0U) {
		TC_PRINT(", %s=%llu", pmu_benchmark_event_name(cfgs[3].event),
			 (unsigned long long)e3);
	}
	TC_PRINT("\n");
}

#define PRINT_AVG(label, total, n)                                                                 \
	TC_PRINT("  %-26s: %llu (avg per op)\n", label, (unsigned long long)((total) / (n)))

/* Mutable copy passed to pmu_configure_counters() */
static struct pmu_counter_config counter_configs[PMU_MAX_COUNTERS];

/* Shared thread resources */
static struct k_thread helper_thread;
static K_THREAD_STACK_DEFINE(helper_stack, HELPER_STACK_SIZE);
static struct k_sem sync_sem;

/* Volatile counter for ISR body */
static volatile uint32_t isr_count;

/* ------------------------------------------------------------------ */
/* Suite setup                                                         */
/* ------------------------------------------------------------------ */

static void test_setup(void)
{
	pmu_info_t pmu_id;
	int ret;

	ret = pmu_init();

	TC_PRINT("\n");
	TC_PRINT("=================================================================\n");
	TC_PRINT(" Kernel Operations PMU Benchmark Suite\n");
	TC_PRINT("=================================================================\n");

	if (!pmu_benchmark_pmuv3_bootstrap(ret)) {
		TC_PRINT("This is common on QEMU - tests will be skipped.\n");
		TC_PRINT("=================================================================\n\n");
		return;
	}

	TC_PRINT("PMU initialized: %u counters available\n", pmu_num_counters());

	pmu_get_info(&pmu_id);
	TC_PRINT("CPU Implementer: 0x%02x, ID Code: 0x%02x\n", pmu_id.implementer, pmu_id.idcode);
	TC_PRINT("CPU Frequency: %u MHz (runtime calibrated)\n", pmu_benchmark_cpu_freq_mhz());

	k_sem_init(&sync_sem, 0, 1);

	TC_PRINT("=================================================================\n\n");
}

static void *pmu_bench_setup(void)
{
	test_setup();

#ifdef CONFIG_SMP
	/* Same as cpu_performance: never k_thread_cpu_pin(k_current_get(), ...) while
	 * the ztest thread is runnable — kernel returns -EINVAL. Helpers are pinned
	 * before k_thread_start(); main stays coop-priority on this CPU.
	 */
	bench_cpu = arch_curr_cpu()->id;
#endif

	return NULL;
}

/* ------------------------------------------------------------------ */
/* Test 01: Context Switch via k_yield() ping-pong                     */
/* ------------------------------------------------------------------ */

static void yield_helper_entry(void *p1, void *p2, void *p3)
{
	uint32_t n = (uint32_t)(uintptr_t)p1;
	struct k_sem *ready = (struct k_sem *)p2;

	/* Signal that we are ready */
	k_sem_give(ready);

	for (uint32_t i = 0; i < n; i++) {
		k_yield();
	}
}

ZTEST(pmu_benchmarks, test_01_context_switch_yield)
{
	uint64_t cycles, c0, c1, c2, c3;
	uint32_t total_switches;

	if (!pmu_benchmark_pmu_ready()) {
		ztest_test_skip();
		return;
	}

	TC_PRINT("\n--- Test 1: Context Switch (k_yield ping-pong) ---\n");

	if (!pmu_benchmark_apply_counter_group(counter_configs, pmu_bench_ctx_switch_counters,
					       ARRAY_SIZE(pmu_bench_ctx_switch_counters))) {
		return;
	}

	/* Create helper at same cooperative priority.
	 * Use K_FOREVER + explicit start so we can pin the thread on SMP
	 * before it begins running.
	 */
	k_thread_create(&helper_thread, helper_stack, K_THREAD_STACK_SIZEOF(helper_stack),
			yield_helper_entry, (void *)(uintptr_t)NUM_ITERATIONS, &sync_sem, NULL,
			BENCH_PRIORITY, 0, K_FOREVER);
#ifdef CONFIG_SMP
	zassert_ok(k_thread_cpu_pin(&helper_thread, bench_cpu), "helper must match bench CPU");
#endif
	k_thread_start(&helper_thread);

	/* Wait for helper to be ready */
	k_sem_take(&sync_sem, K_FOREVER);

	MEASURE_START();

	for (uint32_t i = 0; i < NUM_ITERATIONS; i++) {
		k_yield();
	}

	MEASURE_STOP();

	cycles = pmu_cycle_count();
	c0 = pmu_counter_read(0); /* INST_RETIRED */
	c1 = pmu_counter_read(1); /* EXC_TAKEN */
	c2 = pmu_counter_read(2); /* L1D_CACHE_REFILL */
	c3 = pmu_counter_read(3); /* BR_MIS_PRED */

	/* Each loop iteration = 2 context switches (main->helper, helper->main) */
	total_switches = NUM_ITERATIONS * 2;

	print_pm_result("Context Switch (total)", counter_configs, cycles, c0, c1, c2, c3);

	PRINT_AVG("Cycles/switch", cycles, total_switches);
	PRINT_AVG("Instructions/switch", c0, total_switches);

	k_thread_abort(&helper_thread);
}

/* ------------------------------------------------------------------ */
/* irq_offload: synchronous ISR path (not interrupt-controller latency) */
/* ------------------------------------------------------------------ */

static void minimal_isr(const void *arg)
{
	ARG_UNUSED(arg);
	isr_count++;
}

ZTEST(pmu_benchmarks, test_irq_offload_overhead)
{
	uint64_t cycles, c0, c1, c2, c3;

	if (!pmu_benchmark_pmu_ready()) {
		ztest_test_skip();
		return;
	}

	TC_PRINT("\n--- Test 2: irq_offload overhead (ISR entry/return) ---\n");

	if (!pmu_benchmark_apply_counter_group(counter_configs, pmu_bench_irq_offload_counters,
					       ARRAY_SIZE(pmu_bench_irq_offload_counters))) {
		return;
	}

	isr_count = 0;

	MEASURE_START();

	for (uint32_t i = 0; i < NUM_ITERATIONS; i++) {
		irq_offload(minimal_isr, NULL);
	}

	MEASURE_STOP();

	cycles = pmu_cycle_count();
	c0 = pmu_counter_read(0); /* INST_RETIRED */
	c1 = pmu_counter_read(1); /* EXC_TAKEN */
	c2 = pmu_counter_read(2); /* EXC_RETURN */
	c3 = pmu_counter_read(3); /* BR_MIS_PRED */

	print_pm_result("irq_offload (total)", counter_configs, cycles, c0, c1, c2, c3);

	PRINT_AVG("Cycles/irq_offload", cycles, NUM_ITERATIONS);
	PRINT_AVG("Instructions/irq_offload", c0, NUM_ITERATIONS);

	TC_PRINT("  EXC_TAKEN count         : %llu (expected ~%u)\n", c1, NUM_ITERATIONS);
	TC_PRINT("  EXC_RETURN count        : %llu (expected ~%u)\n", c2, NUM_ITERATIONS);

	zassert_equal(isr_count, NUM_ITERATIONS, "ISR ran %u times, expected %u", isr_count,
		      NUM_ITERATIONS);
}

/* ------------------------------------------------------------------ */
/* Test 03: Thread Suspend / Resume                                    */
/* ------------------------------------------------------------------ */

static void suspend_resume_helper(void *p1, void *p2, void *p3)
{
	struct k_sem *ready = (struct k_sem *)p1;

	/* Signal that we are running */
	k_sem_give(ready);

	/* Repeatedly suspend self; bench thread resumes us each time */
	while (1) {
		k_thread_suspend(k_current_get());
	}
}

ZTEST(pmu_benchmarks, test_03_thread_suspend_resume)
{
	uint64_t cycles, c0, c1, c2, c3;

	if (!pmu_benchmark_pmu_ready()) {
		ztest_test_skip();
		return;
	}

	TC_PRINT("\n--- Test 3: Thread Suspend / Resume ---\n");

	if (!pmu_benchmark_apply_counter_group(counter_configs, pmu_bench_thread_ops_counters,
					       ARRAY_SIZE(pmu_bench_thread_ops_counters))) {
		return;
	}

	/*
	 * Create helper at higher priority (lower numeric value).
	 * Use K_FOREVER + explicit start to pin on SMP first.
	 * When resumed, it runs immediately, then suspends itself,
	 * returning control to the bench thread.
	 */
	k_thread_create(&helper_thread, helper_stack, K_THREAD_STACK_SIZEOF(helper_stack),
			suspend_resume_helper, &sync_sem, NULL, NULL, BENCH_PRIORITY - 1, 0,
			K_FOREVER);
#ifdef CONFIG_SMP
	zassert_ok(k_thread_cpu_pin(&helper_thread, bench_cpu), "helper must match bench CPU");
#endif
	k_thread_start(&helper_thread);

	/* Wait for helper to be running and then suspend itself */
	k_sem_take(&sync_sem, K_FOREVER);
	/*
	 * At this point helper has called k_sem_give and looped back
	 * to k_thread_suspend.  It is now suspended.
	 */

	MEASURE_START();

	for (uint32_t i = 0; i < NUM_ITERATIONS; i++) {
		/* Resume helper -> it runs -> suspends self -> we continue */
		k_thread_resume(&helper_thread);
	}

	MEASURE_STOP();

	cycles = pmu_cycle_count();
	c0 = pmu_counter_read(0); /* INST_RETIRED */
	c1 = pmu_counter_read(1); /* MEM_ACCESS */
	c2 = pmu_counter_read(2); /* L1D_CACHE_REFILL */
	c3 = pmu_counter_read(3); /* BR_MIS_PRED */

	print_pm_result("Suspend/Resume (total)", counter_configs, cycles, c0, c1, c2, c3);

	/* Each iteration = 1 resume + 1 suspend (2 context switches) */
	PRINT_AVG("Cycles/resume-suspend", cycles, NUM_ITERATIONS);
	PRINT_AVG("Instructions/resume-suspend", c0, NUM_ITERATIONS);

	k_thread_abort(&helper_thread);
}

/* ------------------------------------------------------------------ */
/* Test 04: Thread Create / Abort                                      */
/* ------------------------------------------------------------------ */

static void create_abort_helper(void *p1, void *p2, void *p3)
{
	struct k_sem *done = (struct k_sem *)p1;

	/* Signal completion, then exit naturally */
	k_sem_give(done);
}

ZTEST(pmu_benchmarks, test_04_thread_create_abort)
{
	uint64_t cycles, c0, c1, c2, c3;

	if (!pmu_benchmark_pmu_ready()) {
		ztest_test_skip();
		return;
	}

	TC_PRINT("\n--- Test 4: Thread Create / Abort ---\n");

	if (!pmu_benchmark_apply_counter_group(counter_configs, pmu_bench_thread_ops_counters,
					       ARRAY_SIZE(pmu_bench_thread_ops_counters))) {
		return;
	}

	MEASURE_START();

	for (uint32_t i = 0; i < THREAD_CREATE_ITERATIONS; i++) {
		k_sem_reset(&sync_sem);

		/*
		 * Create at higher priority so k_thread_start() immediately
		 * context-switches to the helper.  Helper signals sync_sem
		 * and exits.
		 */
		k_thread_create(&helper_thread, helper_stack, K_THREAD_STACK_SIZEOF(helper_stack),
				create_abort_helper, &sync_sem, NULL, NULL, BENCH_PRIORITY - 1, 0,
				K_FOREVER);
#ifdef CONFIG_SMP
		zassert_ok(k_thread_cpu_pin(&helper_thread, bench_cpu),
			   "helper must match bench CPU");
#endif
		k_thread_start(&helper_thread);

		/* Helper has run and exited; wait for the signal */
		k_sem_take(&sync_sem, K_FOREVER);

		/* Clean up thread resources */
		k_thread_abort(&helper_thread);
	}

	MEASURE_STOP();

	cycles = pmu_cycle_count();
	c0 = pmu_counter_read(0); /* INST_RETIRED */
	c1 = pmu_counter_read(1); /* MEM_ACCESS */
	c2 = pmu_counter_read(2); /* L1D_CACHE_REFILL */
	c3 = pmu_counter_read(3); /* BR_MIS_PRED */

	print_pm_result("Create/Abort (total)", counter_configs, cycles, c0, c1, c2, c3);

	PRINT_AVG("Cycles/create-abort", cycles, THREAD_CREATE_ITERATIONS);
	PRINT_AVG("Instructions/create-abort", c0, THREAD_CREATE_ITERATIONS);
}

/* ------------------------------------------------------------------ */
/* k_uptime_get() — kernel thread (not user-mode syscall trap)         */
/* ------------------------------------------------------------------ */

ZTEST(pmu_benchmarks, test_k_uptime_get_kernel_mode)
{
	volatile int64_t t;
	uint64_t cycles, c0, c1, c2, c3;

	if (!pmu_benchmark_pmu_ready()) {
		ztest_test_skip();
		return;
	}

	TC_PRINT("\n--- Test 5: k_uptime_get (kernel / supervisor context) ---\n");
	TC_PRINT("  Note: this suite runs in kernel mode; this measures the in-kernel API path,\n");
	TC_PRINT("  not user-mode syscall privilege elevation.\n");

	if (!pmu_benchmark_apply_counter_group(counter_configs, pmu_bench_kernel_api_counters,
					       ARRAY_SIZE(pmu_bench_kernel_api_counters))) {
		return;
	}

	MEASURE_START();

	for (uint32_t i = 0; i < NUM_ITERATIONS; i++) {
		t = k_uptime_get();
	}

	MEASURE_STOP();

	cycles = pmu_cycle_count();
	c0 = pmu_counter_read(0); /* INST_RETIRED */
	c1 = pmu_counter_read(1); /* EXC_TAKEN */
	c2 = pmu_counter_read(2); /* EXC_RETURN */
	c3 = pmu_counter_read(3); /* CPU_CYCLES */

	print_pm_result("k_uptime_get (total)", counter_configs, cycles, c0, c1, c2, c3);

	PRINT_AVG("Cycles/call", cycles, NUM_ITERATIONS);
	PRINT_AVG("Instructions/call", c0, NUM_ITERATIONS);

	TC_PRINT("  EXC_TAKEN               : %llu (expect ~0 in kernel mode)\n", c1);

	/* Prevent compiler from optimizing out the loop */
	zassert_true(t >= 0, "Uptime should be non-negative");
}

/* ------------------------------------------------------------------ */
/* k_sem_give / k_sem_take — kernel context */
/* ------------------------------------------------------------------ */

ZTEST(pmu_benchmarks, test_syscall_sem_give_take)
{
	struct k_sem bench_sem;
	uint64_t cycles, c0, c1, c2, c3;

	if (!pmu_benchmark_pmu_ready()) {
		ztest_test_skip();
		return;
	}

	TC_PRINT("\n--- Test 6: k_sem_give / k_sem_take (kernel context) ---\n");
	TC_PRINT("  Note: same as test 5 - supervisor-mode API path, not user syscall.\n");

	/* --- Measure k_sem_give (uncontested, no waiters) --- */

	k_sem_init(&bench_sem, 0, NUM_ITERATIONS);

	if (!pmu_benchmark_apply_counter_group(counter_configs, pmu_bench_kernel_api_counters,
					       ARRAY_SIZE(pmu_bench_kernel_api_counters))) {
		return;
	}

	MEASURE_START();

	for (uint32_t i = 0; i < NUM_ITERATIONS; i++) {
		k_sem_give(&bench_sem);
	}

	MEASURE_STOP();

	cycles = pmu_cycle_count();
	c0 = pmu_counter_read(0);
	c1 = pmu_counter_read(1);
	c2 = pmu_counter_read(2);
	c3 = pmu_counter_read(3);

	print_pm_result("k_sem_give (total)", counter_configs, cycles, c0, c1, c2, c3);

	PRINT_AVG("Cycles/give", cycles, NUM_ITERATIONS);
	PRINT_AVG("Instructions/give", c0, NUM_ITERATIONS);

	/* --- Measure k_sem_take (immediate, no blocking) --- */

	/* Semaphore count is now NUM_ITERATIONS from the gives above */
	if (!pmu_benchmark_apply_counter_group(counter_configs, pmu_bench_kernel_api_counters,
					       ARRAY_SIZE(pmu_bench_kernel_api_counters))) {
		return;
	}

	MEASURE_START();

	for (uint32_t i = 0; i < NUM_ITERATIONS; i++) {
		k_sem_take(&bench_sem, K_NO_WAIT);
	}

	MEASURE_STOP();

	cycles = pmu_cycle_count();
	c0 = pmu_counter_read(0);
	c1 = pmu_counter_read(1);
	c2 = pmu_counter_read(2);
	c3 = pmu_counter_read(3);

	print_pm_result("k_sem_take (total)", counter_configs, cycles, c0, c1, c2, c3);

	PRINT_AVG("Cycles/take", cycles, NUM_ITERATIONS);
	PRINT_AVG("Instructions/take", c0, NUM_ITERATIONS);
}

ZTEST_SUITE(pmu_benchmarks, NULL, pmu_bench_setup, NULL, NULL, NULL);
