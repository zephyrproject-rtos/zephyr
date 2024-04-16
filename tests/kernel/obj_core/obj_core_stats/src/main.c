/*
 * Copyright (c) 2023, Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/sys/mem_blocks.h>

SYS_MEM_BLOCKS_DEFINE(mem_block, 32, 4, 16);  /* Four 32 byte blocks */

K_MEM_SLAB_DEFINE(mem_slab, 32, 4, 16);       /* Four 32 byte blocks */

#if !defined(CONFIG_ARCH_POSIX) && !defined(CONFIG_SPARC) && !defined(CONFIG_MIPS)
static void test_thread_entry(void *, void *, void *);
K_THREAD_DEFINE(test_thread, 1024, test_thread_entry, NULL, NULL, NULL,
		K_HIGHEST_THREAD_PRIO, 0, 0);

K_SEM_DEFINE(wake_main_thread, 0, 1);
K_SEM_DEFINE(wake_test_thread, 0, 1);
#endif /* !CONFIG_ARCH_POSIX && !CONFIG_SPARC && !CONFIG_MIPS */

#if CONFIG_MP_MAX_NUM_CPUS > 1
K_THREAD_STACK_ARRAY_DEFINE(busy_thread_stack, CONFIG_MP_MAX_NUM_CPUS - 1, 512);

struct k_thread busy_thread[CONFIG_MP_MAX_NUM_CPUS - 1];

void busy_thread_entry(void *p1, void *p2, void *p3)
{
	while (1) {
		/* Busy loop to prevent CPU from entering idle */
	}
}

#endif

/***************** SYSTEM (CPUs and KERNEL) ******************/

/*
 * As the k_obj_core_stats_xxx() APIs are essentially wrappers to the
 * thread runtime stats APIs, limit this test to the same architectures as
 * that thread runtime stats test.
 */

