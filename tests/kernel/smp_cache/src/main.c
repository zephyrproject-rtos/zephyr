/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SMP Cache Coherency Test Suite
 *
 * This test suite validates cache coherency and cache management operations in
 * SMP (Symmetric Multi-Processing) environments. It ensures cache operations work
 * correctly across multiple CPU cores and data remains consistent when shared.
 *
 * Tests included:
 * 1. Cache line size detection and validation
 * 2. Multi-writer cache coherency with flush operations
 * 3. Reader-writer cache coherency with flush and invalidate
 * 4. Stress testing of concurrent cache operations
 * 5. Instruction cache coherency across CPUs
 * 6. Cache operation performance measurement
 * 7. DMA-cache interaction scenarios
 *
 * Requirements:
 * - CONFIG_SMP=y
 * - CONFIG_MP_MAX_NUM_CPUS > 1
 * - CONFIG_CACHE_MANAGEMENT=y
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/cache.h>
#include <string.h>

#if CONFIG_MP_MAX_NUM_CPUS < 2
#error "SMP cache test requires at least two CPUs!"
#endif

#define STACK_SIZE (2048 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define TEST_BUFFER_SIZE 4096
#define CACHE_LINE_SIZE_MAX 256
#define NUM_TEST_THREADS CONFIG_MP_MAX_NUM_CPUS
#define TEST_ITERATIONS 100
#define SYNC_DELAY_US 10000

/* Test buffers - aligned to worst-case cache line size */
static uint8_t __aligned(CACHE_LINE_SIZE_MAX) test_data_buffer[TEST_BUFFER_SIZE];
static uint8_t __aligned(CACHE_LINE_SIZE_MAX) shared_data[NUM_TEST_THREADS][CACHE_LINE_SIZE_MAX];

/* Thread stacks and structures */
K_THREAD_STACK_ARRAY_DEFINE(test_stacks, NUM_TEST_THREADS, STACK_SIZE);
static struct k_thread test_threads[NUM_TEST_THREADS];

/* Synchronization primitives */
static struct k_sem sync_sem;
static struct k_sem completion_sem;
static atomic_t thread_counter;
static volatile bool cache_test_error;

/* Test results tracking */
struct cache_test_stats {
	uint32_t flush_count;
	uint32_t invalidate_count;
	uint32_t coherency_errors;
	uint32_t iterations_completed;
};

static struct cache_test_stats thread_stats[NUM_TEST_THREADS];

/**
 * @brief Get current CPU ID in a safe manner
 */
static int get_curr_cpu(void)
{
	unsigned int key = arch_irq_lock();
	int cpu_id = arch_curr_cpu()->id;

	arch_irq_unlock(key);
	return cpu_id;
}

/**
 * @brief Initialize test buffer with pattern
 */
static void init_test_buffer(uint8_t *buffer, size_t size, uint8_t pattern)
{
	for (size_t i = 0; i < size; i++) {
		buffer[i] = (uint8_t)(pattern + i);
	}
}

/**
 * @brief Verify test buffer against expected pattern
 */
static bool verify_test_buffer(uint8_t *buffer, size_t size, uint8_t pattern)
{
	for (size_t i = 0; i < size; i++) {
		if (buffer[i] != (uint8_t)(pattern + i)) {
			return false;
		}
	}
	return true;
}

/**
 * @brief Test: Cache line size detection
 *
 * Validates that cache line size can be detected or configured correctly
 * for both data and instruction caches.
 */
static void cache_coherency_writer_thread(void *arg1, void *arg2, void *arg3)
{
	int thread_id = POINTER_TO_INT(arg1);
	int max_iterations = POINTER_TO_INT(arg2);  /* Get iteration count */
	int cpu_id = get_curr_cpu();
	uint8_t *data_ptr = shared_data[thread_id];

	TC_PRINT("Writer thread %d running on CPU %d\n", thread_id, cpu_id);

	/* Use provided iteration count, or default to TEST_ITERATIONS */
	if (max_iterations == 0) {
		max_iterations = TEST_ITERATIONS;
	}

	for (int iter = 0; iter < max_iterations; iter++) {
		/* Wait for signal to start iteration */
		k_sem_take(&sync_sem, K_FOREVER);

		/* Write unique pattern for this iteration */
		uint8_t pattern = (uint8_t)(thread_id * 10 + iter);

		for (int i = 0; i < CACHE_LINE_SIZE_MAX; i++) {
			data_ptr[i] = (uint8_t)(pattern + i);
		}

		/* Flush to ensure other CPUs see the data */
		int ret = sys_cache_data_flush_range(data_ptr, CACHE_LINE_SIZE_MAX);

		if (ret == 0) {
			thread_stats[thread_id].flush_count++;
		}

		thread_stats[thread_id].iterations_completed++;

		/* Signal completion */
		k_sem_give(&completion_sem);
	}
}

