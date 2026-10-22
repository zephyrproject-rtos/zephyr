/*
 * Copyright (c) 2021 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/linker/linker-defs.h>
#if defined(CONFIG_ARM64)
#include <zephyr/dt-bindings/memory-attr/memory-attr-arm64.h>
#else
#include <zephyr/dt-bindings/memory-attr/memory-attr-arm.h>
#endif

#include <zephyr/multi_heap/shared_multi_heap.h>

#define DT_DRV_COMPAT		zephyr_memory_region

#define SMH_DT_MEM_UNKNOWN_DEFAULT DT_MEM_ARCH_ATTR_UNKNOWN

#define RES0_CACHE_ADDR		DT_REG_ADDR(DT_NODELABEL(res0))
#define RES1_NOCACHE_ADDR	DT_REG_ADDR(DT_NODELABEL(res1))
#define RES2_CACHE_ADDR		DT_REG_ADDR(DT_NODELABEL(res2))

struct region_map {
	struct shared_multi_heap_region region;
	uintptr_t p_addr;
};

#define FOREACH_REG(n)								\
	{									\
		.region = {							\
			.addr = (uintptr_t) DT_INST_REG_ADDR(n),		\
			.size = DT_INST_REG_SIZE(n),				\
			.attr = DT_INST_PROP_OR(n, zephyr_memory_attr,		\
						SMH_DT_MEM_UNKNOWN_DEFAULT),	\
		},								\
	},

struct region_map map[] = {
	DT_INST_FOREACH_STATUS_OKAY(FOREACH_REG)
};

/*
 * Given a virtual address retrieve the original memory region that the mapping
 * is belonging to.
 */
static struct region_map *get_region_map(void *v_addr)
{
	for (size_t reg = 0; reg < ARRAY_SIZE(map); reg++) {
		if ((uintptr_t) v_addr >= map[reg].region.addr &&
		    (uintptr_t) v_addr < map[reg].region.addr + map[reg].region.size) {
			return &map[reg];
		}
	}
	return NULL;
}

static inline enum shared_multi_heap_attr dt_to_reg_attr(uint32_t dt_attr)
{
#if defined(CONFIG_ARM64)
	if (DT_MEM_ATTR_GET(dt_attr) & DT_MEM_CACHEABLE) {
		return SMH_REG_ATTR_CACHEABLE;
	}
	return SMH_REG_ATTR_NON_CACHEABLE;
#else
	switch (DT_MEM_ARM_GET(dt_attr)) {
	case DT_MEM_ARM_MPU_RAM:
		return SMH_REG_ATTR_CACHEABLE;
	case DT_MEM_ARM_MPU_RAM_NOCACHE:
		return SMH_REG_ATTR_NON_CACHEABLE;
	default:
		ztest_test_fail();
	}

	return 0;
#endif
}

static void fill_multi_heap(void)
{
	struct region_map *reg_map;

	for (size_t idx = 0; idx < DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT); idx++) {
		reg_map = &map[idx];

		/* zephyr,memory-attr property not found. Skip it. */
		if (reg_map->region.attr == SMH_DT_MEM_UNKNOWN_DEFAULT) {
			continue;
		}

		/* Convert MPU attributes to shared-multi-heap capabilities */
		reg_map->region.attr = dt_to_reg_attr(reg_map->region.attr);

		/* Assume for now that phys == virt */
		reg_map->p_addr = reg_map->region.addr;

		shared_multi_heap_add(&reg_map->region, NULL);
	}
}

static void *shared_multi_heap_setup(void)
{
	int ret;

	ret = shared_multi_heap_pool_init();
	zassert_equal(0, ret, "failed initialization");

	/*
	 * Fill the buffer pool with the memory heaps coming from DT
	 */
	fill_multi_heap();

	return NULL;
}

ZTEST(shared_multi_heap, test_shared_multi_heap)
{
	struct region_map *reg_map;
	void *block;
	int ret;

	/*
	 * Return -EALREADY if already inited
	 */
	ret = shared_multi_heap_pool_init();
	zassert_equal(-EALREADY, ret, "second init should fail");

	/*
	 * Request a small cacheable chunk. It should be allocated in the
	 * smaller region RES0
	 */
	block = shared_multi_heap_alloc(SMH_REG_ATTR_CACHEABLE, 0x40);
	reg_map = get_region_map(block);

	zassert_equal(reg_map->p_addr, RES0_CACHE_ADDR, "block in the wrong memory region");
	zassert_equal(reg_map->region.attr, SMH_REG_ATTR_CACHEABLE, "wrong memory attribute");

	/*
	 * Request another small cacheable chunk. It should be allocated in the
	 * smaller cacheable region RES0
	 */
	block = shared_multi_heap_alloc(SMH_REG_ATTR_CACHEABLE, 0x80);
	reg_map = get_region_map(block);

	zassert_equal(reg_map->p_addr, RES0_CACHE_ADDR, "block in the wrong memory region");
	zassert_equal(reg_map->region.attr, SMH_REG_ATTR_CACHEABLE, "wrong memory attribute");

	/*
	 * Request a big cacheable chunk. It should be allocated in the
	 * bigger cacheable region RES2
	 */
	block = shared_multi_heap_alloc(SMH_REG_ATTR_CACHEABLE, 0x1200);
	reg_map = get_region_map(block);

	zassert_equal(reg_map->p_addr, RES2_CACHE_ADDR, "block in the wrong memory region");
	zassert_equal(reg_map->region.attr, SMH_REG_ATTR_CACHEABLE, "wrong memory attribute");

	/*
	 * Request a non-cacheable chunk. It should be allocated in the
	 * non-cacheable region RES1
	 */
	block = shared_multi_heap_alloc(SMH_REG_ATTR_NON_CACHEABLE, 0x100);
	reg_map = get_region_map(block);

	zassert_equal(reg_map->p_addr, RES1_NOCACHE_ADDR, "block in the wrong memory region");
	zassert_equal(reg_map->region.attr, SMH_REG_ATTR_NON_CACHEABLE, "wrong memory attribute");

	/*
	 * Request again a non-cacheable chunk. It should be allocated in the
	 * non-cacheable region RES1
	 */
	block = shared_multi_heap_alloc(SMH_REG_ATTR_NON_CACHEABLE, 0x100);
	reg_map = get_region_map(block);

	zassert_equal(reg_map->p_addr, RES1_NOCACHE_ADDR, "block in the wrong memory region");
	zassert_equal(reg_map->region.attr, SMH_REG_ATTR_NON_CACHEABLE, "wrong memory attribute");

	/* Request a block too big */
	block = shared_multi_heap_alloc(SMH_REG_ATTR_NON_CACHEABLE, 0x10000);
	zassert_is_null(block, "allocated buffer too big for the region");

	/* Request a 0-sized block */
	block = shared_multi_heap_alloc(SMH_REG_ATTR_NON_CACHEABLE, 0);
	zassert_is_null(block, "0 size accepted as valid");

	/* Request a non-existent attribute */
	block = shared_multi_heap_alloc(MAX_SHARED_MULTI_HEAP_ATTR, 0x100);
	zassert_is_null(block, "wrong attribute accepted as valid");
}

