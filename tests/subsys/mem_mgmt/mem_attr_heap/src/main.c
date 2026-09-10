/*
 * Copyright (c) 2023 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/mem_mgmt/mem_attr_heap.h>
#include <zephyr/dt-bindings/memory-attr/memory-attr-sw.h>

#define ADDR_MEM_CACHE		DT_REG_ADDR(DT_NODELABEL(mem_cache))
#define ADDR_MEM_CACHE_SW	DT_REG_ADDR(DT_NODELABEL(mem_cache_sw))
#define ADDR_MEM_NON_CACHE_SW	DT_REG_ADDR(DT_NODELABEL(mem_noncache_sw))
#define ADDR_MEM_DMA_SW		DT_REG_ADDR(DT_NODELABEL(mem_dma_sw))
#define ADDR_MEM_CACHE_BIG_SW	DT_REG_ADDR(DT_NODELABEL(mem_cache_sw_big))
#define ADDR_MEM_CACHE_DMA_SW	DT_REG_ADDR(DT_NODELABEL(mem_cache_cache_dma_multi))

ZTEST(mem_attr_heap, test_mem_attr_heap)
{
	const struct mem_attr_region_t *region;
	void *block, *old_block;
	int ret;

	/*
	 * Init the pool.
	 */
	ret = mem_attr_heap_pool_init();
	zassert_equal(0, ret, "Failed initialization");

	/*
	 * Any subsequent initialization should fail.
	 */
	ret = mem_attr_heap_pool_init();
	zassert_equal(-EALREADY, ret, "Second init should be failing");

	/*
	 * Allocate 0x100 bytes of cacheable memory.
	 */
	block = mem_attr_heap_alloc(DT_MEM_SW_ALLOC_CACHE, 0x100);
	zassert_not_null(block, "Failed to allocate memory");

	/*
	 * Check that the just allocated memory was allocated from the correct
	 * memory region.
	 */
	region = mem_attr_heap_get_region(block);
	zassert_equal(region->dt_addr, ADDR_MEM_CACHE_SW,
		      "Memory allocated from the wrong region");

	/*
	 * Allocate 0x100 bytes of non-cacheable memory.
	 */
	block = mem_attr_heap_alloc(DT_MEM_SW_ALLOC_NON_CACHE, 0x100);
	zassert_not_null(block, "Failed to allocate memory");

	/*
	 * Check that the just allocated memory was allocated from the correct
	 * memory region.
	 */
	region = mem_attr_heap_get_region(block);
	zassert_equal(region->dt_addr, ADDR_MEM_NON_CACHE_SW,
		      "Memory allocated from the wrong region");

	/*
	 * Allocate 0x100 bytes of DMA memory.
	 */
	block = mem_attr_heap_alloc(DT_MEM_SW_ALLOC_DMA, 0x100);
	zassert_not_null(block, "Failed to allocate memory");

	/*
	 * Check that the just allocated memory was allocated from the correct
	 * memory region.
	 */
	region = mem_attr_heap_get_region(block);
	zassert_equal(region->dt_addr, ADDR_MEM_DMA_SW,
		      "Memory allocated from the wrong region");

	/*
	 * Allocate 0x100 bytes of cacheable and DMA memory.
	 */
	block = mem_attr_heap_alloc(DT_MEM_SW_ALLOC_CACHE | DT_MEM_SW_ALLOC_DMA, 0x100);
	zassert_not_null(block, "Failed to allocate memory");

	/*
	 * Check that the just allocated memory was allocated from the correct
	 * memory region (CACHE + DMA and not just CACHE or just DMA).
	 */
	region = mem_attr_heap_get_region(block);
	zassert_equal(region->dt_addr, ADDR_MEM_CACHE_DMA_SW,
		      "Memory allocated from the wrong region");

	/*
	 * Allocate memory with a non-existing attribute.
	 */
	block = mem_attr_heap_alloc(DT_MEM_SW(DT_MEM_SW_ATTR_UNKNOWN), 0x100);
	zassert_is_null(block, "Memory allocated with non-existing attribute");

	/*
	 * Allocate memory too big to fit into the first cacheable memory
	 * region. It should be allocated from the second bigger memory region.
	 */
	block = mem_attr_heap_alloc(DT_MEM_SW_ALLOC_CACHE, 0x1500);
	zassert_not_null(block, "Failed to allocate memory");

	/*
	 * Check that the just allocated memory was allocated from the correct
	 * (bigger) cacheable memory region
	 */
	region = mem_attr_heap_get_region(block);
	zassert_equal(region->dt_addr, ADDR_MEM_CACHE_BIG_SW,
		      "Memory allocated from the wrong region");

	/*
	 * Try to allocate a buffer too big.
	 */
	block = mem_attr_heap_alloc(DT_MEM_SW_ALLOC_CACHE, 0x4000);
	zassert_is_null(block, "Buffer too big for regions correctly allocated");

	/*
	 * Check if the memory is correctly released and can be reused
	 */
	block = mem_attr_heap_alloc(DT_MEM_SW_ALLOC_CACHE, 0x100);
	old_block = block;
	mem_attr_heap_free(block);
	block = mem_attr_heap_alloc(DT_MEM_SW_ALLOC_CACHE, 0x100);
	zassert_equal_ptr(old_block, block, "Memory not correctly released");

	/*
	 * Check if the memory is correctly aligned when requested
	 */
	block = mem_attr_heap_alloc(DT_MEM_SW_ALLOC_NON_CACHE, 0x100);
	zassert_true(((uintptr_t) block % 32 != 0), "");
	mem_attr_heap_free(block);
	block = mem_attr_heap_aligned_alloc(DT_MEM_SW_ALLOC_NON_CACHE, 0x100, 32);
	zassert_true(((uintptr_t) block % 32 == 0), "");

	/*
	 * Try with a different alignment
	 */
	block = mem_attr_heap_aligned_alloc(DT_MEM_SW_ALLOC_NON_CACHE, 0x100, 64);
	zassert_true(((uintptr_t) block % 64 == 0), "");
}

ZTEST_SUITE(mem_attr_heap, NULL, NULL, NULL, NULL, NULL);
