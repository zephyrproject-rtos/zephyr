/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief CPU performance benchmark using the Zephyr PMU API
 *
 * This test suite validates the architecture PMU backend (ARMv8-A PMUv3 when
 * @c CONFIG_ARM64_PMUV3 is enabled) and measures CPU behavior on Cortex-A
 * class processors.
 *
 * Test categories:
 * 1. Cache performance (L1 D-cache, L2 cache)
 * 2. Branch prediction
 * 3. Memory access patterns
 * 4. Compute performance
 * 5. TLB performance
 *
 * All tests use only architectural PMU events (0x00-0x1F) for portability
 * across backends that implement the Arm architectural PMU event encoding.
 *
 * AArch64 counter groups are defined in @c pmu_benchmark_arm64_configs.h.
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/pmu.h>
#include <pmu_benchmark_helpers.h>
#include <pmu_benchmark_arm64_configs.h>
#include <zephyr/random/random.h>
#include <zephyr/sys/util.h>

#ifdef CONFIG_SMP
BUILD_ASSERT(IS_ENABLED(CONFIG_SCHED_CPU_MASK),
	     "SMP build requires CONFIG_SCHED_CPU_MASK for CPU pinning; "
	     "PMU registers are per-CPU and measurements are invalid if the "
	     "test thread migrates between CPUs during a benchmark run.");
#endif

#ifdef CONFIG_SMP
/** CPU that owns PMU state for this suite (see suite setup). */
static int bench_cpu;
#endif

/* Test buffer sizes */
#define L1_CACHE_WORK_SIZE (16 * 1024)       /* 16KB working set — fits in L1 D-cache */
#define L2_CACHE_WORK_SIZE (128 * 1024)      /* 128KB working set — fits in L2 cache */
#define MAIN_MEM_TEST_SIZE (2 * 1024 * 1024) /* 2MB - exceeds L2, tests main memory */
#define CACHE_LINE_SIZE    64

#define ARRAY_ELEMENTS       (L2_CACHE_WORK_SIZE / sizeof(uint64_t))
#define LARGE_ARRAY_ELEMENTS (MAIN_MEM_TEST_SIZE / sizeof(uint64_t))

BUILD_ASSERT(MAIN_MEM_TEST_SIZE <= (CONFIG_SRAM_SIZE * 1024UL),
	     "large_array exceeds SRAM size; reduce MAIN_MEM_TEST_SIZE or use a larger platform");

/* Test iterations */
#define COMPUTE_ITERATIONS 10000
#define MEMORY_ITERATIONS  1000
#define BRANCH_ITERATIONS  100000

/* Test buffers */
static uint64_t __aligned(64) test_array[ARRAY_ELEMENTS];
static uint64_t __aligned(64) large_array[LARGE_ARRAY_ELEMENTS];
static uint32_t __aligned(64) random_indices[MEMORY_ITERATIONS];

/* Helper macros */
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

static struct pmu_counter_config counter_configs[PMU_MAX_COUNTERS];

/* Test setup */
static void test_setup(void)
{
	pmu_info_t pmu_id;
	int ret;
	size_t i;

	/* Initialize PMU */
	ret = pmu_init();

	TC_PRINT("\n");
	TC_PRINT("=================================================================\n");
	TC_PRINT(" CPU Performance Benchmark Suite\n");
	TC_PRINT("=================================================================\n");

	if (!pmu_benchmark_pmuv3_bootstrap(ret)) {
		TC_PRINT("PMU may not be available on this platform.\n");
		TC_PRINT("This is common on QEMU - tests will be skipped.\n");
		TC_PRINT("On hardware: ensure TF-A/bootloader enables PMU access.\n");
		TC_PRINT("=================================================================\n\n");
		return;
	}

	TC_PRINT("PMU initialized: %u counters available\n", pmu_num_counters());

	pmu_get_info(&pmu_id);
	TC_PRINT("CPU Implementer: 0x%02x, ID Code: 0x%02x\n", pmu_id.implementer, pmu_id.idcode);
	TC_PRINT("CPU Frequency: %u MHz (runtime calibrated)\n", pmu_benchmark_cpu_freq_mhz());

	/* Initialize test buffers */
	for (i = 0; i < ARRAY_ELEMENTS; i++) {
		test_array[i] = i;
	}
	for (i = 0; i < LARGE_ARRAY_ELEMENTS; i++) {
		large_array[i] = i;
	}

	/* Generate random indices for random access tests */
	for (i = 0; i < MEMORY_ITERATIONS; i++) {
		random_indices[i] = sys_rand32_get() % ARRAY_ELEMENTS;
	}

	TC_PRINT("Test buffers initialized\n");
	TC_PRINT("=================================================================\n\n");
}

