/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SMP Cache Coherency Stress Test Suite
 *
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/cache.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/sys/atomic.h>
#include <string.h>

#if CONFIG_MP_MAX_NUM_CPUS < 2
#error "SMP cache test requires at least two CPUs!"
#endif

#define STACK_SIZE        (4096 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define CACHE_LINE_SIZE   64
#define NUM_CACHE_LINES   256
#define NUM_TEST_THREADS  CONFIG_MP_MAX_NUM_CPUS
#define STRESS_ITERATIONS 50000

/* Thread stacks and structures */
K_THREAD_STACK_ARRAY_DEFINE(test_stacks, NUM_TEST_THREADS, STACK_SIZE);
static struct k_thread test_threads[NUM_TEST_THREADS];

/* Synchronization */
static atomic_t barrier_count;
static atomic_t barrier_sense;
static int thread_local_sense[NUM_TEST_THREADS];

/*
 * ============================================================================
 * Cache Line Data Structures
 * ============================================================================
 */

/* Cache line with value and checksum for integrity verification */
struct cache_line_data {
	volatile uint32_t value;
	volatile uint32_t sequence;
	volatile uint32_t writer_cpu;
	volatile uint32_t checksum;
	uint8_t pad[CACHE_LINE_SIZE - 4 * sizeof(uint32_t)];
} __aligned(CACHE_LINE_SIZE);

/* Per-thread error counters for detailed reporting */
struct thread_error_stats {
	uint32_t checksum_errors;
	uint32_t sequence_errors;
	uint32_t value_errors;
	uint32_t total_reads;
	uint32_t total_writes;
};

static struct thread_error_stats thread_stats[NUM_TEST_THREADS];

/*
 * ============================================================================
 * Utility Functions
 * ============================================================================
 */

static inline int get_curr_cpu(void)
{
	unsigned int key = arch_irq_lock();
	int cpu_id = arch_curr_cpu()->id;

	arch_irq_unlock(key);
	return cpu_id;
}

static void spin_barrier(int num_participants, int thread_id)
{
	thread_local_sense[thread_id] = !thread_local_sense[thread_id];
	int local_sense = thread_local_sense[thread_id];

	barrier_dsync_fence_full();

	int count = atomic_inc(&barrier_count) + 1;

	if (count == num_participants) {
		atomic_set(&barrier_count, 0);
		atomic_set(&barrier_sense, local_sense);
	} else {
		while (atomic_get(&barrier_sense) != local_sense) {
			arch_nop();
		}
	}

	barrier_dsync_fence_full();
}

static inline uint32_t compute_checksum(uint32_t value, uint32_t sequence, uint32_t cpu)
{
	return (value ^ sequence ^ cpu ^ 0xDEADBEEF);
}

/*
 * ============================================================================
 * TEST 1: Strict Write-Wait-Read Cache Test
 * ============================================================================
 * For each cache line:
 *   - One CPU writes a pattern
 *   - Other CPU WAITS for write to complete, then reads and verifies
 *   - Every write has exactly one corresponding read
 *
 * This ensures 1:1 write/read verification.
 */

/* Per cache line synchronization - writer sets ready, reader clears it */
struct cache_line_sync {
	volatile uint32_t value;
	volatile uint32_t iteration;
	volatile uint32_t writer_cpu;
	volatile uint32_t checksum;
	volatile uint32_t ready;       /* 0 = empty, 1 = data ready for read */
	volatile uint32_t ack;         /* 0 = not ack'd, 1 = reader done */
	uint8_t pad[CACHE_LINE_SIZE - 6 * sizeof(uint32_t)];
} __aligned(CACHE_LINE_SIZE);

static struct cache_line_sync __aligned(CACHE_LINE_SIZE)
	sync_data[NUM_CACHE_LINES];

