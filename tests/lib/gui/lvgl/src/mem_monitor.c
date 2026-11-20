/*
 * Copyright (c) 2026 André Costa
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/sys/util.h>
#include <lvgl_zephyr.h>
#include <lvgl_mem.h>
#include <lvgl.h>

#if defined(CONFIG_LV_Z_MEM_POOL_SYS_HEAP) && defined(CONFIG_SYS_HEAP_RUNTIME_STATS)

#define FRAG_BLOCK_SIZE 256
#define MAX_BLOCKS      1024

void *setup_lvgl(void);
void teardown_lvgl(void *data);

static void *blocks[MAX_BLOCKS];

static lv_mem_monitor_t monitor(void)
{
	lv_mem_monitor_t mon;

	lv_mem_monitor(&mon);
	return mon;
}

/* Fills the heap with FRAG_BLOCK_SIZE blocks and returns how many were allocated */
static size_t fill_heap(void)
{
	size_t count = 0;

	while (count < MAX_BLOCKS) {
		blocks[count] = lvgl_malloc(FRAG_BLOCK_SIZE);
		if (blocks[count] == NULL) {
			break;
		}
		count++;
	}
	zassert_true(count < MAX_BLOCKS, "heap larger than the test expects");
	zassert_true(count >= 8, "heap too small for the test");
	return count;
}

static void release_blocks(void *data)
{
	ARG_UNUSED(data);

	for (size_t i = 0; i < ARRAY_SIZE(blocks); i++) {
		lvgl_free(blocks[i]);
		blocks[i] = NULL;
	}
}

static void assert_percentages(const lv_mem_monitor_t *mon)
{
	zassert_equal(mon->used_pct,
		      (uint64_t)(mon->total_size - mon->free_size) * 100U / mon->total_size);
	/* Same formula as LVGL's builtin allocator, so that the value compares across ports */
	if (mon->free_size > 0) {
		zassert_equal(mon->frag_pct,
			      100 - (uint64_t)mon->free_biggest_size * 100U / mon->free_size);
	} else {
		zassert_equal(mon->frag_pct, 0);
	}
}

ZTEST(lvgl_mem_monitor, test_sizes_match_the_heap)
{
	lv_mem_monitor_t mon = monitor();
	struct sys_memory_stats stats;

	lvgl_heap_stats(&stats);

	zassert_equal(mon.total_size, stats.free_bytes + stats.allocated_bytes);
	zassert_equal(mon.free_size, stats.free_bytes);
	zassert_true(mon.free_biggest_size > 0);
	zassert_true(mon.free_biggest_size <= mon.free_size);
	assert_percentages(&mon);
}

ZTEST(lvgl_mem_monitor, test_biggest_free_block_in_a_fragmented_heap)
{
	size_t count = fill_heap();

	/* Free every other block, so the free space is split into holes of about one block */
	for (size_t i = 0; i < count; i += 2) {
		lvgl_free(blocks[i]);
		blocks[i] = NULL;
	}

	lv_mem_monitor_t mon = monitor();

	zassert_true(mon.free_biggest_size >= FRAG_BLOCK_SIZE);
	zassert_true(mon.free_biggest_size < mon.free_size);
	zassert_true(mon.frag_pct > 0);
	assert_percentages(&mon);

	blocks[0] = lvgl_malloc(mon.free_biggest_size + 1);
	zassert_is_null(blocks[0], "a block larger than the biggest hole was allocated");

	blocks[0] = lvgl_malloc(mon.free_biggest_size);
	zassert_not_null(blocks[0], "the biggest hole can't be allocated");
}

ZTEST(lvgl_mem_monitor, test_full_heap)
{
	size_t count = fill_heap();
	lv_mem_monitor_t mon = monitor();

	/* Take what is left after the last FRAG_BLOCK_SIZE block, until nothing fits */
	while (mon.free_biggest_size > 0) {
		zassert_true(count < MAX_BLOCKS);
		blocks[count] = lvgl_malloc(mon.free_biggest_size);
		zassert_not_null(blocks[count], "the biggest free block can't be allocated");
		count++;
		mon = monitor();
	}

	zassert_equal(mon.free_biggest_size, 0);
	assert_percentages(&mon);
}

ZTEST_SUITE(lvgl_mem_monitor, NULL, setup_lvgl, NULL, release_blocks, teardown_lvgl);

#endif /* CONFIG_LV_Z_MEM_POOL_SYS_HEAP && CONFIG_SYS_HEAP_RUNTIME_STATS */