/*
 * Cache Performance Tests
 */

ZTEST(cpu_performance, test_01_l1_dcache_sequential)
{
	uint64_t sum = 0;
	uint64_t cycles, c0, c1, c2, c3;

	if (!pmu_benchmark_pmu_ready()) {
		ztest_test_skip();
		return;
	}

	TC_PRINT("\n--- Test 1: L1 D-Cache Sequential Access ---\n");

	if (!pmu_benchmark_apply_counter_group(counter_configs, pmu_perf_cache_counters,
					       ARRAY_SIZE(pmu_perf_cache_counters))) {
		return;
	}

	MEASURE_START();

	/* Sequential read - should hit L1 cache */
	for (uint32_t iter = 0; iter < 100; iter++) {
		for (size_t i = 0; i < L1_CACHE_WORK_SIZE / sizeof(uint64_t); i++) {
			sum += test_array[i];
		}
	}

	MEASURE_STOP();

	cycles = pmu_cycle_count();
	c0 = pmu_counter_read(0); /* L1D_CACHE */
	c1 = pmu_counter_read(1); /* L1D_CACHE_REFILL */
	c2 = pmu_counter_read(2); /* L2D_CACHE */
	c3 = pmu_counter_read(3); /* L2D_CACHE_REFILL */

	print_pm_result("L1 D-Cache Sequential", counter_configs, cycles, c0, c1, c2, c3);

	/*
	 * Print L1 hit rate for informational purposes only.
	 * Microarchitectural thresholds vary across implementations and
	 * virtualization layers, so a hard assertion would be flaky.
	 */
	if (c0 > 0 && c0 >= c1) {
		double l1_hit_rate = (double)(c0 - c1) / c0 * 100.0;

		TC_PRINT("  L1 D-Cache Hit Rate: %.2f%%\n", l1_hit_rate);
		if (l1_hit_rate < 90.0) {
			TC_PRINT("  WARNING: L1 hit rate lower than expected\n");
		}
	}

	/* Prevent optimization */
	zassert_not_equal(sum, 0, "Sum should not be zero");
}

ZTEST(cpu_performance, test_02_l1_dcache_random)
{
	uint64_t sum = 0;
	uint64_t cycles, c0, c1, c2, c3;

	if (!pmu_benchmark_pmu_ready()) {
		ztest_test_skip();
		return;
	}

	TC_PRINT("\n--- Test 2: L1 D-Cache Random Access ---\n");

	if (!pmu_benchmark_apply_counter_group(counter_configs, pmu_perf_cache_counters,
					       ARRAY_SIZE(pmu_perf_cache_counters))) {
		return;
	}

	MEASURE_START();

	/* Random access within L1-sized buffer */
	for (uint32_t i = 0; i < MEMORY_ITERATIONS * 10; i++) {
		uint32_t idx = random_indices[i % MEMORY_ITERATIONS] %
			       (L1_CACHE_WORK_SIZE / sizeof(uint64_t));
		sum += test_array[idx];
	}

	MEASURE_STOP();

	cycles = pmu_cycle_count();
	c0 = pmu_counter_read(0);
	c1 = pmu_counter_read(1);
	c2 = pmu_counter_read(2);
	c3 = pmu_counter_read(3);

	print_pm_result("L1 D-Cache Random", counter_configs, cycles, c0, c1, c2, c3);

	zassert_not_equal(sum, 0, "Sum should not be zero");
}

