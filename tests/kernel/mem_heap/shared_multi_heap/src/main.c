/*
 * Copyright (c) 2021 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <ztest.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/sys/mem_manage.h>

#include <zephyr/multi_heap/shared_multi_heap.h>

#define DT_DRV_COMPAT		zephyr_memory_region

#define RES0_CACHE_ADDR		DT_REG_ADDR(DT_NODELABEL(res0))
#define RES1_NOCACHE_ADDR	DT_REG_ADDR(DT_NODELABEL(res1))
#define RES2_CACHE_ADDR		DT_REG_ADDR(DT_NODELABEL(res2))

struct region_map {
	struct shared_multi_heap_region region;
	uintptr_t p_addr;
};

#define FOREACH_REG(n)									\
	{										\
		.region = {								\
			.addr = (uintptr_t) DT_INST_REG_ADDR(n),			\
			.size = DT_INST_REG_SIZE(n),					\
			.attr = DT_INST_ENUM_IDX_OR(n, zephyr_memory_region_mpu,	\
						    SMH_REG_ATTR_NUM),			\
		},									\
	},

struct region_map map[] = {
	DT_INST_FOREACH_STATUS_OKAY(FOREACH_REG)
};

#if defined(CONFIG_MMU)
static void smh_reg_map(struct shared_multi_heap_region *region)
{
	uint32_t mem_attr;
	uint8_t *v_addr;

	mem_attr = (region->attr == SMH_REG_ATTR_CACHEABLE) ? K_MEM_CACHE_WB : K_MEM_CACHE_NONE;
	mem_attr |= K_MEM_PERM_RW;

	z_phys_map(&v_addr, region->addr, region->size, mem_attr);

	region->addr = (uintptr_t) v_addr;
}
#endif /* CONFIG_MMU */

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

static inline enum smh_reg_attr mpu_to_reg_attr(int mpu_attr)
{
	/*
	 * All the memory regions defined in the DT with the MPU property `RAM`
	 * can be accessed and memory can be retrieved from using the attribute
	 * `SMH_REG_ATTR_CACHEABLE`.
	 *
	 * All the memory regions defined in the DT with the MPU property
	 * `RAM_NOCACHE` can be accessed and memory can be retrieved from using
	 * the attribute `SMH_REG_ATTR_NON_CACHEABLE`.
	 *
	 * [MPU attr]   -> [SMH attr]
	 *
	 * RAM          -> SMH_REG_ATTR_CACHEABLE
	 * RAM_NOCACHE  -> SMH_REG_ATTR_NON_CACHEABLE
	 */
	switch (mpu_attr) {
	case 0: /* RAM */
		return SMH_REG_ATTR_CACHEABLE;
	case 1: /* RAM_NOCACHE */
		return SMH_REG_ATTR_NON_CACHEABLE;
	default:
		/* How ? */
		ztest_test_fail();
	}

	/* whatever */
	return 0;
}

static void fill_multi_heap(void)
{
	struct region_map *reg_map;

	for (size_t idx = 0; idx < DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT); idx++) {
		reg_map = &map[idx];

		/* zephyr,memory-region-mpu property not found. Skip it. */
		if (reg_map->region.attr == SMH_REG_ATTR_NUM) {
			continue;
		}

		/* Convert MPU attributes to shared-multi-heap capabilities */
		reg_map->region.attr = mpu_to_reg_attr(reg_map->region.attr);

		/* Assume for now that phys == virt */
		reg_map->p_addr = reg_map->region.addr;

#if defined(CONFIG_MMU)
		/*
		 * For MMU-enabled platform we have to MMU-map the physical
		 * address retrieved by DT at run-time because the SMH
		 * framework expects virtual addresses.
		 *
		 * For MPU-enabled plaform the code is assuming that the region
		 * are configured at build-time, so no map is needed.
		 */
		smh_reg_map(&reg_map->region);
#endif /* CONFIG_MMU */

		shared_multi_heap_add(&reg_map->region, NULL);
	}
}

void test_shared_multi_heap(void)
{
	struct region_map *reg_map;
	void *block;
	int ret;

	ret = shared_multi_heap_pool_init();
	zassert_equal(0, ret, "failed initialization");

	/*
	 * Return -EALREADY if already inited
	 */
	ret = shared_multi_heap_pool_init();
	zassert_equal(-EALREADY, ret, "second init should fail");

	/*
	 * Fill the buffer pool with the memory heaps coming from DT
	 */
	fill_multi_heap();

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

void test_main(void)
{
	ztest_test_suite(shared_multi_heap,
			 ztest_1cpu_unit_test(test_shared_multi_heap));
	ztest_run_test_suite(shared_multi_heap);
}