#if !defined(CONFIG_ARCH_POSIX) && !defined(CONFIG_SPARC) && !defined(CONFIG_MIPS)
ZTEST(obj_core_stats_system, test_obj_core_stats_system)
{
	int  status;
	struct k_cycle_stats  kernel_raw[CONFIG_MP_MAX_NUM_CPUS];
	struct k_cycle_stats  cpu_raw;
	struct k_thread_runtime_stats kernel_query;
	struct k_thread_runtime_stats cpu_query;
	struct k_thread_runtime_stats sum_query;
	unsigned int i;

#if CONFIG_MP_MAX_NUM_CPUS > 1

	/* Create 1 busy thread for each core except the current */

	int  prio;

	prio = k_thread_priority_get(k_current_get());

	for (i = 0; i < CONFIG_MP_MAX_NUM_CPUS - 1; i++) {
		k_thread_create(&busy_thread[i], busy_thread_stack[i],
				K_THREAD_STACK_SIZEOF(busy_thread_stack[i]),
				busy_thread_entry, NULL, NULL, NULL,
				prio + 10, 0, K_NO_WAIT);
	}
#endif

	status = k_obj_core_stats_raw(K_OBJ_CORE(&_kernel), kernel_raw,
				      sizeof(kernel_raw));
	zassert_equal(status, 0, "Expected 0, got %d\n", status);

	/*
	 * Not much can be predicted for the raw stats aside from the
	 * the contents of the CPU sampling to be at least as large as
	 * kernel sampling. The same goes for the query stats.
	 */

	for (i = 0; i < CONFIG_MP_MAX_NUM_CPUS; i++) {
		status = k_obj_core_stats_raw(K_OBJ_CORE(&_kernel.cpus[i]),
					      &cpu_raw, sizeof(cpu_raw));
		zassert_equal(status, 0, "Expected 0, got %d on CPU %u\n",
			      status, i);

		zassert_true(cpu_raw.total >= kernel_raw[i].total);
#ifdef CONFIG_SCHED_THREAD_USAGE_ANALYSIS
		zassert_true(cpu_raw.current >= kernel_raw[i].current);
		zassert_true(cpu_raw.longest >= kernel_raw[i].longest);
		zassert_true(cpu_raw.num_windows >= kernel_raw[i].num_windows);
#endif
		zassert_true(cpu_raw.track_usage == kernel_raw[i].track_usage);
	}

	status = k_obj_core_stats_query(K_OBJ_CORE(&_kernel), &kernel_query,
					sizeof(kernel_query));
	zassert_equal(status, 0, "Expected 0, got %d\n", status);

	sum_query = (struct k_thread_runtime_stats){};

	for (i = 0; i < CONFIG_MP_MAX_NUM_CPUS; i++) {
		status = k_obj_core_stats_query(K_OBJ_CORE(&_kernel.cpus[i]),
						&cpu_query, sizeof(cpu_query));
		zassert_equal(status, 0, "Expected 0, got %d on CPU %u\n",
			      status, i);

#ifdef CONFIG_SCHED_THREAD_USAGE
		sum_query.execution_cycles += cpu_query.execution_cycles;
		sum_query.total_cycles += cpu_query.total_cycles;
#endif
#ifdef CONFIG_SCHED_THREAD_USAGE_ANALYSIS
		sum_query.current_cycles += cpu_query.current_cycles;
		sum_query.peak_cycles += cpu_query.peak_cycles;
		sum_query.average_cycles += cpu_query.average_cycles;
#endif
#ifdef CONFIG_SCHED_THREAD_USAGE_ALL
		sum_query.idle_cycles += cpu_query.idle_cycles;
#endif
	}

#ifdef CONFIG_SCHED_THREAD_USAGE
	zassert_true(sum_query.execution_cycles >= kernel_query.execution_cycles);
	zassert_true(sum_query.total_cycles >= kernel_query.total_cycles);
#endif
#ifdef CONFIG_SCHED_THREAD_USAGE_ANALYSIS
	zassert_true(sum_query.current_cycles >= kernel_query.current_cycles);
	zassert_true(sum_query.peak_cycles >= kernel_query.peak_cycles);
	zassert_true(sum_query.average_cycles >= kernel_query.average_cycles);
#endif
#ifdef CONFIG_SCHED_THREAD_USAGE_ALL
	zassert_true(sum_query.idle_cycles >= kernel_query.idle_cycles);
#endif
}
#endif  /* !CONFIG_ARCH_POSIX && !CONFIG_SPARC && !CONFIG_MIPS */

ZTEST(obj_core_stats_system, test_obj_core_stats_cpu_reset)
{
	int  status;

	for (unsigned int i = 0; i < CONFIG_MP_MAX_NUM_CPUS; i++) {
		status = k_obj_core_stats_reset(K_OBJ_CORE(&_kernel.cpus[i]));
		zassert_equal(status, -ENOTSUP,
			      "Expected %d, got %d on CPU%d\n",
			      -ENOTSUP, status, i);
	}
}

ZTEST(obj_core_stats_system, test_obj_core_stats_cpu_disable)
{
	int  status;

	for (unsigned int i = 0; i < CONFIG_MP_MAX_NUM_CPUS; i++) {
		status = k_obj_core_stats_disable(K_OBJ_CORE(&_kernel.cpus[i]));
		zassert_equal(status, -ENOTSUP,
			      "Expected %d, got %d on CPU%d\n",
			      -ENOTSUP, status, i);
	}
}

ZTEST(obj_core_stats_system, test_obj_core_stats_cpu_enable)
{
	int  status;

	for (unsigned int i = 0; i < CONFIG_MP_MAX_NUM_CPUS; i++) {
		status = k_obj_core_stats_enable(K_OBJ_CORE(&_kernel.cpus[i]));
		zassert_equal(status, -ENOTSUP,
			      "Expected %d, got %d on CPU%d\n",
			      -ENOTSUP, status, i);
	}
}

ZTEST(obj_core_stats_system, test_obj_core_stats_kernel_reset)
{
	int  status;

	status = k_obj_core_stats_reset(K_OBJ_CORE(&_kernel));
	zassert_equal(status, -ENOTSUP, "Expected %d, got %d\n",
		      -ENOTSUP, status);
}