static void writer_thread(void *arg1, void *arg2, void *arg3)
{
	int thread_id = POINTER_TO_INT(arg1);
	int total_threads = POINTER_TO_INT(arg2);

	ARG_UNUSED(arg3);

	int cpu_id = get_curr_cpu();
	int num_writers = total_threads / 2;
	int lines_per_writer = NUM_CACHE_LINES / num_writers;
	int my_line_start = thread_id * lines_per_writer;
	int my_line_end = my_line_start + lines_per_writer;

	memset(&thread_stats[thread_id], 0, sizeof(thread_stats[thread_id]));

	spin_barrier(total_threads, thread_id);

	for (int iter = 0; iter < STRESS_ITERATIONS; iter++) {
		/* Write to each of my cache lines */
		for (int line_idx = my_line_start; line_idx < my_line_end; line_idx++) {
			/* Wait for reader to acknowledge previous data */
			while (sync_data[line_idx].ready != 0) {
				arch_nop();
			}

			/* Write data pattern */
			uint32_t value = (iter << 16) | (thread_id << 8) | (line_idx & 0xFF);

			sync_data[line_idx].value = value;
			sync_data[line_idx].iteration = iter;
			sync_data[line_idx].writer_cpu = cpu_id;
			barrier_dmem_fence_full();

			/* Compute and write checksum */
			sync_data[line_idx].checksum = compute_checksum(value, iter, cpu_id);
			barrier_dmem_fence_full();

			/* Signal data is ready */
			sync_data[line_idx].ready = 1;

			thread_stats[thread_id].total_writes++;
		}
	}

	spin_barrier(total_threads, thread_id);
}

static void reader_thread(void *arg1, void *arg2, void *arg3)
{
	int thread_id = POINTER_TO_INT(arg1);
	int total_threads = POINTER_TO_INT(arg2);

	ARG_UNUSED(arg3);

	int num_writers = total_threads / 2;
	int writer_id = thread_id - num_writers;  /* My corresponding writer */
	int lines_per_writer = NUM_CACHE_LINES / num_writers;
	int my_line_start = writer_id * lines_per_writer;
	int my_line_end = my_line_start + lines_per_writer;

	memset(&thread_stats[thread_id], 0, sizeof(thread_stats[thread_id]));

	spin_barrier(total_threads, thread_id);

	for (int iter = 0; iter < STRESS_ITERATIONS; iter++) {
		/* Read from each of my cache lines */
		for (int line_idx = my_line_start; line_idx < my_line_end; line_idx++) {
			/* WAIT for writer to signal data is ready */
			while (sync_data[line_idx].ready != 1) {
				arch_nop();
			}

			barrier_dmem_fence_full();

			/* Read all data */
			uint32_t value = sync_data[line_idx].value;
			uint32_t iteration = sync_data[line_idx].iteration;
			uint32_t writer_cpu = sync_data[line_idx].writer_cpu;
			uint32_t checksum = sync_data[line_idx].checksum;

			thread_stats[thread_id].total_reads++;

			/* Verify checksum */
			uint32_t expected = compute_checksum(value, iteration, writer_cpu);

			if (checksum != expected) {
				thread_stats[thread_id].checksum_errors++;
			}

			/* Verify data pattern */
			uint32_t expected_value = (iter << 16) | (writer_id << 8) |
						  (line_idx & 0xFF);
			if (value != expected_value) {
				thread_stats[thread_id].value_errors++;
			}

			/* Verify iteration */
			if (iteration != (uint32_t)iter) {
				thread_stats[thread_id].sequence_errors++;
			}

			barrier_dmem_fence_full();

			/* Acknowledge - signal writer we're done */
			sync_data[line_idx].ready = 0;
		}
	}

	spin_barrier(total_threads, thread_id);
}

