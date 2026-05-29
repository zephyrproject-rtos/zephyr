/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/cache.h>

#include <zephyr/toolchain.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include <zephyr/drivers/mm/system_mm.h>

#include <soc.h>
#include <adsp_memory.h>

#define N_PAGES 3
#define PAGE_SZ CONFIG_MM_DRV_PAGE_SIZE

struct pagemem {
	uint32_t mem[1024];
};

/*
 * Test 3 pages at a time, but another set of 3 pages
 * is needed for remapping test.
 */
uint8_t __aligned(PAGE_SZ) buf[2 * N_PAGES * PAGE_SZ];

ZTEST(adsp_mem, test_adsp_mem_map_region)
{
	int ret;
	uintptr_t pa[N_PAGES];
	struct pagemem *page_buf = (void *)buf;

	/* Find a virtual address beyond physical memory */
	void *va = (void *)ROUND_UP(L2_SRAM_BASE + L2_SRAM_SIZE, PAGE_SZ);

	for (int i = 0; i < N_PAGES; i++) {
		/* First N_PAGES pages of page_buf */
		pa[i] = POINTER_TO_UINT(&page_buf[i]);
	}

	/* Map our physical pages to the new location */
	ret = sys_mm_drv_map_region(va, pa[0], N_PAGES * PAGE_SZ, 0U);

	zassert_true(ret == 0, "sys_mm_drv_map_region() failed (%d)", ret);

	/* Mark the new pages */
	struct pagemem *vps = va;
	const uint32_t markers[] = { 0x11111111, 0x22222222, 0x33333333 };

	for (int i = 0; i < N_PAGES; i++) {
		vps[i].mem[0] = markers[i];

		/*
		 * Make sure it is written back to the mapped
		 * physical memory.
		 */
		sys_cache_data_flush_range(&vps[i].mem[0], PAGE_SZ);

		/*
		 * pa[i] is a cached address which means that the cached
		 * version still has the old value before the assignment
		 * above. So we need to invalidate the cache to reload
		 * the new value.
		 */
		sys_cache_data_invd_range(UINT_TO_POINTER(pa[i]), PAGE_SZ);
	}

	/* Verify the originals reflect the change */
	for (int i = 0; i < N_PAGES; i++) {
		zassert_equal(vps[i].mem[0], *(uint32_t *)pa[i],
			      "mapping and original don't match (0x%x != 0x%x)",
			      vps[i].mem[0], *(uint32_t *)pa[i]);
	}

	/* Remap to another region (this will unmap the first virtual pages) */
	struct pagemem *vps2 = &vps[N_PAGES];

	/*
	 * Need to unmap vps2 as sys_mm_drv_remap_region() checks
	 * if the new virtual memory region is all unmapped.
	 */
	ret = sys_mm_drv_unmap_region(vps2, N_PAGES * PAGE_SZ);
	zassert_true(ret == 0, "sys_mm_drv_unmap_region() failed (%d)", ret);

	ret = sys_mm_drv_remap_region(vps, N_PAGES * PAGE_SZ, vps2);
	zassert_true(ret == 0, "sys_mm_drv_remap_region() failed (%d)", ret);

	for (int i = 0; i < N_PAGES; i++) {
		zassert_equal(vps2[i].mem[0], *(uint32_t *)pa[i],
			      "remapping and original don't match");
	}

	/* Now copy the remapped pages to a third virtual region */
	struct pagemem *vps3 = &vps2[N_PAGES];
	uintptr_t new_pages[N_PAGES];

	for (int i = 0; i < N_PAGES; i++) {
		/* N_PAGES after the first N_PAGES pages of page_buf */
		new_pages[i] = POINTER_TO_UINT(&page_buf[i + N_PAGES]);
	}

	/*
	 * Need to unmap vps3 as sys_mm_drv_remap_region() checks
	 * if the new virtual memory region is all unmapped.
	 */
	ret = sys_mm_drv_unmap_region(vps3, N_PAGES * PAGE_SZ);
	zassert_true(ret == 0, "sys_mm_drv_unmap_region() failed (%d)", ret);

	ret = sys_mm_drv_move_region(vps2, N_PAGES * PAGE_SZ,
				     vps3, new_pages[0]);
	zassert_true(ret == 0, "sys_mm_drv_move_region() failed (%d)", ret);

	/* Make sure they match the originals */
	for (int i = 0; i < N_PAGES; i++) {
		zassert_equal(vps3[i].mem[0], *(uint32_t *)pa[i],
			      "page copy failed to match data (0x%x != 0x%x)",
			      vps3[i].mem[0], *(uint32_t *)pa[i]);
	}

	/* Modify the copy and make sure the originals are unmodified */
	const int poison = 0x5a5a5a5a;

	for (int i = 0; i < N_PAGES; i++) {
		vps3[i].mem[0] = poison;

		/*
		 * Make sure it is written back to the mapped
		 * physical memory.
		 */
		sys_cache_data_flush_range(&vps3[i].mem[0], PAGE_SZ);

		zassert_equal(*(int *)pa[i], markers[i],
			      "page copy modified original");
	}

	/* Unmap */
	ret = sys_mm_drv_unmap_region(vps2, N_PAGES * PAGE_SZ);
	zassert_true(ret == 0, "sys_mm_drv_unmap_region() failed (%d)", ret);

	/* Verify the copied region was not unmapped */
	for (int i = 0; i < N_PAGES; i++) {
		zassert_equal(vps3[i].mem[0], poison,
			      "copied page data changed after unmap");
	}

	/* Verify the originals are still unmodified */
	for (int i = 0; i < N_PAGES; i++) {
		zassert_equal(*(int *)pa[i], markers[i],
			      "original page data changed after unmap");
	}
}