ZTEST(obj_core_stats_system, test_obj_core_stats_kernel_disable)
{
	int  status;

	status = k_obj_core_stats_disable(K_OBJ_CORE(&_kernel));
	zassert_equal(status, -ENOTSUP, "Expected %d, got %d\n",
		      -ENOTSUP, status);
}

ZTEST(obj_core_stats_system, test_obj_core_stats_kernel_enable)
{
	int  status;

	status = k_obj_core_stats_enable(K_OBJ_CORE(&_kernel));
	zassert_equal(status, -ENOTSUP, "Expected %d, got %d\n",
		      -ENOTSUP, status);
}

/***************** THREADS ******************/

#if !defined(CONFIG_ARCH_POSIX) && !defined(CONFIG_SPARC) && !defined(CONFIG_MIPS)
/*
 * As the k_obj_core_stats_xxx() APIs are essentially wrappers to the
 * thread runtime stats APIs, limit this test to the same architectures as
 * that thread runtime stats test.
 */
void test_thread_entry(void *p1, void *p2, void *p3)
{
	while (1) {
		k_busy_wait(10000);

		k_sem_give(&wake_main_thread);
		k_sem_take(&wake_test_thread, K_FOREVER);
	}
}

ZTEST(obj_core_stats_thread, test_obj_core_stats_thread_test)
{
	struct k_cycle_stats raw1;
	struct k_cycle_stats raw2;
	struct k_thread_runtime_stats  query1;
	struct k_thread_runtime_stats  query2;
	struct k_thread_runtime_stats  query3;
	int    status;

	k_sem_take(&wake_main_thread, K_FOREVER);
	k_busy_wait(10000);

	/* test_thread should now be blocked on wake_test_thread */

	status = k_obj_core_stats_raw(K_OBJ_CORE(test_thread), &raw1,
				      sizeof(raw1));
	zassert_equal(status, 0, "Expected 0, got %d", status);

	status = k_obj_core_stats_query(K_OBJ_CORE(test_thread), &query1,
					sizeof(query1));
	zassert_equal(status, 0, "Expected 0, got %d", status);

	/*
	 * Busy wait for 10 msec. As test_thread should still be blocked,
	 * its stats data should not change.
	 */

	k_busy_wait(10000);

	status = k_obj_core_stats_raw(K_OBJ_CORE(test_thread), &raw2,
				      sizeof(raw2));
	zassert_equal(status, 0, "Expected 0, got %d", status);

	status = k_obj_core_stats_query(K_OBJ_CORE(test_thread), &query2,
					sizeof(query2));
	zassert_equal(status, 0, "Expected 0, got %d", status);

	zassert_mem_equal(&raw1, &raw2, sizeof(raw1),
			  "Thread raw stats changed while blocked\n");
	zassert_mem_equal(&query1, &query2, sizeof(query1),
			  "Thread query stats changed while blocked\n");

	/*
	 * Let test_thread execute for a short bit and then re-sample the
	 * stats. As the k_obj_core_stats_query() backend is identical to
	 * that of k_thread_runtime_stats_get(), their queries should be
	 * identical (and different from the previous sample).
	 */

	k_sem_give(&wake_test_thread);
	k_sem_take(&wake_main_thread, K_FOREVER);
	k_busy_wait(10000);

	/* test_thread should now be blocked. */

	status = k_obj_core_stats_query(K_OBJ_CORE(test_thread), &query2,
					sizeof(query3));
	zassert_equal(status, 0, "Expected 0, got %d\n", status);

	status = k_thread_runtime_stats_get(test_thread, &query3);
	zassert_equal(status, 0, "Expected 0, got %d\n", status);
	zassert_mem_equal(&query2, &query3, sizeof(query2),
			  "Queries not equal!\n");

#ifdef CONFIG_SCHED_THREAD_USAGE
	zassert_true(query2.execution_cycles > query1.execution_cycles,
		     "Execution cycles did not increase\n");
	zassert_true(query2.total_cycles > query1.total_cycles,
		     "Total cycles did not increase\n");
#endif

#ifdef CONFIG_SCHED_THREAD_USAGE_ANALYSIS

	/*
	 * [current_cycles], [peak_cycles] and [average_cycles] can not be
	 * predicted by this test.
	 */

#endif

#ifdef CONFIG_SCHED_THREAD_USAGE_ALL
	zassert_equal(query2.idle_cycles, 0,
		      "Expected 0, got %llu\n", query2.idle_cycles);
#endif

	/* Reset the stats */

	status = k_obj_core_stats_reset(K_OBJ_CORE(test_thread));
	zassert_equal(status, 0, "Expected 0, got %d\n", status);

	status = k_obj_core_stats_query(K_OBJ_CORE(test_thread),
					&query3, sizeof(query3));
	zassert_equal(status, 0, "Expected 0, got %d\n", status);

#ifdef CONFIG_SCHED_THREAD_USAGE
	zassert_equal(query3.execution_cycles, 0,
		      "Expected 0, got %llu\n", query3.execution_cycles);
	zassert_equal(query3.total_cycles, 0,
		      "Expected 0, got %llu\n", query3.total_cycles);
#endif

#ifdef CONFIG_SCHED_THREAD_USAGE_ANALYSIS
	zassert_equal(query3.current_cycles, 0,
		      "Expected 0, got %llu\n", query3.current_cycles);
	zassert_equal(query3.peak_cycles, 0,
		      "Expected 0, got %llu\n", query3.peak_cycles);
	zassert_equal(query3.average_cycles, 0,
		      "Expected 0, got %llu\n", query3.average_cycles);
#endif

#ifdef CONFIG_SCHED_THREAD_USAGE_ALL
	zassert_equal(query3.idle_cycles, 0,
		      "Expected 0, got %llu\n", query3.idle_cycles);
#endif

	/* Disable the stats (re-using query2 and query3) */

	status = k_obj_core_stats_disable(K_OBJ_CORE(test_thread));
	zassert_equal(status, 0, "Expected 0, got %llu\n", status);

	k_sem_give(&wake_test_thread);
	k_sem_take(&wake_main_thread, K_FOREVER);
	k_busy_wait(10000);

	k_obj_core_stats_query(K_OBJ_CORE(test_thread),
			       &query2, sizeof(query2));

	zassert_mem_equal(&query2, &query3, sizeof(query2),
			  "Stats changed while disabled!\n");

	/* Enable the stats */

	status = k_obj_core_stats_enable(K_OBJ_CORE(test_thread));
	zassert_equal(status, 0, "Expected 0, got %llu\n", status);

	k_sem_give(&wake_test_thread);
	k_sem_take(&wake_main_thread, K_FOREVER);
	k_busy_wait(10000);

	k_obj_core_stats_query(K_OBJ_CORE(test_thread),
			       &query3, sizeof(query3));

	/* We can not predict the stats, but they should be non-zero. */

#ifdef CONFIG_SCHED_THREAD_USAGE
	zassert_true(query3.execution_cycles > 0);
	zassert_true(query3.total_cycles > 0);
#endif
#ifdef CONFIG_SCHED_THREAD_USAGE
	zassert_true(query3.current_cycles > 0);
	zassert_true(query3.peak_cycles > 0);
	zassert_true(query3.average_cycles > 0);
#endif
#ifdef CONFIG_SCHED_THREAD_USAGE_ALL
	zassert_true(query3.idle_cycles == 0);
#endif

	k_thread_abort(test_thread);
}
#endif /* !CONFIG_ARCH_POSIX && !CONFIG_SPARC && !CONFIG_MIPS */