ZTEST(smp_cache, test_cache_stress)
{
	int total_threads = NUM_TEST_THREADS;
	int num_writers = total_threads / 2;
	int num_readers = total_threads / 2;

	if (num_writers < 1 || num_readers < 1) {
		TC_PRINT("Skipping: need at least 2 CPUs\n");
		ztest_test_skip();
		return;
	}

	TC_PRINT("=== Strict Write-Wait-Read Cache Test ===\n");
	TC_PRINT("CPUs: %d, Writers: %d, Readers: %d\n",
		 total_threads, num_writers, num_readers);
	TC_PRINT("Cache lines: %d, Iterations per thread: %d\n",
		 NUM_CACHE_LINES, STRESS_ITERATIONS);
	TC_PRINT("Lines per writer/reader pair: %d\n", NUM_CACHE_LINES / num_writers);

	memset(sync_data, 0, sizeof(sync_data));
	memset(thread_local_sense, 0, sizeof(thread_local_sense));
	atomic_set(&barrier_count, 0);
	atomic_set(&barrier_sense, 0);

	uint64_t start_time = k_uptime_get();

	/* Create writer threads (first half) */
	for (int i = 0; i < num_writers; i++) {
		k_thread_create(&test_threads[i], test_stacks[i], STACK_SIZE,
				writer_thread,
				INT_TO_POINTER(i),
				INT_TO_POINTER(total_threads),
				NULL,
				K_PRIO_PREEMPT(5), 0, K_FOREVER);
#if defined(CONFIG_SCHED_CPU_MASK)
		k_thread_cpu_pin(&test_threads[i], i % CONFIG_MP_MAX_NUM_CPUS);
#endif
	}

	/* Create reader threads (second half) - pin to different CPUs */
	for (int i = 0; i < num_readers; i++) {
		int thread_idx = num_writers + i;

		k_thread_create(&test_threads[thread_idx], test_stacks[thread_idx],
				STACK_SIZE, reader_thread,
				INT_TO_POINTER(thread_idx),
				INT_TO_POINTER(total_threads),
				NULL,
				K_PRIO_PREEMPT(5), 0, K_FOREVER);
#if defined(CONFIG_SCHED_CPU_MASK)
		/* Pin readers to different CPUs than their writers */
		k_thread_cpu_pin(&test_threads[thread_idx],
				 (num_writers + i) % CONFIG_MP_MAX_NUM_CPUS);
#endif
	}

	/* Start all threads */
	for (int i = 0; i < total_threads; i++) {
		k_thread_start(&test_threads[i]);
	}

	for (int i = 0; i < total_threads; i++) {
		k_thread_join(&test_threads[i], K_FOREVER);
	}

	uint64_t end_time = k_uptime_get();
	uint32_t duration_ms = (uint32_t)(end_time - start_time);

	/* Aggregate results */
	uint32_t total_checksum_errors = 0;
	uint32_t total_value_errors = 0;
	uint32_t total_sequence_errors = 0;
	uint32_t total_reads = 0;
	uint32_t total_writes = 0;

	for (int i = 0; i < num_writers; i++) {
		total_writes += thread_stats[i].total_writes;
	}
	for (int i = num_writers; i < total_threads; i++) {
		total_checksum_errors += thread_stats[i].checksum_errors;
		total_value_errors += thread_stats[i].value_errors;
		total_sequence_errors += thread_stats[i].sequence_errors;
		total_reads += thread_stats[i].total_reads;
	}

	TC_PRINT("Duration: %u ms\n", duration_ms);
	TC_PRINT("Total writes: %u, Total reads: %u\n", total_writes, total_reads);
	TC_PRINT("Checksum errors: %u, Value errors: %u, Sequence errors: %u\n",
		 total_checksum_errors, total_value_errors, total_sequence_errors);

	/* Check for failures and report */
	if (total_writes != total_reads ||
	    total_checksum_errors > 0 ||
	    total_value_errors > 0 ||
	    total_sequence_errors > 0) {
		TC_PRINT("FAILED: Cache coherency test detected errors!\n");
	}

	zassert_equal(total_writes, total_reads,
		      "Write/Read mismatch: %u writes vs %u reads",
		      total_writes, total_reads);
	zassert_equal(total_checksum_errors, 0,
		      "Cache coherency errors: %u checksum mismatches",
		      total_checksum_errors);
	zassert_equal(total_value_errors, 0,
		      "Cache coherency errors: %u value mismatches",
		      total_value_errors);
	zassert_equal(total_sequence_errors, 0,
		      "Cache coherency errors: %u sequence mismatches",
		      total_sequence_errors);

	TC_PRINT("PASSED\n\n");
}

