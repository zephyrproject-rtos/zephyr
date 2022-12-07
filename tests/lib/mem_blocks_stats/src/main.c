/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <zephyr/sys/heap_listener.h>
#include <zephyr/sys/mem_blocks.h>
#include <zephyr/sys/util.h>

#define BLK_SZ     64
#define NUM_BLOCKS 8

SYS_MEM_BLOCKS_DEFINE(mem_block_01, BLK_SZ, NUM_BLOCKS, 4);

ZTEST(lib_mem_blocks_stats_test, test_mem_blocks_stats_invalid)
{
	struct sys_memory_stats  stats;
	int  status;

	/*
	 * Verify that sys_mem_blocks_runtime_stats_get() returns -EINVAL
	 * when an invalid parameter is passed.
	 */

	status = sys_mem_blocks_runtime_stats_get(NULL, &stats);
	zassert_equal(status, -EINVAL, "Routine returned %d instead of %d",
		      status, -EINVAL);

	status = sys_mem_blocks_runtime_stats_get(&mem_block_01, NULL);
	zassert_equal(status, -EINVAL, "Routine returned %d instead of %d",
		      status, -EINVAL);

	/*
	 * Verify that sys_mem_blocks_runtime_stats_reset_max() returns -EINVAL
	 * when an invalid parameter is passed.
	 */

	status = sys_mem_blocks_runtime_stats_reset_max(NULL);
	zassert_equal(status, -EINVAL, "Routine returned %d instead of %d",
		      status, -EINVAL);
}

ZTEST(lib_mem_blocks_stats_test, test_mem_blocks_runtime_stats)
{
	struct sys_memory_stats  stats;
	int   status;
	void *blocks[3];

	/* Verify initial stats */

	status = sys_mem_blocks_runtime_stats_get(&mem_block_01, &stats);
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

	status = sys_mem_blocks_alloc(&mem_block_01, 3, blocks);
	zassert_equal(status, 0, "Routine failed to allocate 3 blocks (%d)\n",
		      status);
	status = sys_mem_blocks_runtime_stats_get(&mem_block_01, &stats);
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

	status = sys_mem_blocks_free(&mem_block_01, 2, &blocks[1]);
	zassert_equal(status, 0, "Routine failed with status %d\n", status);

	status = sys_mem_blocks_runtime_stats_get(&mem_block_01, &stats);
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

	status = sys_mem_blocks_alloc(&mem_block_01, 1, &blocks[1]);
	zassert_equal(status, 0, "Routine failed with status %d\n", status);

	status = sys_mem_blocks_runtime_stats_get(&mem_block_01, &stats);
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

	status = sys_mem_blocks_runtime_stats_reset_max(&mem_block_01);
	zassert_equal(status, 0, "Routine failed with status %d\n", status);

	status = sys_mem_blocks_runtime_stats_get(&mem_block_01, &stats);
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

	status = sys_mem_blocks_free(&mem_block_01, 2, &blocks[0]);
	zassert_equal(status, 0, "Routine failed with status %d\n", status);

	status = sys_mem_blocks_runtime_stats_get(&mem_block_01, &stats);
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

ZTEST_SUITE(lib_mem_blocks_stats_test, NULL, NULL, NULL, NULL, NULL);