/***************** SYSTEM MEMORY BLOCKS *********************/

ZTEST(obj_core_stats_mem_block, test_sys_mem_block_enable)
{
	int  status;

	status = k_obj_core_stats_enable(K_OBJ_CORE(&mem_block));
	zassert_equal(status, -ENOTSUP,
		      "Not supposed to be supported. Got %d, not %d\n",
		      status, -ENOTSUP);
}

ZTEST(obj_core_stats_mem_block, test_sys_mem_block_disable)
{
	int  status;

	status = k_obj_core_stats_disable(K_OBJ_CORE(&mem_block));
	zassert_equal(status, -ENOTSUP,
		      "Not supposed to be supported. Got %d, not %d\n",
		      status, -ENOTSUP);
}

static void test_mem_block_raw(const char *str,
			       struct sys_mem_blocks_info *expected)
{
	int  status;
	struct sys_mem_blocks_info  raw;

	status = k_obj_core_stats_raw(K_OBJ_CORE(&mem_block), &raw,
				      sizeof(raw));
	zassert_equal(status, 0,
		      "%s: Failed to get raw stats (%d)\n", str, status);

	zassert_equal(raw.num_blocks, expected->num_blocks,
		      "%s: Expected %u blocks, got %u\n",
		      str, expected->num_blocks, raw.num_blocks);
	zassert_equal(raw.blk_sz_shift, expected->blk_sz_shift,
		      "%s: Expected blk_sz_shift=%u, got %u\n",
		      str, expected->blk_sz_shift, raw.blk_sz_shift);
#ifdef CONFIG_SYS_MEM_BLOCKS_RUNTIME_STATS
	zassert_equal(raw.used_blocks, expected->used_blocks,
		      "%s: Expected %u used, got %d\n",
		      str, expected->used_blocks, raw.used_blocks);
	zassert_equal(raw.max_used_blocks, expected->max_used_blocks,
		      "%s: Expected max %u used, got %d\n",
		      str, expected->max_used_blocks, raw.max_used_blocks);
#endif
}

