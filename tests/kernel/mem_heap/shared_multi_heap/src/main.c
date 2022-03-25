/*
 * Copyright (c) 2021 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <ztest.h>
#include <linker/linker-defs.h>
#include <sys/mem_manage.h>

#include <multi_heap/shared_multi_heap.h>

#define MAX_REGIONS (3)

static struct {
	struct shared_multi_heap_region *reg;
	uint8_t *v_addr;
} map[MAX_REGIONS];

static bool smh_reg_init(struct shared_multi_heap_region *reg, uint8_t **v_addr, size_t *size)
{
	static int reg_idx;
	uint32_t mem_attr;

	mem_attr = (reg->attr == SMH_REG_ATTR_CACHEABLE) ? K_MEM_CACHE_WB : K_MEM_CACHE_NONE;
	mem_attr |= K_MEM_PERM_RW;

	z_phys_map(v_addr, reg->addr, reg->size, mem_attr);

	*size = reg->size;

	/* Save the mapping to retrieve the region from the vaddr */
	map[reg_idx].reg = reg;
	map[reg_idx].v_addr = *v_addr;

	reg_idx++;

	return true;
}

static struct shared_multi_heap_region *get_reg_addr(uint8_t *v_addr)
{
	for (size_t reg = 0; reg < MAX_REGIONS; reg++) {
		if (v_addr >= map[reg].v_addr &&
		    v_addr < map[reg].v_addr + map[reg].reg->size) {
			return map[reg].reg;
		}
	}
	return NULL;
}

void test_shared_multi_heap(void)
{
	struct shared_multi_heap_region *reg;
	uint8_t *block;

	shared_multi_heap_pool_init(smh_reg_init);

	/*
	 * Request a small cacheable chunk. It should be allocated in the
	 * smaller region (@ 0x42000000)
	 */
	block = shared_multi_heap_alloc(SMH_REG_ATTR_CACHEABLE, 0x40);
	reg = get_reg_addr(block);

	zassert_equal(reg->addr, 0x42000000, "block in the wrong memory region");
	zassert_equal(reg->attr, SMH_REG_ATTR_CACHEABLE, "wrong memory attribute");

	/*
	 * Request another small cacheable chunk. It should be allocated in the
	 * smaller cacheable region (@ 0x42000000)
	 */
	block = shared_multi_heap_alloc(SMH_REG_ATTR_CACHEABLE, 0x80);
	reg = get_reg_addr(block);

	zassert_equal(reg->addr, 0x42000000, "block in the wrong memory region");
	zassert_equal(reg->attr, SMH_REG_ATTR_CACHEABLE, "wrong memory attribute");

	/*
	 * Request a big cacheable chunk. It should be allocated in the
	 * bigger cacheable region (@ 0x44000000)
	 */
	block = shared_multi_heap_alloc(SMH_REG_ATTR_CACHEABLE, 0x1200);
	reg = get_reg_addr(block);

	zassert_equal(reg->addr, 0x44000000, "block in the wrong memory region");
	zassert_equal(reg->attr, SMH_REG_ATTR_CACHEABLE, "wrong memory attribute");

	/*
	 * Request a non-cacheable chunk. It should be allocated in the
	 * non-cacheable region (@ 0x43000000)
	 */
	block = shared_multi_heap_alloc(SMH_REG_ATTR_NON_CACHEABLE, 0x100);
	reg = get_reg_addr(block);

	zassert_equal(reg->addr, 0x43000000, "block in the wrong memory region");
	zassert_equal(reg->attr, SMH_REG_ATTR_NON_CACHEABLE, "wrong memory attribute");

	/*
	 * Request again a non-cacheable chunk. It should be allocated in the
	 * non-cacheable region (@ 0x43000000)
	 */
	block = shared_multi_heap_alloc(SMH_REG_ATTR_NON_CACHEABLE, 0x100);
	reg = get_reg_addr(block);

	zassert_equal(reg->addr, 0x43000000, "block in the wrong memory region");
	zassert_equal(reg->attr, SMH_REG_ATTR_NON_CACHEABLE, "wrong memory attribute");

	/* Request a block too big */
	block = shared_multi_heap_alloc(SMH_REG_ATTR_NON_CACHEABLE, 0x10000);
	zassert_is_null(block, "allocated buffer too big for the region");

	/* Request a 0-sized block */
	block = shared_multi_heap_alloc(SMH_REG_ATTR_NON_CACHEABLE, 0);
	zassert_is_null(block, "0 size accepted as valid");

	/* Request a non-existent attribute */
	block = shared_multi_heap_alloc(SMH_REG_ATTR_NUM + 1, 0x100);
	zassert_is_null(block, "wrong attribute accepted as valid");

}

void test_main(void)
{
	ztest_test_suite(shared_multi_heap,
			 ztest_1cpu_unit_test(test_shared_multi_heap));
	ztest_run_test_suite(shared_multi_heap);
}