/*
 * ============================================================================
 * TEST 2: Producer-Consumer Cache Test
 * ============================================================================
 * Producer CPUs write data with checksums, consumer CPUs read and verify.
 * Tests that data written on one CPU is correctly visible to other CPUs.
 */

struct prod_cons_buffer {
	volatile uint32_t data[14];
	volatile uint32_t checksum;
	volatile uint32_t ready;
} __aligned(CACHE_LINE_SIZE);

static struct prod_cons_buffer __aligned(CACHE_LINE_SIZE)
	pc_buffers[NUM_TEST_THREADS / 2];

static void producer_thread(void *arg1, void *arg2, void *arg3)
{
	int thread_id = POINTER_TO_INT(arg1);
	int total_threads = POINTER_TO_INT(arg2);
	int pair_id = thread_id;

	ARG_UNUSED(arg3);

	memset(&thread_stats[thread_id], 0, sizeof(thread_stats[thread_id]));

	spin_barrier(total_threads, thread_id);

	for (int iter = 0; iter < STRESS_ITERATIONS; iter++) {
		/* Write data pattern */
		uint32_t checksum = 0;

		for (int i = 0; i < 14; i++) {
			uint32_t val = (iter << 16) | (thread_id << 8) | i;

			pc_buffers[pair_id].data[i] = val;
			checksum ^= val;
		}

		barrier_dmem_fence_full();
		pc_buffers[pair_id].checksum = checksum;
		barrier_dmem_fence_full();
		pc_buffers[pair_id].ready = iter + 1;

		thread_stats[thread_id].total_writes++;

		/* Wait for consumer to acknowledge */
		while (pc_buffers[pair_id].ready != 0) {
			arch_nop();
		}
	}

	spin_barrier(total_threads, thread_id);
}

static void consumer_thread(void *arg1, void *arg2, void *arg3)
{
	int thread_id = POINTER_TO_INT(arg1);
	int total_threads = POINTER_TO_INT(arg2);
	int pair_id = thread_id - (total_threads / 2);

	ARG_UNUSED(arg3);

	memset(&thread_stats[thread_id], 0, sizeof(thread_stats[thread_id]));

	spin_barrier(total_threads, thread_id);

	for (int iter = 0; iter < STRESS_ITERATIONS; iter++) {
		/* Wait for producer */
		while (pc_buffers[pair_id].ready != iter + 1) {
			arch_nop();
		}

		barrier_dmem_fence_full();

		/* Read and verify checksum */
		uint32_t expected_checksum = pc_buffers[pair_id].checksum;
		uint32_t actual_checksum = 0;

		for (int i = 0; i < 14; i++) {
			actual_checksum ^= pc_buffers[pair_id].data[i];
		}

		thread_stats[thread_id].total_reads++;

		if (actual_checksum != expected_checksum) {
			thread_stats[thread_id].checksum_errors++;
		}

		/* Verify data pattern */
		int producer_id = pair_id;

		for (int i = 0; i < 14; i++) {
			uint32_t expected = (iter << 16) | (producer_id << 8) | i;
			uint32_t actual = pc_buffers[pair_id].data[i];

			if (actual != expected) {
				thread_stats[thread_id].value_errors++;
				break;
			}
		}

		/* Signal done */
		pc_buffers[pair_id].ready = 0;
	}

	spin_barrier(total_threads, thread_id);
}