#define STRESS_THREADS    4
#define STRESS_STACK_SIZE (1024 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define STRESS_SLOTS      4
#define STRESS_MAX_SIZE   256U
#define STRESS_ITERATIONS 20000

static K_THREAD_STACK_ARRAY_DEFINE(stress_stacks, STRESS_THREADS, STRESS_STACK_SIZE);
static struct k_thread stress_threads[STRESS_THREADS];
static atomic_t stress_errors;

static uint32_t stress_rand(uint32_t *state)
{
	*state = (*state * 1103515245U) + 12345U;

	return *state >> 16;
}

static bool stress_check(const uint8_t *block, uint8_t pattern, size_t size)
{
	for (size_t i = 0; i < size; i++) {
		if (block[i] != pattern) {
			return false;
		}
	}

	return true;
}

static size_t largest_cacheable_block(void)
{
	void *block;

	for (size_t size = 0x4000; size > 0; size -= 0x40) {
		block = shared_multi_heap_alloc(SMH_REG_ATTR_CACHEABLE, size);
		if (block != NULL) {
			shared_multi_heap_free(block);
			return size;
		}
	}

	return 0;
}

static void stress_entry(void *p1, void *p2, void *p3)
{
	uint8_t pattern = (uint8_t)(uintptr_t)p1;
	uint32_t seed = pattern;
	uint8_t *blocks[STRESS_SLOTS] = {NULL};
	size_t sizes[STRESS_SLOTS] = {0};
	uint8_t *block;
	uint32_t slot;
	size_t size;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	for (int i = 0; i < STRESS_ITERATIONS; i++) {
		slot = stress_rand(&seed) % STRESS_SLOTS;
		size = 1U + (stress_rand(&seed) % STRESS_MAX_SIZE);

		if (blocks[slot] != NULL) {
			if (!stress_check(blocks[slot], pattern, sizes[slot])) {
				atomic_inc(&stress_errors);
			}

			if ((stress_rand(&seed) % 4U) == 0U) {
				block = shared_multi_heap_realloc(SMH_REG_ATTR_CACHEABLE,
								  blocks[slot], size);
				if (block != NULL) {
					blocks[slot] = block;
					sizes[slot] = size;
					memset(block, pattern, size);
				}
				continue;
			}

			shared_multi_heap_free(blocks[slot]);
			blocks[slot] = NULL;
			continue;
		}

		block = shared_multi_heap_aligned_alloc(SMH_REG_ATTR_CACHEABLE, 16, size);
		if (block != NULL) {
			memset(block, pattern, size);
			blocks[slot] = block;
			sizes[slot] = size;
		}
	}

	for (slot = 0; slot < STRESS_SLOTS; slot++) {
		if (blocks[slot] != NULL) {
			if (!stress_check(blocks[slot], pattern, sizes[slot])) {
				atomic_inc(&stress_errors);
			}
			shared_multi_heap_free(blocks[slot]);
		}
	}
}

ZTEST(shared_multi_heap, test_shared_multi_heap_concurrent)
{
	size_t largest;

	largest = largest_cacheable_block();
	zassert_not_equal(largest, 0, "no cacheable memory available");

	atomic_set(&stress_errors, 0);
	k_sched_time_slice_set(1, K_PRIO_PREEMPT(1));

	for (int i = 0; i < STRESS_THREADS; i++) {
		k_thread_create(&stress_threads[i], stress_stacks[i],
				K_THREAD_STACK_SIZEOF(stress_stacks[i]), stress_entry,
				(void *)(uintptr_t)(i + 1), NULL, NULL, K_PRIO_PREEMPT(1), 0,
				K_NO_WAIT);
	}

	for (int i = 0; i < STRESS_THREADS; i++) {
		k_thread_join(&stress_threads[i], K_FOREVER);
	}

	k_sched_time_slice_set(CONFIG_TIMESLICE_SIZE, CONFIG_TIMESLICE_PRIORITY);

	zassert_equal(atomic_get(&stress_errors), 0, "block content corrupted");

	/* All blocks are freed, so the cacheable pool must be as it was before */
	zassert_equal(largest_cacheable_block(), largest,
		      "cacheable pool damaged after concurrent use");
}

ZTEST_SUITE(shared_multi_heap, NULL, shared_multi_heap_setup, NULL, NULL, NULL);