/**
 * @brief Thread function for cache coherency test - reader
 *
 * This thread reads data written by another thread and validates cache coherency.
 */
static void cache_coherency_reader_thread(void *arg1, void *arg2, void *arg3)
{
	int thread_id = POINTER_TO_INT(arg1);
	int max_iterations = POINTER_TO_INT(arg2);  /* Get iteration count */
	int cpu_id = get_curr_cpu();
	/* Read from the previous thread's data (wrap around for thread 0) */
	int writer_id = (thread_id + NUM_TEST_THREADS - 1) % NUM_TEST_THREADS;
	uint8_t *data_ptr = shared_data[writer_id];

	TC_PRINT("Reader thread %d running on CPU %d (reading from thread %d)\n",
		 thread_id, cpu_id, writer_id);

	/* Use provided iteration count, or default to TEST_ITERATIONS */
	if (max_iterations == 0) {
		max_iterations = TEST_ITERATIONS;
	}

	/* Calculate the expected pattern from the writer's last iteration
	 * Writers ran for TEST_ITERATIONS/2, so their last iter was (TEST_ITERATIONS/2 - 1)
	 */
	int writer_last_iter = max_iterations - 1;
	uint8_t expected_pattern = (uint8_t)(writer_id * 10 + writer_last_iter);

	for (int iter = 0; iter < max_iterations; iter++) {
		/* Wait for signal to start iteration */
		k_sem_take(&sync_sem, K_FOREVER);

		/* Invalidate cache to get fresh data from memory */
		int ret = sys_cache_data_invd_range(data_ptr, CACHE_LINE_SIZE_MAX);

		if (ret == 0) {
			thread_stats[thread_id].invalidate_count++;
		}

		/* Read and verify data written by writer
		 * The data should be stable at the pattern from the writer's last iteration
		 */
		for (int i = 0; i < CACHE_LINE_SIZE_MAX; i++) {
			if (data_ptr[i] != (uint8_t)(expected_pattern + i)) {
				thread_stats[thread_id].coherency_errors++;
				cache_test_error = true;
				break;
			}
		}

		thread_stats[thread_id].iterations_completed++;

		/* Signal completion */
		k_sem_give(&completion_sem);
	}
}

/**
 * @brief Test: Cache line size detection
 *
 * Validates that cache line size can be detected or configured correctly
 * for both data and instruction caches.
 */

ZTEST(smp_cache, test_cache_line_size_detection)
{
	size_t dcache_line_size = sys_cache_data_line_size_get();
	size_t icache_line_size = sys_cache_instr_line_size_get();

	TC_PRINT("D-Cache line size: %zu bytes\n", dcache_line_size);
	TC_PRINT("I-Cache line size: %zu bytes\n", icache_line_size);

	/* Cache line size should be power of 2 if cache is enabled */
#if defined(CONFIG_DCACHE)
	if (dcache_line_size > 0) {
		zassert_true((dcache_line_size & (dcache_line_size - 1)) == 0,
			     "D-cache line size must be power of 2");
		zassert_true(dcache_line_size >= 16 && dcache_line_size <= 256,
			     "D-cache line size out of expected range");
	}
#endif

#if defined(CONFIG_ICACHE)
	if (icache_line_size > 0) {
		zassert_true((icache_line_size & (icache_line_size - 1)) == 0,
			     "I-cache line size must be power of 2");
		zassert_true(icache_line_size >= 16 && icache_line_size <= 256,
			     "I-cache line size out of expected range");
	}
#endif
}