static void test_mem_block_query(const char *str,
				 struct sys_memory_stats *expected)
{
	struct sys_memory_stats query;
	int  status;

	status = k_obj_core_stats_query(K_OBJ_CORE(&mem_block), &query,
					sizeof(query));
	zassert_equal(status, 0,
		      "%s: Failed to get query stats (%d)\n", str, status);

	zassert_equal(query.free_bytes, expected->free_bytes,
		      "%s: Expected %u free bytes, got %u\n",
		      str, expected->free_bytes, query.free_bytes);
#ifdef CONFIG_SYS_MEM_BLOCKS_RUNTIME_STATS
	zassert_equal(query.allocated_bytes, expected->allocated_bytes,
		      "%s: Expected %u allocated bytes, got %u\n",
		      str, expected->allocated_bytes, query.allocated_bytes);
	zassert_equal(query.max_allocated_bytes, expected->max_allocated_bytes,
		      "%s: Expected %u max_allocated bytes, got %d\n",
		      str, expected->max_allocated_bytes,
		      query.max_allocated_bytes);
#endif
}

ZTEST(obj_core_stats_mem_block, test_obj_core_stats_mem_block)
{
	struct sys_mem_blocks_info  raw =  {
		.num_blocks = 4, .blk_sz_shift = 5,
#ifdef CONFIG_SYS_MEM_BLOCKS_RUNTIME_STATS
		.used_blocks = 0, .max_used_blocks = 0
#endif
	};
	struct sys_memory_stats query = {
		.free_bytes = 128,
		.allocated_bytes = 0,
		.max_allocated_bytes = 0
	};
	void *mem1;
	void *mem2;
	int  status;

	/*
	 * As the ordering of the "raw", "query" and "reset" tests matter,
	 * they have been grouped together here. As they are for the most
	 * wrappers for the runtime stats routines, minimal testing is
	 * being done.
	 */

	/* Initial checks */

	test_mem_block_raw("Initial", &raw);
	test_mem_block_query("Initial", &query);

	/* Allocate 1st block */

	status = sys_mem_blocks_alloc(&mem_block, 1, &mem1);
	zassert_equal(status, 0, "Expected 0, got %d\n", status);

	query.free_bytes -= 32;
	query.allocated_bytes += 32;
#ifdef CONFIG_SYS_MEM_BLOCKS_RUNTIME_STATS
	raw.used_blocks++;
	raw.max_used_blocks++;
	query.max_allocated_bytes += 32;
#endif
	test_mem_block_raw("1st Alloc", &raw);
	test_mem_block_query("1st Alloc", &query);

	/* Allocate 2nd block */

	status = sys_mem_blocks_alloc(&mem_block, 1, &mem2);
	zassert_equal(status, 0, "Expected 0, got %d\n", status);

	query.free_bytes -= 32;
	query.allocated_bytes += 32;
#ifdef CONFIG_SYS_MEM_BLOCKS_RUNTIME_STATS
	raw.used_blocks++;
	raw.max_used_blocks++;
	query.max_allocated_bytes += 32;
#endif
	test_mem_block_raw("2nd Alloc", &raw);
	test_mem_block_query("2nd Alloc", &query);

	/* Free 1st block */

	sys_mem_blocks_free(&mem_block, 1, &mem1);

	query.free_bytes += 32;
	query.allocated_bytes -= 32;
#ifdef CONFIG_SYS_MEM_BLOCKS_RUNTIME_STATS
	raw.used_blocks--;
#endif
	test_mem_block_raw("Free 1st", &raw);
	test_mem_block_query("Free 1st", &query);

	/* Reset the mem block stats */

	status = k_obj_core_stats_reset(K_OBJ_CORE(&mem_block));
	zassert_equal(status, 0, "Expected 0, got %d\n", status);
#ifdef CONFIG_SYS_MEM_BLOCKS_RUNTIME_STATS
	raw.max_used_blocks = raw.used_blocks;
	query.max_allocated_bytes = query.allocated_bytes;
#endif
	test_mem_block_raw("Reset", &raw);
	test_mem_block_query("Reset", &query);

	/* Cleanup - Free 2nd block */
	sys_mem_blocks_free(&mem_block, 1, &mem2);
}