ZTEST(cpu_performance, test_03_l2_cache_sequential)
{
	uint64_t sum = 0;
	uint64_t cycles, c0, c1, c2, c3;

	if (!pmu_benchmark_pmu_ready()) {
		ztest_test_skip();
		return;
	}

	TC_PRINT("\n--- Test 3: L2 Cache Sequential Access ---\n");

	if (!pmu_benchmark_apply_counter_group(counter_configs, pmu_perf_cache_counters,
					       ARRAY_SIZE(pmu_perf_cache_counters))) {
		return;
	}

	MEASURE_START();

	/* Sequential read across L2-sized buffer */
	for (uint32_t iter = 0; iter < 10; iter++) {
		for (size_t i = 0; i < ARRAY_ELEMENTS; i++) {
			sum += test_array[i];
		}
	}

	MEASURE_STOP();

	cycles = pmu_cycle_count();
	c0 = pmu_counter_read(0);
	c1 = pmu_counter_read(1);
	c2 = pmu_counter_read(2);
	c3 = pmu_counter_read(3);

	print_pm_result("L2 Cache Sequential", counter_configs, cycles, c0, c1, c2, c3);

	/* Calculate and print L2 hit rate; guard against counter noise (c3 > c2) */
	if (c2 > 0 && c2 >= c3) {
		double l2_hit_rate = (double)(c2 - c3) / c2 * 100.0;

		TC_PRINT("  L2 Cache Hit Rate: %.2f%%\n", l2_hit_rate);
	}

	zassert_not_equal(sum, 0, "Sum should not be zero");
}

ZTEST(cpu_performance, test_04_l2_cache_random)
{
	uint64_t sum = 0;
	uint64_t cycles, c0, c1, c2, c3;

	if (!pmu_benchmark_pmu_ready()) {
		ztest_test_skip();
		return;
	}

	TC_PRINT("\n--- Test 4: L2 Cache Random Access ---\n");

	if (!pmu_benchmark_apply_counter_group(counter_configs, pmu_perf_cache_counters,
					       ARRAY_SIZE(pmu_perf_cache_counters))) {
		return;
	}

	MEASURE_START();

	/* Random access across L2-sized buffer */
	for (uint32_t i = 0; i < MEMORY_ITERATIONS * 10; i++) {
		sum += test_array[random_indices[i % MEMORY_ITERATIONS]];
	}

	MEASURE_STOP();

	cycles = pmu_cycle_count();
	c0 = pmu_counter_read(0);
	c1 = pmu_counter_read(1);
	c2 = pmu_counter_read(2);
	c3 = pmu_counter_read(3);

	print_pm_result("L2 Cache Random", counter_configs, cycles, c0, c1, c2, c3);

	zassert_not_equal(sum, 0, "Sum should not be zero");
}