/**
 * @brief Test: Cache coherency with multiple writers
 *
 * Tests cache coherency when multiple threads on different CPUs write to their
 * own cache lines and flush them. Verifies that writes are visible across CPUs.
 */
ZTEST(smp_cache, test_cache_coherency_writers)
{
	uint64_t start_time, end_time;
	uint32_t duration_ms;

	TC_PRINT("Testing cache coherency with multiple writers\n");

	/* Initialize synchronization */
	k_sem_init(&sync_sem, 0, NUM_TEST_THREADS);
	k_sem_init(&completion_sem, 0, NUM_TEST_THREADS);
	memset(thread_stats, 0, sizeof(thread_stats));
	cache_test_error = false;

	/* Initialize shared data */
	memset(shared_data, 0, sizeof(shared_data));

	/* Start timing */
	start_time = k_uptime_get();

	/* Create writer threads */
	for (int i = 0; i < NUM_TEST_THREADS; i++) {
		k_thread_create(&test_threads[i], test_stacks[i], STACK_SIZE,
				cache_coherency_writer_thread,
				INT_TO_POINTER(i), INT_TO_POINTER(TEST_ITERATIONS), NULL,
				K_PRIO_PREEMPT(5), 0, K_FOREVER);
		zassert_not_null(&test_threads[i], "Failed to create writer thread %d", i);
#ifdef CONFIG_SCHED_CPU_MASK
		int num_cpus = arch_num_cpus();
		int target_cpu = i % num_cpus;
		int ret = k_thread_cpu_pin(&test_threads[i], target_cpu);

		zassert_equal(ret, 0, "Failed to pin writer thread %d to CPU %d", i, target_cpu);
		TC_PRINT("Writer thread %d pinned to CPU %d\n", i, target_cpu);
#endif
		k_thread_start(&test_threads[i]);
	}

	/* Run iterations */
	for (int iter = 0; iter < TEST_ITERATIONS; iter++) {
		/* Signal all threads to start */
		for (int i = 0; i < NUM_TEST_THREADS; i++) {
			k_sem_give(&sync_sem);
		}

		/* Wait for all threads to complete */
		for (int i = 0; i < NUM_TEST_THREADS; i++) {
			k_sem_take(&completion_sem, K_FOREVER);
		}

		k_busy_wait(1000); /* Small delay between iterations */
	}

	/* Wait for threads to complete */
	for (int i = 0; i < NUM_TEST_THREADS; i++) {
		k_thread_join(&test_threads[i], K_FOREVER);
	}

	/* End timing */
	end_time = k_uptime_get();
	duration_ms = (uint32_t)(end_time - start_time);

	/* Print statistics and validate */
	TC_PRINT("\nWriter Test Statistics:\n");
	for (int i = 0; i < NUM_TEST_THREADS; i++) {
		TC_PRINT("Thread %d: Flushes=%u, Iterations=%u\n",
			 i, thread_stats[i].flush_count,
			 thread_stats[i].iterations_completed);
		zassert_equal(thread_stats[i].iterations_completed, TEST_ITERATIONS,
			      "Thread %d incomplete: %u of %u iterations",
			      i, thread_stats[i].iterations_completed, TEST_ITERATIONS);
		zassert_equal(thread_stats[i].coherency_errors, 0,
			      "Thread %d detected %u coherency errors",
			      i, thread_stats[i].coherency_errors);
	}

	TC_PRINT("\n========================================\n");
	TC_PRINT("Test Execution Time: %u ms (%u.%03u seconds)\n",
		 duration_ms, duration_ms / 1000, duration_ms % 1000);
	TC_PRINT("========================================\n");

	zassert_false(cache_test_error, "Cache coherency test detected errors");
	TC_PRINT("Cache coherency writer test passed\n");
}

/**
 * @brief Test: Cache coherency with reader-writer pattern
 *
 * Tests cache coherency when writers flush data and readers invalidate their
 * caches before reading. Simulates producer-consumer patterns across CPUs.
 */
