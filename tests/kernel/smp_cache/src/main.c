/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SMP Cache Coherency Test Suite
 *
 * Tests true SMP cache functionality using a "rotate writer" pattern:
 * one CPU writes data and flushes caches, while ALL other CPUs invalidate
 * and read/verify the data. The writer role rotates through every CPU,
 * so each core exercises both the write-flush and invalidate-read paths.
 *
 * Synchronization uses monotonically increasing sequence numbers:
 *   - Odd seq (2R+1):  data for round R is ready, readers may read.
 *   - Even seq (2R+2): round R is complete, next round may proceed.
 *   - reader_ack[i]:   reader i sets this to the odd seq value after
 *                       verifying, telling the writer "I'm done".
 *
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/cache.h>
#include <string.h>

BUILD_ASSERT(CONFIG_MP_MAX_NUM_CPUS >= 2,
	     "SMP cache test requires at least two CPUs");

#define CACHE_LINE_SIZE   CONFIG_DCACHE_LINE_SIZE
BUILD_ASSERT(CACHE_LINE_SIZE > 0,
	     "CONFIG_DCACHE_LINE_SIZE must be set for cache coherency tests");

#define STACK_SIZE            (4096 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define NUM_CACHE_LINES       16
#define NUM_TEST_THREADS      CONFIG_MP_MAX_NUM_CPUS
#define ITERATIONS_PER_WRITER 200

K_THREAD_STACK_ARRAY_DEFINE(test_stacks, NUM_TEST_THREADS, STACK_SIZE);
static struct k_thread test_threads[NUM_TEST_THREADS];

/* Per-thread error stats, padded to one cache line to prevent false sharing */
struct thread_error_stats {
	union {
		struct {
			uint32_t checksum_errors;
			uint32_t value_errors;
			uint32_t total_reads;
			uint32_t total_writes;
		};
		uint8_t _pad[CACHE_LINE_SIZE];
	};
};

BUILD_ASSERT(sizeof(struct thread_error_stats) == CACHE_LINE_SIZE,
	     "thread_error_stats must be exactly one cache line");

static struct thread_error_stats __aligned(CACHE_LINE_SIZE)
	thread_stats[NUM_TEST_THREADS];

/* Test data structure - exactly one cache line */
struct test_cache_line {
	uint32_t data[CACHE_LINE_SIZE / sizeof(uint32_t) - 4];
	uint32_t sequence;      /* 4 bytes */
	uint32_t writer_id;     /* 4 bytes - logical writer ID */
	uint32_t checksum;      /* 4 bytes */
	uint32_t reserved;      /* 4 bytes padding */
};

BUILD_ASSERT(sizeof(struct test_cache_line) == CACHE_LINE_SIZE,
	     "test_cache_line must be exactly one cache line");

#define DATA_WORDS (CACHE_LINE_SIZE / sizeof(uint32_t) - 4)

#define DATA_PATTERN(iter, writer, line_idx, word) \
	(((iter) << 20) | ((writer) << 12) | ((line_idx) << 4) | (word))

struct cache_flag {
	volatile uint32_t value;
	uint8_t reserved[CACHE_LINE_SIZE - sizeof(uint32_t)];
};

BUILD_ASSERT(sizeof(struct cache_flag) == CACHE_LINE_SIZE,
	     "cache_flag must be exactly one cache line");

/* Shared data: written by current writer, read by all other CPUs */
static struct test_cache_line __aligned(CACHE_LINE_SIZE)
	shared_data[NUM_CACHE_LINES];

/*
 * round_seq: global round sequence counter (monotonically increasing).
 * reader_ack[i]: per-reader acknowledgment flag.
 */
static struct cache_flag __aligned(CACHE_LINE_SIZE) round_seq;
static struct cache_flag __aligned(CACHE_LINE_SIZE) reader_ack[NUM_TEST_THREADS];

/* Track which cache operation variant to use */
enum cache_op_mode {
	CACHE_OP_FLUSH_PLUS_INVD,       /* separate flush + invd */
	CACHE_OP_FLUSH_AND_INVD,        /* combined flush_and_invd */
};

static enum cache_op_mode current_cache_op_mode;

static uint32_t compute_checksum(const struct test_cache_line *line)
{
	uint32_t sum = 0;
	int i;

	for (i = 0; i < (int)DATA_WORDS; i++) {
		sum ^= line->data[i];
	}
	sum ^= line->sequence;
	sum ^= line->writer_id;

	return sum;
}