ZTEST(adsp_mem, test_adsp_mem_map_array)
{
	int ret;
	uintptr_t pa[N_PAGES];
	struct pagemem *page_buf = (void *)buf;

	/* Find a virtual address beyond physical memory */
	void *va = (void *)ROUND_UP(L2_SRAM_BASE + L2_SRAM_SIZE, PAGE_SZ);

	for (int i = 0; i < N_PAGES; i++) {
		/* Uses pages #0, #2, #4 */
		pa[i] = POINTER_TO_UINT(&page_buf[i * 2]);
	}

	/* Map our physical pages to the new location */
	ret = sys_mm_drv_map_array(va, pa, N_PAGES, 0U);

	zassert_true(ret == 0, "sys_mm_drv_map_array() failed (%d)", ret);

	/* Mark the new pages */
	struct pagemem *vps = va;
	const uint32_t markers[] = { 0x11111111, 0x22222222, 0x33333333 };

	for (int i = 0; i < N_PAGES; i++) {
		vps[i].mem[0] = markers[i];

		/*
		 * Make sure it is written back to the mapped
		 * physical memory.
		 */
		sys_cache_data_flush_range(&vps[i].mem[0], PAGE_SZ);

		/*
		 * pa[i] is a cached address which means that the cached
		 * version still has the old value before the assignment
		 * above. So we need to invalidate the cache to reload
		 * the new value.
		 */
		sys_cache_data_invd_range(UINT_TO_POINTER(pa[i]), PAGE_SZ);
	}

	/* Verify the originals reflect the change */
	for (int i = 0; i < N_PAGES; i++) {
		zassert_equal(vps[i].mem[0], *(uint32_t *)pa[i],
			      "mapping and original don't match (0x%x != 0x%x)",
			      vps[i].mem[0], *(uint32_t *)pa[i]);
	}

	/* Remap to another region (this will unmap the first virtual pages) */
	struct pagemem *vps2 = &vps[N_PAGES];

	/*
	 * Need to unmap vps2 as sys_mm_drv_remap_region() checks
	 * if the new virtual memory region is all unmapped.
	 */
	ret = sys_mm_drv_unmap_region(vps2, N_PAGES * PAGE_SZ);
	zassert_true(ret == 0, "sys_mm_drv_unmap_region() failed (%d)", ret);

	ret = sys_mm_drv_remap_region(vps, N_PAGES * PAGE_SZ, vps2);
	zassert_true(ret == 0, "sys_mm_drv_remap_region() failed (%d)", ret);

	for (int i = 0; i < N_PAGES; i++) {
		zassert_equal(vps2[i].mem[0], *(uint32_t *)pa[i],
			      "remapping and original don't match");
	}

	/* Now copy the remapped pages to a third virtual region */
	struct pagemem *vps3 = &vps2[N_PAGES];
	uintptr_t new_pages[N_PAGES];

	for (int i = 0; i < N_PAGES; i++) {
		/* Uses pages #1, #3, #5 */
		new_pages[i] = POINTER_TO_UINT(&page_buf[i * 2 + 1]);
	}

	/*
	 * Need to unmap vps3 as sys_mm_drv_remap_region() checks
	 * if the new virtual memory region is all unmapped.
	 */
	ret = sys_mm_drv_unmap_region(vps3, N_PAGES * PAGE_SZ);
	zassert_true(ret == 0, "sys_mm_drv_unmap_region() failed (%d)", ret);

	ret = sys_mm_drv_move_array(vps2, N_PAGES * PAGE_SZ,
				    vps3, new_pages, N_PAGES);
	zassert_true(ret == 0, "sys_mm_drv_move_array() failed (%d)", ret);

	/* Make sure they match the originals */
	for (int i = 0; i < N_PAGES; i++) {
		zassert_equal(vps3[i].mem[0], *(uint32_t *)pa[i],
			      "page copy failed to match data (0x%x != 0x%x)",
			      vps3[i].mem[0], *(uint32_t *)pa[i]);
	}

	/* Modify the copy and make sure the originals are unmodified */
	const int poison = 0x5a5a5a5a;

	for (int i = 0; i < N_PAGES; i++) {
		vps3[i].mem[0] = poison;

		/*
		 * Make sure it is written back to the mapped
		 * physical memory.
		 */
		sys_cache_data_flush_range(&vps3[i].mem[0], PAGE_SZ);

		zassert_equal(*(int *)pa[i], markers[i],
			      "page copy modified original");
	}

	/* Unmap */
	ret = sys_mm_drv_unmap_region(vps2, N_PAGES * PAGE_SZ);
	zassert_true(ret == 0, "sys_mm_drv_unmap_region() failed (%d)", ret);

	/* Verify the copied region was not unmapped */
	for (int i = 0; i < N_PAGES; i++) {
		zassert_equal(vps3[i].mem[0], poison,
			      "copied page data changed after unmap");
	}

	/* Verify the originals are still unmodified */
	for (int i = 0; i < N_PAGES; i++) {
		zassert_equal(*(int *)pa[i], markers[i],
			      "original page data changed after unmap");
	}
}

ZTEST_SUITE(adsp_mem, NULL, NULL, NULL, NULL, NULL);