ZTEST(smp_cache, test_cache_coherency_reader_writer)
{
	uint64_t start_time, end_time;
	uint32_t duration_ms;
	int num_cpus = arch_num_cpus();

	TC_PRINT("Testing cache coherency with readers and writers\n");
	TC_PRINT("Available CPUs: %d\n", num_cpus);

	/* Initialize synchronization */
	k_sem_init(&sync_sem, 0, NUM_TEST_THREADS * 2);
	k_sem_init(&completion_sem, 0, NUM_TEST_THREADS * 2);
	memset(thread_stats, 0, sizeof(thread_stats));
	cache_test_error = false;

	/* Initialize shared data */
	memset(shared_data, 0, sizeof(shared_data));

	/* Start timing */
	start_time = k_uptime_get();

	/* Create writer threads */
	for (int i = 0; i < NUM_TEST_THREADS; i++) {
		k_thread_create(&test_threads[i], test_stacks[i], STACK_SIZE,
				cache_coherency_writer_thread,
				INT_TO_POINTER(i), INT_TO_POINTER(TEST_ITERATIONS / 2), NULL,
				K_PRIO_PREEMPT(5), 0, K_FOREVER);

		/* Pin to specific CPU if supported */
#ifdef CONFIG_SCHED_CPU_MASK
		int target_cpu = i % num_cpus;
		int ret = k_thread_cpu_pin(&test_threads[i], target_cpu);

		zassert_equal(ret, 0, "Failed to pin writer thread %d to CPU %d", i, target_cpu);
		TC_PRINT("Writer thread %d pinned to CPU %d\n", i, target_cpu);
#endif
		k_thread_start(&test_threads[i]);
	}

	/* Small delay to let writers initialize */
	k_msleep(100);

	/* Run iterations with writer-reader phases */
	for (int iter = 0; iter < TEST_ITERATIONS / 2; iter++) {
		/* Writer phase */
		for (int i = 0; i < NUM_TEST_THREADS; i++) {
			k_sem_give(&sync_sem);
		}

		for (int i = 0; i < NUM_TEST_THREADS; i++) {
			k_sem_take(&completion_sem, K_FOREVER);
		}

		/* Ensure writes are visible */
		sys_cache_data_flush_all();
		k_busy_wait(SYNC_DELAY_US);
	}

	/* Wait for writer threads to complete */
	for (int i = 0; i < NUM_TEST_THREADS; i++) {
		k_thread_join(&test_threads[i], K_FOREVER);
	}

	/* Now create reader threads that will verify the final state */
	TC_PRINT("\nStarting reader threads...\n");

	/* Reset synchronization for reader phase */
	k_sem_init(&sync_sem, 0, NUM_TEST_THREADS);
	k_sem_init(&completion_sem, 0, NUM_TEST_THREADS);

	/* Create reader threads */
	for (int i = 0; i < NUM_TEST_THREADS; i++) {
		k_thread_create(&test_threads[i], test_stacks[i], STACK_SIZE,
				cache_coherency_reader_thread,
				INT_TO_POINTER(i), INT_TO_POINTER(TEST_ITERATIONS / 2), NULL,
				K_PRIO_PREEMPT(5), 0, K_FOREVER);

#ifdef CONFIG_SCHED_CPU_MASK
		int target_cpu = (i + 1) % num_cpus;  /* Different CPU than writer */
		int ret = k_thread_cpu_pin(&test_threads[i], target_cpu);


		zassert_equal(ret, 0, "Failed to pin reader thread %d to CPU %d", i, target_cpu);
		TC_PRINT("Reader thread %d pinned to CPU %d\n", i, target_cpu);
#endif
		k_thread_start(&test_threads[i]);
	}

	/* Run reader iterations */
	for (int iter = 0; iter < TEST_ITERATIONS / 2; iter++) {
		/* Reader phase */
		for (int i = 0; i < NUM_TEST_THREADS; i++) {
			k_sem_give(&sync_sem);
		}

		for (int i = 0; i < NUM_TEST_THREADS; i++) {
			k_sem_take(&completion_sem, K_FOREVER);
		}

		k_busy_wait(SYNC_DELAY_US / 10);
	}

	/* Wait for reader threads to complete */
	for (int i = 0; i < NUM_TEST_THREADS; i++) {
		k_thread_join(&test_threads[i], K_FOREVER);
	}

	/* End timing */
	end_time = k_uptime_get();
	duration_ms = (uint32_t)(end_time - start_time);

	/* Print statistics */
	TC_PRINT("\nReader-Writer Test Statistics:\n");
	uint32_t total_errors = 0;

	for (int i = 0; i < NUM_TEST_THREADS; i++) {
		TC_PRINT("Thread %d: Flushes=%u, Invalidates=%u, Errors=%u, Iterations=%u\n",
			 i, thread_stats[i].flush_count,
			 thread_stats[i].invalidate_count,
			 thread_stats[i].coherency_errors,
			 thread_stats[i].iterations_completed);
		total_errors += thread_stats[i].coherency_errors;
		zassert_equal(thread_stats[i].coherency_errors, 0,
			      "Thread %d detected %u coherency errors",
			      i, thread_stats[i].coherency_errors);
	}

	TC_PRINT("\n========================================\n");
	TC_PRINT("Test Execution Time: %u ms (%u.%03u seconds)\n",
		 duration_ms, duration_ms / 1000, duration_ms % 1000);
	TC_PRINT("Total coherency errors: %u\n", total_errors);
	TC_PRINT("========================================\n");

	zassert_false(cache_test_error, "Cache coherency test detected errors");
	zassert_equal(total_errors, 0, "No coherency errors should occur");
	TC_PRINT("Cache coherency reader-writer test passed\n");
}