/**
 * Flush data to memory using the current cache operation mode.
 * FLUSH_PLUS_INVD: sys_cache_data_flush_range()
 * FLUSH_AND_INVD:  sys_cache_data_flush_and_invd_range()
 */
static void flush_data(void *addr, size_t size)
{
	int ret;

	if (current_cache_op_mode == CACHE_OP_FLUSH_AND_INVD) {
		ret = sys_cache_data_flush_and_invd_range(addr, size);
	} else {
		ret = sys_cache_data_flush_range(addr, size);
	}
	zassert_ok(ret, "cache flush failed: %d", ret);
}

/**
 * Invalidate data from cache using the current cache operation mode.
 * FLUSH_PLUS_INVD: sys_cache_data_invd_range()
 * FLUSH_AND_INVD:  sys_cache_data_flush_and_invd_range()
 */
static void invd_data(void *addr, size_t size)
{
	int ret;

	if (current_cache_op_mode == CACHE_OP_FLUSH_AND_INVD) {
		ret = sys_cache_data_flush_and_invd_range(addr, size);
	} else {
		ret = sys_cache_data_invd_range(addr, size);
	}
	zassert_ok(ret, "cache invalidate failed: %d", ret);
}

/*
 * Unified thread function for the "rotate writer" pattern.
 *
 * All CPUs run this same function. The total number of rounds is
 * num_threads * ITERATIONS_PER_WRITER. Each CPU gets ITERATIONS_PER_WRITER
 * consecutive rounds as writer. When not the writer, a CPU acts as a reader.
 *
 * Protocol per round R (writer = R / ITERATIONS_PER_WRITER):
 *   1. Writer: write data → flush → set round_seq = 2R+1 (odd) → flush
 *   2. Readers: poll round_seq >= 2R+1 → invd data → verify → set
 *      reader_ack[my_id] = 2R+1 → flush
 *   3. Writer: poll all reader_ack[r] >= 2R+1
 *   4. Writer: set round_seq = 2R+2 (even) → flush
 *   5. Readers: poll round_seq >= 2R+2 → proceed to next round
 *
 */
static void smp_cache_thread(void *arg1, void *arg2, void *arg3)
{
	int my_id = POINTER_TO_INT(arg1);
	int num_threads = POINTER_TO_INT(arg2);
	uint32_t total_rounds;
	uint32_t round;
	int writer_id;
	int iter;
	uint32_t seq_data_ready;
	uint32_t seq_round_done;
	int idx, i, r;

	ARG_UNUSED(arg3);

	total_rounds = (uint32_t)num_threads * ITERATIONS_PER_WRITER;
	memset(&thread_stats[my_id], 0, sizeof(thread_stats[my_id]));

	for (round = 0; round < total_rounds; round++) {
		writer_id = (int)(round / ITERATIONS_PER_WRITER);
		iter = (int)(round % ITERATIONS_PER_WRITER);
		seq_data_ready = round * 2 + 1;   /* odd */
		seq_round_done = round * 2 + 2;   /* even */

		if (my_id == writer_id) {
			/*
			 * === WRITER: write data, flush, signal all readers ===
			 */

			/* Write test pattern to all shared cache lines */
			for (idx = 0; idx < NUM_CACHE_LINES; idx++) {
				for (i = 0; i < (int)DATA_WORDS; i++) {
					shared_data[idx].data[i] =
						DATA_PATTERN(iter, writer_id,
							     idx, i);
				}
				shared_data[idx].sequence = iter;
				shared_data[idx].writer_id = writer_id;
				shared_data[idx].checksum =
					compute_checksum(&shared_data[idx]);
			}

			/* Flush all written data to main memory */
			flush_data(shared_data, sizeof(shared_data));

			/* Signal: data is ready (seq becomes odd) */
			round_seq.value = seq_data_ready;
			flush_data(&round_seq, sizeof(struct cache_flag));

			thread_stats[my_id].total_writes += NUM_CACHE_LINES;

			/* Wait for every reader to acknowledge */
			for (r = 0; r < num_threads; r++) {
				if (r == my_id) {
					continue;
				}
				while (reader_ack[r].value < seq_data_ready) {
					invd_data(&reader_ack[r],
						  sizeof(struct cache_flag));
					k_busy_wait(1);
				}
			}

			/* Signal: round complete (seq becomes even) */
			round_seq.value = seq_round_done;
			flush_data(&round_seq, sizeof(struct cache_flag));
		} else {
			/*
			 * === READER: wait, invalidate, verify, acknowledge ===
			 */

			/* Wait for writer to signal data ready */
			while (round_seq.value < seq_data_ready) {
				invd_data(&round_seq,
					  sizeof(struct cache_flag));
				k_busy_wait(1);
			}

			/* Invalidate shared data to get fresh copy */
			invd_data(shared_data, sizeof(shared_data));

			/* Verify every cache line */
			for (idx = 0; idx < NUM_CACHE_LINES; idx++) {
				uint32_t stored_cs;
				uint32_t computed_cs;
				uint32_t expected;

				stored_cs = shared_data[idx].checksum;
				computed_cs =
					compute_checksum(&shared_data[idx]);
				if (stored_cs != computed_cs) {
					thread_stats[my_id].checksum_errors++;
				}

				if (shared_data[idx].writer_id !=
				    (uint32_t)writer_id) {
					thread_stats[my_id].value_errors++;
				}

				if (shared_data[idx].sequence !=
				    (uint32_t)iter) {
					thread_stats[my_id].value_errors++;
				}

				for (i = 0; i < (int)DATA_WORDS; i++) {
					expected = DATA_PATTERN(iter,
								writer_id,
								idx, i);
					if (shared_data[idx].data[i] !=
					    expected) {
						TC_PRINT("CPU%d mismatch: "
							 "writer=%d line=%d "
							 "word=%d got=0x%08x "
							 "exp=0x%08x\n",
							 my_id, writer_id,
							 idx, i,
							 shared_data[idx]
								.data[i],
							 expected);
						thread_stats[my_id]
							.value_errors++;
						break;
					}
				}
			}

			thread_stats[my_id].total_reads += NUM_CACHE_LINES;

			/* Acknowledge: tell writer I verified this round */
			reader_ack[my_id].value = seq_data_ready;
			flush_data(&reader_ack[my_id],
				   sizeof(struct cache_flag));

			/*
			 * Wait for writer to signal round complete before
			 * proceeding to the next round. This prevents readers
			 * from racing ahead and seeing stale data.
			 */
			while (round_seq.value < seq_round_done) {
				invd_data(&round_seq,
					  sizeof(struct cache_flag));
				k_busy_wait(1);
			}
		}
	}

	/* Flush stats so main thread reads correct values */
	sys_cache_data_flush_range(&thread_stats[my_id],
				   sizeof(struct thread_error_stats));
}

