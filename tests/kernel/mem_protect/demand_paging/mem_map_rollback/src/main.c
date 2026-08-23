/*
 * Copyright (c) 2026 Hui Su
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel/mm.h>
#include <zephyr/ztest.h>

ZTEST(mem_map_rollback, test_partial_mapping_failure_is_recoverable)
{
	size_t free_mem = k_mem_free_get();
	/* The RAM backing store reserves its last slot for page faults. */
	size_t backing_store = (CONFIG_BACKING_STORE_RAM_PAGES - 1) *
			       CONFIG_MMU_PAGE_SIZE;
	size_t failing_size = free_mem + backing_store + CONFIG_MMU_PAGE_SIZE;
	size_t successful_size = failing_size - CONFIG_MMU_PAGE_SIZE;
	void *mapped;

	zassert_not_equal(free_mem, 0, "no free memory");

	/* This mapping succeeds for several pages, then fails when both RAM and
	 * backing store are exhausted. It must release all partial state.
	 */
	mapped = k_mem_map(failing_size, K_MEM_PERM_RW);
	zassert_is_null(mapped, "mapping should fail after partial allocation");
	zassert_equal(k_mem_free_get(), free_mem,
		      "partial mapping leaked physical memory");

	/* A mapping that fits exactly in the recovered RAM and backing store must
	 * still succeed. This also detects leaked paged-out backing-store slots.
	 */
	mapped = k_mem_map(successful_size, K_MEM_PERM_RW);
	zassert_not_null(mapped, "mapping resources were not fully recovered");
	k_mem_unmap(mapped, successful_size);

	zassert_equal(k_mem_free_get(), free_mem,
		      "mapping cleanup did not restore physical memory");
}

ZTEST_SUITE(mem_map_rollback, NULL, NULL, NULL, NULL, NULL);