/**
 * @brief Thread function for stress testing cache operations
 *
 * Performs various cache operations concurrently to stress test the cache subsystem.
 */
static void cache_stress_thread(void *arg1, void *arg2, void *arg3)
{
	int thread_id = POINTER_TO_INT(arg1);
	int cpu_id = get_curr_cpu();
	size_t cache_line_size = sys_cache_data_line_size_get();

	if (cache_line_size == 0) {
		cache_line_size = CACHE_LINE_SIZE_MAX;
	}

	/* Calculate buffer size per thread, aligned to cache line to prevent false sharing */
	size_t buffer_size = (TEST_BUFFER_SIZE / NUM_TEST_THREADS) & ~(cache_line_size - 1);
	uint8_t *buffer = &test_data_buffer[thread_id * buffer_size];

	/* Ensure we don't exceed buffer bounds */
	if ((thread_id + 1) * buffer_size > TEST_BUFFER_SIZE) {
		TC_PRINT("ERROR: Thread %d buffer would overflow!\n", thread_id);
		cache_test_error = true;
		atomic_dec(&thread_counter);
		return;
	}

	TC_PRINT("Stress thread %d running on CPU %d\n", thread_id, cpu_id);

	for (int iter = 0; iter < TEST_ITERATIONS * 2; iter++) {
		/* Phase 1: Write and flush */
		init_test_buffer(buffer, buffer_size, (uint8_t)(thread_id + iter));
		sys_cache_data_flush_range(buffer, buffer_size);
		thread_stats[thread_id].flush_count++;

		/* Phase 2: Invalidate and read */
		sys_cache_data_invd_range(buffer, buffer_size);
		thread_stats[thread_id].invalidate_count++;

		/* Verify data is still intact */
		if (!verify_test_buffer(buffer, buffer_size, (uint8_t)(thread_id + iter))) {
			thread_stats[thread_id].coherency_errors++;
			cache_test_error = true;
		}

		/* Phase 3: Flush and invalidate */
		sys_cache_data_flush_and_invd_range(buffer, buffer_size);

		thread_stats[thread_id].iterations_completed++;

		/* Occasional full cache operations */
		if (iter % 10 == 0) {
			sys_cache_data_flush_all();
		}

		k_yield(); /* Allow other threads to run */
	}

	atomic_dec(&thread_counter);
}

/**
 * @brief Test: Stress test for cache operations under heavy concurrent load
 *
 * Creates multiple threads that perform intensive cache operations
 * concurrently to stress test the cache management system.
 */