/**
 * Helper: run the rotate-writer test with the currently configured
 * cache operation mode. Creates one thread per CPU, each participates
 * in every round as either writer or reader.
 */
static void run_smp_cache_test(const char *test_name)
{
	int num_cpus = NUM_TEST_THREADS;
	int i;
	uint64_t start_time;
	uint64_t end_time;
	uint32_t duration_ms;
	uint32_t total_checksum_errors = 0;
	uint32_t total_value_errors = 0;
	uint32_t total_reads = 0;
	uint32_t total_writes = 0;
	uint32_t expected_writes;
	uint32_t expected_reads;

	TC_PRINT("=== %s ===\n", test_name);
	TC_PRINT("CPUs: %d, Iterations per writer: %d, Cache lines: %d\n",
		 num_cpus, ITERATIONS_PER_WRITER, NUM_CACHE_LINES);
	TC_PRINT("Total rounds: %d (each CPU writes %d rounds)\n",
		 num_cpus * ITERATIONS_PER_WRITER, ITERATIONS_PER_WRITER);

	/* Initialize shared state */
	memset(shared_data, 0, sizeof(shared_data));
	round_seq.value = 0;
	for (i = 0; i < NUM_TEST_THREADS; i++) {
		reader_ack[i].value = 0;
	}

	sys_cache_data_flush_range(shared_data, sizeof(shared_data));
	sys_cache_data_flush_range(&round_seq, sizeof(round_seq));
	sys_cache_data_flush_range(reader_ack, sizeof(reader_ack));

	start_time = k_uptime_get();

	/* Create one thread per CPU — all run the same unified function */
	for (i = 0; i < num_cpus; i++) {
		k_thread_create(&test_threads[i], test_stacks[i], STACK_SIZE,
				smp_cache_thread,
				INT_TO_POINTER(i),
				INT_TO_POINTER(num_cpus),
				NULL,
				K_PRIO_PREEMPT(5), 0, K_FOREVER);
#if defined(CONFIG_SCHED_CPU_MASK)
		k_thread_cpu_pin(&test_threads[i], i);
#endif
		k_thread_start(&test_threads[i]);
	}

	/* Wait for all threads to complete */
	for (i = 0; i < num_cpus; i++) {
		k_thread_join(&test_threads[i], K_FOREVER);
	}

	end_time = k_uptime_get();
	duration_ms = (uint32_t)(end_time - start_time);

	/*
	 * Invalidate thread_stats before reading: threads ran on other CPUs
	 * and stats may be stale in our cache even after k_thread_join().
	 */
	sys_cache_data_invd_range(thread_stats, sizeof(thread_stats));

	for (i = 0; i < num_cpus; i++) {
		TC_PRINT("  CPU%d: writes=%u reads=%u "
			 "chk_err=%u val_err=%u\n",
			 i,
			 thread_stats[i].total_writes,
			 thread_stats[i].total_reads,
			 thread_stats[i].checksum_errors,
			 thread_stats[i].value_errors);
		total_checksum_errors += thread_stats[i].checksum_errors;
		total_value_errors += thread_stats[i].value_errors;
		total_reads += thread_stats[i].total_reads;
		total_writes += thread_stats[i].total_writes;
	}

	TC_PRINT("Duration: %u ms\n", duration_ms);
	TC_PRINT("Total writes: %u, Total reads: %u\n",
		 total_writes, total_reads);
	TC_PRINT("Checksum errors: %u, Value errors: %u\n",
		 total_checksum_errors, total_value_errors);

	/*
	 * Expected counts:
	 * - Each CPU writes ITERATIONS_PER_WRITER rounds × NUM_CACHE_LINES
	 * - Each CPU reads (num_cpus-1) × ITERATIONS_PER_WRITER rounds ×
	 *   NUM_CACHE_LINES (it reads in every round where it's not writer)
	 */
	expected_writes = (uint32_t)num_cpus *
			  ITERATIONS_PER_WRITER * NUM_CACHE_LINES;
	expected_reads = (uint32_t)num_cpus *
			 ITERATIONS_PER_WRITER *
			 (num_cpus - 1) * NUM_CACHE_LINES;

	zassert_equal(total_writes, expected_writes,
		      "Write count: got %u, expected %u",
		      total_writes, expected_writes);
	zassert_equal(total_reads, expected_reads,
		      "Read count: got %u, expected %u",
		      total_reads, expected_reads);
	zassert_equal(total_checksum_errors, 0,
		      "Checksum errors: %u", total_checksum_errors);
	zassert_equal(total_value_errors, 0,
		      "Value errors: %u", total_value_errors);

	TC_PRINT("All %d CPUs verified as both writer and reader\n", num_cpus);
	TC_PRINT("PASSED\n");
}