/***************** MEMORY SLABS *********************/

ZTEST(obj_core_stats_mem_slab,  test_mem_slab_enable)
{
	int  status;

	status = k_obj_core_stats_disable(K_OBJ_CORE(&mem_slab));
	zassert_equal(status, -ENOTSUP,
		      "Not supposed to be supported. Got %d, not %d\n",
		      status, -ENOTSUP);
}

ZTEST(obj_core_stats_mem_slab,  test_mem_slab_disable)
{
	int  status;

	status = k_obj_core_stats_disable(K_OBJ_CORE(&mem_slab));
	zassert_equal(status, -ENOTSUP,
		      "Not supposed to be supported. Got %d, not %d\n",
		      status, -ENOTSUP);
}

static void test_mem_slab_raw(const char *str, struct k_mem_slab_info *expected)
{
	int  status;
	struct k_mem_slab_info  raw;

	status = k_obj_core_stats_raw(K_OBJ_CORE(&mem_slab), &raw,
				      sizeof(raw));
	zassert_equal(status, 0,
		      "%s: Failed to get raw stats (%d)\n", str, status);

	zassert_equal(raw.num_blocks, expected->num_blocks,
		      "%s: Expected %u blocks, got %u\n",
		      str, expected->num_blocks, raw.num_blocks);
	zassert_equal(raw.block_size, expected->block_size,
		      "%s: Expected block size=%u blocks, got %u\n",
		      str, expected->block_size, raw.block_size);
	zassert_equal(raw.num_used, expected->num_used,
		      "%s: Expected %u used, got %d\n",
		      str, expected->num_used, raw.num_used);
#ifdef CONFIG_MEM_SLAB_TRACE_MAX_UTILIZATION
	zassert_equal(raw.max_used, expected->max_used,
		      "%s: Expected max %u used, got %d\n",
		      str, expected->max_used, raw.max_used);
#endif
}

static void test_mem_slab_query(const char *str,
				struct sys_memory_stats *expected)
{
	struct sys_memory_stats query;
	int  status;

	status = k_obj_core_stats_query(K_OBJ_CORE(&mem_slab), &query,
					sizeof(query));
	zassert_equal(status, 0,
		      "%s: Failed to get query stats (%d)\n", str, status);