ZTEST(smp_cache, test_cache_operations_stress)
{
	uint64_t start_time, end_time;
	uint32_t duration_ms;

	TC_PRINT("Starting cache operations stress test\n");

	/* Initialize */
	memset(thread_stats, 0, sizeof(thread_stats));
	cache_test_error = false;
	atomic_set(&thread_counter, NUM_TEST_THREADS);

	/* Start timing */
	start_time = k_uptime_get();

	/* Create stress test threads */
	for (int i = 0; i < NUM_TEST_THREADS; i++) {
		k_thread_create(&test_threads[i], test_stacks[i], STACK_SIZE,
				cache_stress_thread,
				INT_TO_POINTER(i), NULL, NULL,
				K_PRIO_PREEMPT(5 + i), 0, K_FOREVER);
		k_thread_start(&test_threads[i]);
	}

	/* Wait for all threads to complete */
	while (atomic_get(&thread_counter) > 0) {
		k_msleep(100);
	}

	for (int i = 0; i < NUM_TEST_THREADS; i++) {
		k_thread_join(&test_threads[i], K_FOREVER);
	}

	/* End timing */
	end_time = k_uptime_get();
	duration_ms = (uint32_t)(end_time - start_time);

	/* Print statistics */
	TC_PRINT("\nStress Test Statistics:\n");
	uint32_t total_flushes = 0;
	uint32_t total_invalidates = 0;
	uint32_t total_errors = 0;

	for (int i = 0; i < NUM_TEST_THREADS; i++) {
		TC_PRINT("Thread %d: Flushes=%u, Invalidates=%u, Errors=%u, Iterations=%u\n",
			 i, thread_stats[i].flush_count,
			 thread_stats[i].invalidate_count,
			 thread_stats[i].coherency_errors,
			 thread_stats[i].iterations_completed);

		total_flushes += thread_stats[i].flush_count;
		total_invalidates += thread_stats[i].invalidate_count;
		total_errors += thread_stats[i].coherency_errors;
		zassert_equal(thread_stats[i].coherency_errors, 0,
			      "Thread %d detected %u coherency errors",
			      i, thread_stats[i].coherency_errors);
	}

	TC_PRINT("\nTotal: Flushes=%u, Invalidates=%u, Errors=%u\n",
		 total_flushes, total_invalidates, total_errors);

	TC_PRINT("\n========================================\n");
	TC_PRINT("Test Execution Time: %u ms (%u.%03u seconds)\n",
		 duration_ms, duration_ms / 1000, duration_ms % 1000);
	TC_PRINT("========================================\n");

	zassert_false(cache_test_error, "Cache stress test detected errors");
	zassert_equal(total_errors, 0, "Cache coherency errors detected");
	TC_PRINT("Cache operations stress test passed\n");
}

/**
 * @brief Test: Instruction cache coherency
 *
 * Tests instruction cache coherency across CPUs. Critical for scenarios like
 * JIT compilation or dynamic code loading.
 */
ZTEST(smp_cache, test_instruction_cache_coherency)
{
	size_t icache_line_size = sys_cache_instr_line_size_get();

	TC_PRINT("Testing instruction cache coherency\n");
	TC_PRINT("I-Cache line size: %zu bytes\n", icache_line_size);

	/* Perform data cache synchronization across all CPUs */
	TC_PRINT("Performing data cache synchronization across all CPUs\n");
	sys_cache_data_flush_and_invd_all();

	TC_PRINT("Instruction cache coherency test completed\n");
}

/**
 * @brief Test: Cache operation performance
 *
 * Measures performance of various cache operations including full flush,
 * range flush, invalidate, and combined operations.
 */