/**
 * @brief SMP cache coherency test using rotate-writer pattern.
 *
 * Each CPU takes a turn as writer while ALL other CPUs act as readers.
 * The writer flushes data to memory, readers invalidate and verify.
 * Every CPU exercises both the write-flush and invalidate-read paths.
 * Works for any CPU count >= 2.
 *
 * Runs two phases back-to-back:
 *   Phase 1: Separate flush (sys_cache_data_flush_range) +
 *            invalidate (sys_cache_data_invd_range)
 *   Phase 2: Combined flush-and-invalidate
 *            (sys_cache_data_flush_and_invd_range) — tests a distinct
 *            cache maintenance path (DC CIVAC on ARM64)
 */
ZTEST(smp_cache, test_smp_cache_coherency)
{
	TC_PRINT("--- Phase 1: Separate flush + invalidate ---\n");
	current_cache_op_mode = CACHE_OP_FLUSH_PLUS_INVD;
	run_smp_cache_test("SMP Cache Flush + Invalidate");

	TC_PRINT("\n--- Phase 2: Combined flush-and-invalidate ---\n");
	current_cache_op_mode = CACHE_OP_FLUSH_AND_INVD;
	run_smp_cache_test("SMP Cache Flush-and-Invalidate");
}

static void *smp_cache_setup(void)
{
	TC_PRINT("\n========================================\n");
	TC_PRINT("SMP Cache Management Test Suite\n");
	TC_PRINT("========================================\n");
	TC_PRINT("CPUs: %u\n", arch_num_cpus());
	TC_PRINT("Cache line size: %d bytes\n", CACHE_LINE_SIZE);
	TC_PRINT("========================================\n\n");

	return NULL;
}

static void smp_cache_teardown(void *fixture)
{
	ARG_UNUSED(fixture);

	TC_PRINT("========================================\n");
	TC_PRINT("Test completed\n");
	TC_PRINT("========================================\n");
}

ZTEST_SUITE(smp_cache, NULL, smp_cache_setup, NULL, NULL, smp_cache_teardown);