ZTEST(smp_cache, test_producer_consumer)
{
	int total_threads = NUM_TEST_THREADS;
	int num_pairs = total_threads / 2;

	if (num_pairs < 1) {
		TC_PRINT("Skipping: need at least 2 CPUs\n");
		ztest_test_skip();
		return;
	}

	TC_PRINT("=== Producer-Consumer Cache Test ===\n");
	TC_PRINT("Producer-Consumer pairs: %d, Iterations: %d\n",
		 num_pairs, STRESS_ITERATIONS);

	memset(pc_buffers, 0, sizeof(pc_buffers));
	memset(thread_local_sense, 0, sizeof(thread_local_sense));
	atomic_set(&barrier_count, 0);
	atomic_set(&barrier_sense, 0);

	uint64_t start_time = k_uptime_get();

	/* Create producer threads */
	for (int i = 0; i < num_pairs; i++) {
		k_thread_create(&test_threads[i], test_stacks[i], STACK_SIZE,
				producer_thread,
				INT_TO_POINTER(i),
				INT_TO_POINTER(total_threads),
				NULL,
				K_PRIO_PREEMPT(5), 0, K_FOREVER);
#if defined(CONFIG_SCHED_CPU_MASK)
		k_thread_cpu_pin(&test_threads[i], i % CONFIG_MP_MAX_NUM_CPUS);
#endif
	}

	/* Create consumer threads */
	for (int i = 0; i < num_pairs; i++) {
		int thread_idx = num_pairs + i;

		k_thread_create(&test_threads[thread_idx], test_stacks[thread_idx],
				STACK_SIZE, consumer_thread,
				INT_TO_POINTER(thread_idx),
				INT_TO_POINTER(total_threads),
				NULL,
				K_PRIO_PREEMPT(5), 0, K_FOREVER);
#if defined(CONFIG_SCHED_CPU_MASK)
		k_thread_cpu_pin(&test_threads[thread_idx],
				 (num_pairs + i) % CONFIG_MP_MAX_NUM_CPUS);
#endif
	}

	/* Start all threads */
	for (int i = 0; i < total_threads; i++) {
		k_thread_start(&test_threads[i]);
	}

	for (int i = 0; i < total_threads; i++) {
		k_thread_join(&test_threads[i], K_FOREVER);
	}

	uint64_t end_time = k_uptime_get();
	uint32_t duration_ms = (uint32_t)(end_time - start_time);

	/* Aggregate consumer errors */
	uint32_t total_checksum_errors = 0;
	uint32_t total_value_errors = 0;

	for (int i = num_pairs; i < total_threads; i++) {
		total_checksum_errors += thread_stats[i].checksum_errors;
		total_value_errors += thread_stats[i].value_errors;
	}

	TC_PRINT("Duration: %u ms\n", duration_ms);
	TC_PRINT("Checksum errors: %u, Value errors: %u\n",
		 total_checksum_errors, total_value_errors);

	/* Check for failures and report */
	if (total_checksum_errors > 0 || total_value_errors > 0) {
		TC_PRINT("FAILED: Producer-Consumer test detected errors!\n");
	}

	zassert_equal(total_checksum_errors, 0,
		      "Cache coherency errors: %u checksum mismatches",
		      total_checksum_errors);
	zassert_equal(total_value_errors, 0,
		      "Cache coherency errors: %u value mismatches",
		      total_value_errors);

	TC_PRINT("PASSED\n\n");
}

/*
 * ============================================================================
 * Test Suite Setup/Teardown
 * ============================================================================
 */

static void *smp_cache_setup(void)
{
	TC_PRINT("\n");
	TC_PRINT("========================================\n");
	TC_PRINT("SMP Cache Coherency Test Suite\n");
	TC_PRINT("========================================\n");
	TC_PRINT("CPUs: %d\n", arch_num_cpus());
	TC_PRINT("Cache line size: %d bytes\n", CACHE_LINE_SIZE);
	TC_PRINT("Test data size: %zu bytes (%d cache lines)\n",
		 sizeof(sync_data), NUM_CACHE_LINES);
	TC_PRINT("========================================\n\n");
	return NULL;
}

static void smp_cache_teardown(void *fixture)
{
	ARG_UNUSED(fixture);
	TC_PRINT("========================================\n");
	TC_PRINT("All tests completed\n");
	TC_PRINT("========================================\n");
}

ZTEST_SUITE(smp_cache, NULL, smp_cache_setup, NULL, NULL, smp_cache_teardown);
