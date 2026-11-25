/*
 * Copyright (c) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Check that the Intel ADSP TLB driver does not replace an existing
 * mapping, and that moving a region keeps its data and does not fault.
 */

#include <zephyr/ztest.h>
#include <zephyr/cache.h>
#include <zephyr/drivers/mm/system_mm.h>
#include <adsp_memory.h>

#define PAGE_SZ		CONFIG_MM_DRV_PAGE_SIZE
#define FLAGS		SYS_MM_MEM_PERM_RW
#define TEST_PAGES	4

/* Unused virtual pages at the end of the L2 virtual window */
#define TEST_VA(n)	((uint32_t *)(L2_VIRTUAL_SRAM_BASE + L2_VIRTUAL_SRAM_SIZE - \
				      (TEST_PAGES - (n)) * PAGE_SZ))

static uintptr_t phys_of(void *va)
{
	uintptr_t pa = 0;

	zassert_ok(sys_mm_drv_page_phys_get(va, &pa), "%p is not mapped", va);

	return pa;
}

static void mark(uint32_t *va, uint32_t marker)
{
	va[0] = marker;
	va[PAGE_SZ / sizeof(uint32_t) - 1] = ~marker;
	sys_cache_data_flush_and_invd_range(va, PAGE_SZ);
}

static bool marked(uint32_t *va, uint32_t marker)
{
	sys_cache_data_invd_range(va, PAGE_SZ);

	return va[0] == marker && va[PAGE_SZ / sizeof(uint32_t) - 1] == ~marker;
}

static void unmap_all(void *fixture)
{
	ARG_UNUSED(fixture);

	for (int n = 0; n < TEST_PAGES; n++) {
		if (!sys_mm_drv_page_phys_get(TEST_VA(n), NULL)) {
			(void)sys_mm_drv_unmap_page(TEST_VA(n));
		}
	}
}

/* Mapping an already mapped virtual page must fail and keep the mapping */
ZTEST(adsp_tlb, test_map_mapped_page_fails)
{
	uintptr_t pa;

	zassert_ok(sys_mm_drv_map_page(TEST_VA(0), 0, FLAGS));
	pa = phys_of(TEST_VA(0));
	mark(TEST_VA(0), 0x11111111);

	zassert_equal(sys_mm_drv_map_page(TEST_VA(0), 0, FLAGS), -EFAULT,
		      "mapping a mapped page did not fail");
	zassert_equal(phys_of(TEST_VA(0)), pa, "mapping a mapped page replaced it");
	zassert_true(marked(TEST_VA(0), 0x11111111), "mapped page lost its data");
}

/* Moving a region must keep its data and unmap the old pages */
ZTEST(adsp_tlb, test_move_region)
{
	uintptr_t pa;

	zassert_ok(sys_mm_drv_map_region(TEST_VA(0), 0, 2 * PAGE_SZ, FLAGS));
	pa = phys_of(TEST_VA(1));
	mark(TEST_VA(0), 0x22222222);
	mark(TEST_VA(1), 0x33333333);

	zassert_ok(sys_mm_drv_move_region(TEST_VA(0), 2 * PAGE_SZ, TEST_VA(2), 0));

	zassert_equal(phys_of(TEST_VA(3)), pa, "move changed the physical page");
	zassert_true(marked(TEST_VA(2), 0x22222222), "data not moved");
	zassert_true(marked(TEST_VA(3), 0x33333333), "data not moved");
	zassert_not_ok(sys_mm_drv_page_phys_get(TEST_VA(0), NULL), "old page still mapped");
	zassert_not_ok(sys_mm_drv_page_phys_get(TEST_VA(1), NULL), "old page still mapped");
}

ZTEST_SUITE(adsp_tlb, NULL, NULL, unmap_all, unmap_all, NULL);