ZTEST(smp_cache, test_cache_performance)
{
	uint64_t start_time, end_time, duration_us;
	int ret;
	size_t cache_line_size = sys_cache_data_line_size_get();
	const int iterations = 1000;

	TC_PRINT("Testing cache operation performance\n");

	/* Test 1: D-cache full flush performance */
	start_time = k_uptime_get();
	for (int i = 0; i < iterations; i++) {
		sys_cache_data_flush_all();
	}
	end_time = k_uptime_get();
	duration_us = (end_time - start_time) * 1000 / iterations;
	TC_PRINT("D-cache full flush: avg %.2f us per operation\n",
		 (double)duration_us / 1000.0);

	/* Test 2: D-cache range flush performance */
	if (cache_line_size > 0) {
		size_t test_size = cache_line_size * 16;

		init_test_buffer(test_data_buffer, test_size, 0xC1);

		start_time = k_uptime_get();
		for (int i = 0; i < iterations; i++) {
			sys_cache_data_flush_range(test_data_buffer, test_size);
		}
		end_time = k_uptime_get();
		duration_us = (end_time - start_time) * 1000 / iterations;
		TC_PRINT("D-cache range flush (%zu bytes): avg %.2f us per operation\n",
			 test_size, (double)duration_us / 1000.0);
	}

	/* Test 3: D-cache invalidate performance */
	start_time = k_uptime_get();
	for (int i = 0; i < iterations; i++) {
		sys_cache_data_invd_all();
	}
	end_time = k_uptime_get();
	duration_us = (end_time - start_time) * 1000 / iterations;
	TC_PRINT("D-cache full invalidate: avg %.2f us per operation\n",
		 (double)duration_us / 1000.0);

	/* Test 4: Combined flush+invalidate performance */
	start_time = k_uptime_get();
	for (int i = 0; i < iterations; i++) {
		sys_cache_data_flush_and_invd_all();
	}
	end_time = k_uptime_get();
	duration_us = (end_time - start_time) * 1000 / iterations;
	TC_PRINT("D-cache full flush+invalidate: avg %.2f us per operation\n",
		 (double)duration_us / 1000.0);

	/* Test 5: Memory bandwidth estimation */
	size_t bandwidth_test_size = TEST_BUFFER_SIZE;

	init_test_buffer(test_data_buffer, bandwidth_test_size, 0xC2);

	start_time = k_uptime_get();
	ret = sys_cache_data_flush_range(test_data_buffer, bandwidth_test_size);
	end_time = k_uptime_get();

	if (ret == 0 && end_time > start_time) {
		duration_us = (end_time - start_time) * 1000;
		double mb_per_sec = (double)bandwidth_test_size / duration_us;

		TC_PRINT("Cache flush bandwidth: %.2f MB/s (%zu bytes in %llu us)\n",
			 mb_per_sec, bandwidth_test_size, duration_us);
	}

	TC_PRINT("Cache performance test completed\n");
}

/**
 * @brief Test: DMA-cache interaction (simulated)
 *
 * Simulates DMA operations to validate proper cache management patterns used
 * in device drivers. Tests cache flush/invalidate sequences for:
 * - Device-to-memory (DMA write): invalidate before CPU reads
 * - Memory-to-device (DMA read): flush before device accesses
 * - Bidirectional transfers with proper synchronization
 *
 * Note: Uses memset() to simulate DMA hardware directly modifying memory.
 */