	zassert_equal(query.free_bytes, expected->free_bytes,
		      "%s: Expected %u free bytes, got %u\n",
		      str, expected->free_bytes, query.free_bytes);
	zassert_equal(query.allocated_bytes, expected->allocated_bytes,
		      "%s: Expected %u allocated bytes, got %u\n",
		      str, expected->allocated_bytes, query.allocated_bytes);
	zassert_equal(query.max_allocated_bytes, expected->max_allocated_bytes,
		      "%s: Expected %u max_allocated bytes, got %d\n",
		      str, expected->max_allocated_bytes,
		      query.max_allocated_bytes);
}

ZTEST(obj_core_stats_mem_slab, test_obj_core_stats_mem_slab)
{
	struct k_mem_slab_info  raw =  {
		.num_blocks = 4, .block_size = 32, .num_used = 0,
#ifdef CONFIG_MEM_SLAB_TRACE_MAX_UTILIZATION
		.max_used = 0
#endif
	};
	struct sys_memory_stats query = {
		.free_bytes = 128,
		.allocated_bytes = 0,
		.max_allocated_bytes = 0
	};
	void *mem1;
	void *mem2;
	int  status;

	/*
	 * As the ordering of the "raw", "query" and "reset" tests matter,
	 * they have been grouped together here. As they are for the most
	 * wrappers for the runtime stats routines, minimal testing is
	 * being done.
	 */


	/* Initial checks */

	test_mem_slab_raw("Initial", &raw);
	test_mem_slab_query("Initial", &query);

	/* Allocate 1st block */

	status = k_mem_slab_alloc(&mem_slab, &mem1, K_FOREVER);
	zassert_equal(status, 0, "Expected 0, got %d\n", status);

	raw.num_used++;
	query.free_bytes -= 32;
	query.allocated_bytes += 32;
#ifdef CONFIG_MEM_SLAB_TRACE_MAX_UTILIZATION
	raw.max_used++;
	query.max_allocated_bytes += 32;
#endif
	test_mem_slab_raw("1st Alloc", &raw);
	test_mem_slab_query("1st Alloc", &query);

	/* Allocate 2nd block */

	status = k_mem_slab_alloc(&mem_slab, &mem2, K_FOREVER);
	zassert_equal(status, 0, "Expected 0, got %d\n", status);

	raw.num_used++;
	query.free_bytes -= 32;
	query.allocated_bytes += 32;
#ifdef CONFIG_MEM_SLAB_TRACE_MAX_UTILIZATION
	raw.max_used++;
	query.max_allocated_bytes += 32;
#endif
	test_mem_slab_raw("2nd Alloc", &raw);
	test_mem_slab_query("2nd Alloc", &query);

	/* Free 1st block */
	k_mem_slab_free(&mem_slab, mem1);

	raw.num_used--;
	query.free_bytes += 32;
	query.allocated_bytes -= 32;
	test_mem_slab_raw("Free 1st", &raw);
	test_mem_slab_query("Free 1st", &query);

	/* Reset the mem slab stats */
	status = k_obj_core_stats_reset(K_OBJ_CORE(&mem_slab));
	zassert_equal(status, 0, "Expected 0, got %d\n", status);
#ifdef CONFIG_MEM_SLAB_TRACE_MAX_UTILIZATION
	raw.max_used = raw.num_used;
	query.max_allocated_bytes = query.allocated_bytes;
#endif
	test_mem_slab_raw("Reset", &raw);
	test_mem_slab_query("Reset", &query);

	/* Cleanup - Free 2nd block */
	k_mem_slab_free(&mem_slab, mem2);
}

ZTEST_SUITE(obj_core_stats_system, NULL, NULL,
	    ztest_simple_1cpu_before, ztest_simple_1cpu_after, NULL);

ZTEST_SUITE(obj_core_stats_thread, NULL, NULL,
	    ztest_simple_1cpu_before, ztest_simple_1cpu_after, NULL);

ZTEST_SUITE(obj_core_stats_mem_block, NULL, NULL,
	    ztest_simple_1cpu_before, ztest_simple_1cpu_after, NULL);

ZTEST_SUITE(obj_core_stats_mem_slab, NULL, NULL,
	    ztest_simple_1cpu_before, ztest_simple_1cpu_after, NULL);