ZTEST(cpu_performance, test_05_memory_bandwidth)
{
	uint64_t sum = 0;
	uint64_t cycles, c0, c1, c2, c3;
	uint32_t cpu_mhz;

	if (!pmu_benchmark_pmu_ready()) {
		ztest_test_skip();
		return;
	}

	TC_PRINT("\n--- Test 5: Memory Bandwidth (Main Memory, 2MB) ---\n");

	if (!pmu_benchmark_apply_counter_group(counter_configs, pmu_perf_cache_counters,
					       ARRAY_SIZE(pmu_perf_cache_counters))) {
		return;
	}

	MEASURE_START();

	/* Access large buffer that exceeds L2 cache */
	for (size_t i = 0; i < LARGE_ARRAY_ELEMENTS; i++) {
		sum += large_array[i];
	}

	MEASURE_STOP();

	cycles = pmu_cycle_count();
	c0 = pmu_counter_read(0);
	c1 = pmu_counter_read(1);
	c2 = pmu_counter_read(2);
	c3 = pmu_counter_read(3);

	print_pm_result("Memory Bandwidth", counter_configs, cycles, c0, c1, c2, c3);

	/* Calculate bandwidth using runtime-calibrated CPU frequency */
	cpu_mhz = pmu_benchmark_cpu_freq_mhz();
	if (cpu_mhz > 0 && cycles > 0) {
		double bytes_accessed = LARGE_ARRAY_ELEMENTS * sizeof(uint64_t);
		double bandwidth_mbps = (bytes_accessed / (double)cycles) * cpu_mhz;

		TC_PRINT("  Estimated Bandwidth: %.2f MB/s (at %u MHz)\n", bandwidth_mbps, cpu_mhz);
	} else if (cpu_mhz == 0) {
		TC_PRINT("  Bandwidth estimation skipped: "
			 "CPU frequency calibration unavailable\n");
	} else {
		TC_PRINT("  Bandwidth estimation skipped: "
			 "no cycle count from PMU\n");
	}

	zassert_not_equal(sum, 0, "Sum should not be zero");
}

/*
 * Branch Prediction Tests
 */

ZTEST(cpu_performance, test_06_branch_predictable)
{
	uint64_t sum = 0;
	uint64_t cycles, c0, c1, c2, c3;

	if (!pmu_benchmark_pmu_ready()) {
		ztest_test_skip();
		return;
	}

	TC_PRINT("\n--- Test 6: Predictable Branches ---\n");

	if (!pmu_benchmark_apply_counter_group(counter_configs, pmu_perf_branch_counters,
					       ARRAY_SIZE(pmu_perf_branch_counters))) {
		return;
	}

	MEASURE_START();

	/* Highly predictable branch pattern */
	for (uint32_t i = 0; i < BRANCH_ITERATIONS; i++) {
		if (i % 2 == 0) {
			sum += i;
		} else {
			sum += i * 2;
		}
	}

	MEASURE_STOP();

	cycles = pmu_cycle_count();
	c0 = pmu_counter_read(0); /* BR_PRED */
	c1 = pmu_counter_read(1); /* BR_MIS_PRED */
	c2 = pmu_counter_read(2); /* INST_RETIRED */
	c3 = pmu_counter_read(3); /* INST_SPEC */

	print_pm_result("Predictable Branches", counter_configs, cycles, c0, c1, c2, c3);

	if (c0 > 0 && c0 >= c1) {
		double accuracy = (double)(c0 - c1) / c0 * 100.0;

		TC_PRINT("  Branch Prediction Accuracy: %.2f%%\n", accuracy);
		if (accuracy < 95.0) {
			TC_PRINT("  WARNING: Branch prediction accuracy "
				 "lower than expected\n");
		}
	}

	if (c2 > 0) {
		double ipc = (double)c2 / cycles;

		TC_PRINT("  Instructions Per Cycle (IPC): %.2f\n", ipc);
	}

	zassert_not_equal(sum, 0, "Sum should not be zero");
}