ZTEST(smp_cache, test_dma_cache_interaction)
{
	int ret;
	size_t cache_line_size = sys_cache_data_line_size_get();
	size_t dma_buffer_size = 1024;

	TC_PRINT("Testing DMA-cache interaction\n");

	if (cache_line_size == 0) {
		TC_PRINT("Cache line size is 0, skipping DMA tests\n");
		ztest_test_skip();
		return;
	}

	/* Simulate DMA write scenario (device writes to memory, CPU reads) */
	TC_PRINT("\nScenario 1: DMA write (device -> memory -> CPU)\n");

	/* Step 1: Initialize buffer (simulate memory state) */
	init_test_buffer(test_data_buffer, dma_buffer_size, 0xE1);
	TC_PRINT("  1. Buffer initialized with pattern 0xE1\n");

	/* Step 2: Flush to ensure data is in memory */
	ret = sys_cache_data_flush_range(test_data_buffer, dma_buffer_size);
	TC_PRINT("  2. Flushed cache to memory: ret=%d\n", ret);

	/* Step 3: Simulate DMA write (modify memory directly) */
	memset(test_data_buffer, 0xE2, dma_buffer_size);
	TC_PRINT("  3. Simulated DMA write (memory updated to 0xE2)\n");

	/* Step 4: Invalidate cache before CPU reads (critical!) */
	ret = sys_cache_data_invd_range(test_data_buffer, dma_buffer_size);
	TC_PRINT("  4. Invalidated cache before CPU read: ret=%d\n", ret);

	/* Step 5: Verify CPU sees DMA data */
	bool dma_data_visible = (test_data_buffer[0] == 0xE2);

	TC_PRINT("  5. CPU read verification: %s\n",
		 dma_data_visible ? "PASS - DMA data visible" : "FAIL - stale cache data");
	zassert_true(dma_data_visible, "CPU should see DMA-written data after invalidate");

	/* Simulate DMA read scenario (CPU writes to memory, device reads) */
	TC_PRINT("\nScenario 2: DMA read (CPU -> memory -> device)\n");

	/* Step 1: CPU writes data */
	init_test_buffer(test_data_buffer, dma_buffer_size, 0xE3);
	TC_PRINT("  1. CPU wrote data pattern 0xE3\n");

	/* Step 2: Flush to push CPU writes to memory (critical!) */
	ret = sys_cache_data_flush_range(test_data_buffer, dma_buffer_size);
	TC_PRINT("  2. Flushed cache to memory for DMA: ret=%d\n", ret);

	/* Step 3: Verify data is in memory (simulation check) */
	sys_cache_data_invd_range(test_data_buffer, dma_buffer_size);
	bool cpu_data_flushed = (test_data_buffer[0] == 0xE3);

	TC_PRINT("  3. Memory verification: %s\n",
		 cpu_data_flushed ? "PASS - CPU data in memory" : "FAIL - data not flushed");
	zassert_true(cpu_data_flushed, "CPU data should be in memory after flush");

	/* Simulate bidirectional DMA (both read and write) */
	TC_PRINT("\nScenario 3: Bidirectional DMA\n");

	/* Step 1: CPU prepares output buffer */
	init_test_buffer(test_data_buffer, dma_buffer_size / 2, 0xE4);
	TC_PRINT("  1. CPU prepared output data\n");

	/* Step 2: Flush output for device */
	ret = sys_cache_data_flush_range(test_data_buffer, dma_buffer_size / 2);
	TC_PRINT("  2. Flushed output buffer: ret=%d\n", ret);

	/* Step 3: Invalidate input buffer */
	uint8_t *input_buffer = test_data_buffer + dma_buffer_size / 2;

	ret = sys_cache_data_invd_range(input_buffer, dma_buffer_size / 2);
	TC_PRINT("  3. Invalidated input buffer: ret=%d\n", ret);

	/* Step 4: Simulate device operation */
	memset(input_buffer, 0xE5, dma_buffer_size / 2);
	TC_PRINT("  4. Simulated device processing\n");

	/* Step 5: Invalidate again before reading input */
	ret = sys_cache_data_invd_range(input_buffer, dma_buffer_size / 2);
	TC_PRINT("  5. Invalidated input buffer again: ret=%d\n", ret);

	/* Step 6: Verify CPU sees device results */
	bool device_data_visible = (input_buffer[0] == 0xE5);

	TC_PRINT("  6. Result verification: %s\n",
		 device_data_visible ? "PASS" : "FAIL");
	zassert_true(device_data_visible, "CPU should see device results");

	TC_PRINT("\nDMA-cache interaction test completed\n");
}

/**
 * @brief Test suite setup function
 */
static void *smp_cache_setup(void)
{
	/* Initialize test buffers */
	init_test_buffer(test_data_buffer, TEST_BUFFER_SIZE, 0x00);

	/* Enable caches */
	sys_cache_data_enable();
	sys_cache_instr_enable();

	/* Allow system to stabilize */
	k_msleep(100);

	TC_PRINT("\n========================================\n");
	TC_PRINT("SMP Cache Coherency Test Suite\n");
	TC_PRINT("CPUs: %d\n", arch_num_cpus());
	TC_PRINT("D-Cache line size: %zu bytes\n", sys_cache_data_line_size_get());
	TC_PRINT("I-Cache line size: %zu bytes\n", sys_cache_instr_line_size_get());
	TC_PRINT("========================================\n\n");

	return NULL;
}

/**
 * @brief Test suite teardown function
 */
static void smp_cache_teardown(void *fixture)
{
	ARG_UNUSED(fixture);

	/* Flush all caches to clean state */
	sys_cache_data_flush_all();
	sys_cache_instr_flush_all();
}

ZTEST_SUITE(smp_cache, NULL, smp_cache_setup, NULL, NULL, smp_cache_teardown);
