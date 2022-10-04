/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <ztest.h>

#define BLK_SZ     64
#define NUM_BLOCKS 8

K_MEM_SLAB_DEFINE(kmslab, BLK_SZ, NUM_BLOCKS, 4);

ZTEST(lib_mem_slab_stats_test, test_mem_slab_stats_invalid_params)
{
	struct sys_memory_stats  stats;
	int  status;

	/*
	 * Verify that k_mem_slab_runtime_stats_get() returns -EINVAL
	 * when an invalid parameter is passed.
	 */

	status = k_mem_slab_runtime_stats_get(NULL, &stats);
	zassert_equal(status, -EINVAL, "Routine returned %d instead of %d",
		      status, -EINVAL);

	status = k_mem_slab_runtime_stats_get(&kmslab, NULL);
	zassert_equal(status, -EINVAL, "Routine returned %d instead of %d",
		      status, -EINVAL);

	/*
	 * Verify that k_mem_slab_runtime_stats_reset_max() returns -EINVAL
	 * when an invalid parameter is passed.
	 */

	status = k_mem_slab_runtime_stats_reset_max(NULL);
	zassert_equal(status, -EINVAL, "Routine returned %d instead of %d",
		      status, -EINVAL);
}

ZTEST(lib_mem_slab_stats_test, test_mem_slab_runtime_stats)
{
	struct sys_memory_stats  stats;
	int   status;
	void *memory[3];

	/* Verify initial stats */

	status = k_mem_slab_runtime_stats_get(&kmslab, &stats);
	zassert_equal(status, 0, "Routine failed with status %d\n", status);

	zassert_equal(stats.free_bytes, BLK_SZ * NUM_BLOCKS,
		      "Expected %zu free bytes, not %zu\n",
		      BLK_SZ * NUM_BLOCKS, stats.free_bytes);
	zassert_equal(stats.allocated_bytes, 0,
		      "Expected 0 allocated bytes, not %zu\n",
		      stats.allocated_bytes);
	zassert_equal(stats.max_allocated_bytes, 0,
		      "Expected 0 max allocated bytes, not %zu\n",
		      stats.max_allocated_bytes);

	/* Allocate three blocks, and then verify the stats. */

	status = k_mem_slab_alloc(&kmslab, &memory[0], K_NO_WAIT);
	zassert_equal(status, 0, "Routine failed to allocate 1st block (%d)\n",
		      status);
	status = k_mem_slab_alloc(&kmslab, &memory[1], K_NO_WAIT);
	zassert_equal(status, 0, "Routine failed to allocate 2nd block (%d)\n",
		      status);
	status = k_mem_slab_alloc(&kmslab, &memory[2], K_NO_WAIT);
	zassert_equal(status, 0, "Routine failed to allocate 3rd block (%d)\n",
		      status);
	status = k_mem_slab_runtime_stats_get(&kmslab, &stats);
	zassert_equal(status, 0, "Routine failed with status %d\n", status);

	zassert_equal(stats.free_bytes, BLK_SZ * (NUM_BLOCKS - 3),
		      "Expected %zu free bytes, not %zu\n",
		      BLK_SZ * (NUM_BLOCKS - 3), stats.free_bytes);
	zassert_equal(stats.allocated_bytes, 3 * BLK_SZ,
		      "Expected %zu allocated bytes, not %zu\n",
		      3 * BLK_SZ, stats.allocated_bytes);
	zassert_equal(stats.max_allocated_bytes, 3 * BLK_SZ,
		      "Expected %zu max allocated bytes, not %zu\n",
		      3 * BLK_SZ, stats.max_allocated_bytes);

	/* Free blocks 1 and 2, and then verify the stats. */

	k_mem_slab_free(&kmslab, &memory[2]);
	k_mem_slab_free(&kmslab, &memory[1]);

	status = k_mem_slab_runtime_stats_get(&kmslab, &stats);
	zassert_equal(status, 0, "Routine failed with status %d\n", status);

	zassert_equal(stats.free_bytes, BLK_SZ * (NUM_BLOCKS - 1),
		      "Expected %zu free bytes, not %zu\n",
		      BLK_SZ * (NUM_BLOCKS - 1), stats.free_bytes);
	zassert_equal(stats.allocated_bytes, 1 * BLK_SZ,
		      "Expected %zu allocated bytes, not %zu\n",
		      1 * BLK_SZ, stats.allocated_bytes);
	zassert_equal(stats.max_allocated_bytes, 3 * BLK_SZ,
		      "Expected %zu max allocated bytes, not %zu\n",
		      3 * BLK_SZ, stats.max_allocated_bytes);

	/* Allocate 1 block and verify the max is still at 3 */

	status = k_mem_slab_alloc(&kmslab, &memory[1], K_NO_WAIT);
	zassert_equal(status, 0, "Routine failed with status %d\n", status);

	status = k_mem_slab_runtime_stats_get(&kmslab, &stats);
	zassert_equal(status, 0, "Routine failed with status %d\n", status);

	zassert_equal(stats.free_bytes, BLK_SZ * (NUM_BLOCKS - 2),
		      "Expected %zu free bytes, not %zu\n",
		      BLK_SZ * (NUM_BLOCKS - 2), stats.free_bytes);
	zassert_equal(stats.allocated_bytes, 2 * BLK_SZ,
		      "Expected %zu allocated bytes, not %zu\n",
		      2 * BLK_SZ, stats.allocated_bytes);
	zassert_equal(stats.max_allocated_bytes, 3 * BLK_SZ,
		      "Expected %zu max allocated bytes, not %zu\n",
		      3 * BLK_SZ, stats.max_allocated_bytes);


	/* Reset the max allocated blocks; verify max is 2 blocks */

	status = k_mem_slab_runtime_stats_reset_max(&kmslab);
	zassert_equal(status, 0, "Routine failed with status %d\n", status);

	status = k_mem_slab_runtime_stats_get(&kmslab, &stats);
	zassert_equal(status, 0, "Routine failed with status %d\n", status);

	zassert_equal(stats.free_bytes, BLK_SZ * (NUM_BLOCKS - 2),
		      "Expected %zu free bytes, not %zu\n",
		      BLK_SZ * (NUM_BLOCKS - 2), stats.free_bytes);
	zassert_equal(stats.allocated_bytes, 2 * BLK_SZ,
		      "Expected %zu allocated bytes, not %zu\n",
		      2 * BLK_SZ, stats.allocated_bytes);
	zassert_equal(stats.max_allocated_bytes, 2 * BLK_SZ,
		      "Expected %zu max allocated bytes, not %zu\n",
		      2 * BLK_SZ, stats.max_allocated_bytes);

	/* Free the last two blocks; verify stats results */

	k_mem_slab_free(&kmslab, &memory[0]);
	k_mem_slab_free(&kmslab, &memory[1]);

	status = k_mem_slab_runtime_stats_get(&kmslab, &stats);
	zassert_equal(status, 0, "Routine failed with status %d\n", status);

	zassert_equal(stats.free_bytes, BLK_SZ * NUM_BLOCKS,
		      "Expected %zu free bytes, not %zu\n",
		      BLK_SZ * NUM_BLOCKS, stats.free_bytes);
	zassert_equal(stats.allocated_bytes, 0,
		      "Expected %zu allocated bytes, not %zu\n",
		      0, stats.allocated_bytes);
	zassert_equal(stats.max_allocated_bytes, 2 * BLK_SZ,
		      "Expected %zu max allocated bytes, not %zu\n",
		      2 * BLK_SZ, stats.max_allocated_bytes);
}

ZTEST_SUITE(lib_mem_slab_stats_test, NULL, NULL, NULL, NULL, NULL);