ZTEST(cpu_performance, test_07_branch_random)
{
	uint64_t sum = 0;
	uint64_t cycles, c0, c1, c2, c3;

	if (!pmu_benchmark_pmu_ready()) {
		ztest_test_skip();
		return;
	}

	TC_PRINT("\n--- Test 7: Random Branches ---\n");

	if (!pmu_benchmark_apply_counter_group(counter_configs, pmu_perf_branch_counters,
					       ARRAY_SIZE(pmu_perf_branch_counters))) {
		return;
	}

	MEASURE_START();

	/* Unpredictable branch pattern */
	for (uint32_t i = 0; i < BRANCH_ITERATIONS; i++) {
		uint32_t rnd = random_indices[i % MEMORY_ITERATIONS];

		if (rnd & 1) {
			sum += rnd;
		} else {
			sum += rnd * 2;
		}
	}

	MEASURE_STOP();

	cycles = pmu_cycle_count();
	c0 = pmu_counter_read(0);
	c1 = pmu_counter_read(1);
	c2 = pmu_counter_read(2);
	c3 = pmu_counter_read(3);

	print_pm_result("Random Branches", counter_configs, cycles, c0, c1, c2, c3);

	if (c0 > 0 && c0 >= c1) {
		double accuracy = (double)(c0 - c1) / c0 * 100.0;

		TC_PRINT("  Branch Prediction Accuracy: %.2f%% (expected ~50%%)\n", accuracy);
	}

	if (c2 > 0) {
		double ipc = (double)c2 / cycles;

		TC_PRINT("  Instructions Per Cycle (IPC): %.2f\n", ipc);
	}

	zassert_not_equal(sum, 0, "Sum should not be zero");
}

/*
 * Memory Access Pattern Tests
 */

ZTEST(cpu_performance, test_08_memory_sequential_read)
{
	uint64_t sum = 0;
	uint64_t cycles, c0, c1, c2, c3;

	if (!pmu_benchmark_pmu_ready()) {
		ztest_test_skip();
		return;
	}

	TC_PRINT("\n--- Test 8: Sequential Memory Read ---\n");

	if (!pmu_benchmark_apply_counter_group(counter_configs, pmu_perf_memory_counters,
					       ARRAY_SIZE(pmu_perf_memory_counters))) {
		return;
	}

	MEASURE_START();

	for (size_t i = 0; i < ARRAY_ELEMENTS; i++) {
		sum += test_array[i];
	}

	MEASURE_STOP();

	cycles = pmu_cycle_count();
	c0 = pmu_counter_read(0); /* MEM_ACCESS */
	c1 = pmu_counter_read(1); /* BUS_ACCESS */
	c2 = pmu_counter_read(2); /* INST_RETIRED */
	c3 = pmu_counter_read(3); /* CPU_CYCLES (should match) */

	print_pm_result("Sequential Read", counter_configs, cycles, c0, c1, c2, c3);

	zassert_not_equal(sum, 0, "Sum should not be zero");
}

ZTEST(cpu_performance, test_09_memory_sequential_write)
{
	uint64_t cycles, c0, c1, c2, c3;

	if (!pmu_benchmark_pmu_ready()) {
		ztest_test_skip();
		return;
	}

	TC_PRINT("\n--- Test 9: Sequential Memory Write ---\n");

	if (!pmu_benchmark_apply_counter_group(counter_configs, pmu_perf_memory_counters,
					       ARRAY_SIZE(pmu_perf_memory_counters))) {
		return;
	}

	MEASURE_START();

	for (size_t i = 0; i < ARRAY_ELEMENTS; i++) {
		test_array[i] = i * 2;
	}

	MEASURE_STOP();

	cycles = pmu_cycle_count();
	c0 = pmu_counter_read(0);
	c1 = pmu_counter_read(1);
	c2 = pmu_counter_read(2);
	c3 = pmu_counter_read(3);

	print_pm_result("Sequential Write", counter_configs, cycles, c0, c1, c2, c3);

	zassert_true(cycles > 0, "Write loop should have consumed cycles");
}

ZTEST(cpu_performance, test_10_memory_stride_access)
{
	uint64_t sum = 0;
	uint64_t cycles, c0, c1, c2, c3;

	if (!pmu_benchmark_pmu_ready()) {
		ztest_test_skip();
		return;
	}

	TC_PRINT("\n--- Test 10: Strided Memory Access ---\n");

	if (!pmu_benchmark_apply_counter_group(counter_configs, pmu_perf_memory_counters,
					       ARRAY_SIZE(pmu_perf_memory_counters))) {
		return;
	}

	MEASURE_START();

	/* Access every 8th element (stride = 64 bytes = cache line size) */
	for (size_t i = 0; i < ARRAY_ELEMENTS; i += 8) {
		sum += test_array[i];
	}

	MEASURE_STOP();

	cycles = pmu_cycle_count();
	c0 = pmu_counter_read(0);
	c1 = pmu_counter_read(1);
	c2 = pmu_counter_read(2);
	c3 = pmu_counter_read(3);

	print_pm_result("Strided Access (64B)", counter_configs, cycles, c0, c1, c2, c3);

	zassert_not_equal(sum, 0, "Sum should not be zero");
}

/*
 * Compute Performance Tests
 */

ZTEST(cpu_performance, test_11_integer_alu)
{
	uint64_t result = 1;
	uint64_t cycles, c0, c1, c2, c3;

	if (!pmu_benchmark_pmu_ready()) {
		ztest_test_skip();
		return;
	}

	TC_PRINT("\n--- Test 11: Integer ALU Performance ---\n");

	if (!pmu_benchmark_apply_counter_group(counter_configs, pmu_perf_branch_counters,
					       ARRAY_SIZE(pmu_perf_branch_counters))) {
		return;
	}

	MEASURE_START();

	/* Integer arithmetic operations */
	for (uint32_t i = 0; i < COMPUTE_ITERATIONS; i++) {
		result = result + i;
		result = result * 3;
		result = result - i;
		result = result ^ i;
	}

	MEASURE_STOP();

	cycles = pmu_cycle_count();
	c0 = pmu_counter_read(0);
	c1 = pmu_counter_read(1);
	c2 = pmu_counter_read(2); /* INST_RETIRED */
	c3 = pmu_counter_read(3);

	print_pm_result("Integer ALU", counter_configs, cycles, c0, c1, c2, c3);

	if (c2 > 0) {
		double ipc = (double)c2 / cycles;

		TC_PRINT("  Instructions Per Cycle (IPC): %.2f\n", ipc);
	}

	zassert_not_equal(result, 0, "Result should not be zero");
}

ZTEST(cpu_performance, test_12_integer_multiply)
{
	uint64_t result = 1;
	uint64_t cycles, c0, c1, c2, c3;

	if (!pmu_benchmark_pmu_ready()) {
		ztest_test_skip();
		return;
	}

	TC_PRINT("\n--- Test 12: Integer Multiply Performance ---\n");

	if (!pmu_benchmark_apply_counter_group(counter_configs, pmu_perf_branch_counters,
					       ARRAY_SIZE(pmu_perf_branch_counters))) {
		return;
	}

	MEASURE_START();

	/* Integer multiply operations */
	for (uint32_t i = 1; i < COMPUTE_ITERATIONS; i++) {
		result = result * (i & 0xFF);
		result = (result >> 8) | 1; /* Prevent overflow */
	}

	MEASURE_STOP();

	cycles = pmu_cycle_count();
	c0 = pmu_counter_read(0);
	c1 = pmu_counter_read(1);
	c2 = pmu_counter_read(2);
	c3 = pmu_counter_read(3);

	print_pm_result("Integer Multiply", counter_configs, cycles, c0, c1, c2, c3);

	zassert_not_equal(result, 0, "Result should not be zero");
}

static void *cpu_perf_setup(void)
{
	test_setup();

#ifdef CONFIG_SMP
	/* Record the CPU we booted the suite on. Do not call k_thread_cpu_pin() on
	 * k_current_get() here: the mask APIs require the target thread to not be
	 * currently runnable (see tests/kernel/threads/thread_apis and kernel.h).
	 * ZTEST runs this thread at cooperative priority by default; pin helper
	 * threads with k_thread_cpu_pin() before k_thread_start() instead.
	 */
	bench_cpu = arch_curr_cpu()->id;
#endif

	return NULL;
}

ZTEST_SUITE(cpu_performance, NULL, cpu_perf_setup, NULL, NULL, NULL);
