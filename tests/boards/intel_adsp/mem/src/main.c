/* Copyright (c) 2021 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <zephyr.h>
#include <sys/printk.h>
#include <soc/memory.h>
#include <adsp_mem.h>

extern char _heap_sentry;

#define N_PAGES 3

struct pagemem {
	uint32_t mem[1024];
};

void test_adsp_mem(void)
{
	soc_adsp_page_id_t p[N_PAGES];
	void *pa[N_PAGES];

	for (int i = 0; i < N_PAGES; i++) {
		p[i] = soc_adsp_page_alloc(SOC_ADSP_MEM_HP_SRAM);
		zassert_true(p[i] > 0, "page alloc failed");
		pa[i] = soc_adsp_page_addr(p[i]);
		zassert_not_null(pa[i], "no addr for page");
	}

	/* Whitebox, make sure a page we just freed is the first one
	 * returned.  Easy way of ensuring that the free added it
	 * correctly, even though we don't specify this ordering.
	 */
	soc_adsp_page_free(p[2]);
	zassert_equal(p[2], soc_adsp_page_alloc(SOC_ADSP_MEM_HP_SRAM),
		      "just-freed page not reused");

	/* Find a virtual address beyond physical memory */
	void *va = (void *)ROUND_UP(HP_SRAM_BASE + HP_SRAM_SIZE, 4096);

	va = z_soc_uncached_ptr(va);

	/* Map our physical pages to the new location */
	int32_t ret = soc_adsp_pages_map(N_PAGES, p, va); /* assert == 0 */

	zassert_true(ret == 0, "soc_adsp_pages_map() failed");

	/* Mark the new pages */
	struct pagemem *vps = va;
	const uint32_t markers[] = { 0x11111111, 0x22222222, 0x33333333 };

	for (int i = 0; i < N_PAGES; i++) {
		vps[i].mem[0] = markers[i];
	}

	/* Verify the originals reflect the change */
	for (int i = 0; i < N_PAGES; i++) {
		zassert_equal(vps[i].mem[0], *(uint32_t *)pa[i],
			      "mapping and original don't match");
	}

	/* Remap to another region (this will unmap the first virtual pages) */
	struct pagemem *vps2 = &vps[N_PAGES];

	ret = soc_adsp_pages_remap(N_PAGES, vps, vps2);
	zassert_true(ret == 0, "soc_adsp_pages_remap() failed");

	for (int i = 0; i < N_PAGES; i++) {
		zassert_equal(vps2[i].mem[0], *(uint32_t *)pa[i],
			      "remapping and original don't match");
	}

	/* Now copy the remapped pages to a third virtual region */
	struct pagemem *vps3 = &vps2[N_PAGES];
	soc_adsp_page_id_t new_pages[N_PAGES];

	ret = soc_adsp_pages_alloc(SOC_ADSP_MEM_HP_SRAM, N_PAGES, new_pages);
	zassert_true(ret == 0, "soc_adsp_pages_alloc failed");

	ret = soc_adsp_pages_copy(N_PAGES, vps2, vps3, new_pages);
	zassert_true(ret == 0, "soc_adsp_pages_copy failed");

	/* Make sure they match the originals */
	for (int i = 0; i < N_PAGES; i++) {
		zassert_equal(*(int *)pa[i], vps3[i].mem[0],
			      "page copy failed to match data");
	}

	/* Modify the copy and make sure the originals are unmodified */
	const int poison = 0x5a5a5a5a;

	for (int i = 0; i < N_PAGES; i++) {
		vps3[i].mem[0] = poison;
		zassert_equal(*(int *)pa[i], markers[i],
			      "page copy modified original");
	}

	/* Unmap */
	ret = soc_adsp_pages_unmap(N_PAGES, vps2);
	zassert_true(ret == 0, "soc_adsp_pages_unmap() failed");

	/* Verify the copied region was not unmapped */
	for (int i = 0; i < N_PAGES; i++) {
		zassert_equal(vps3[i].mem[0], poison,
			      "original page data changed after unmap");
	}

	/* Verify the originals are still unmodified */
	for (int i = 0; i < N_PAGES; i++) {
		zassert_equal(*(int *)pa[i], markers[i],
			      "original page data changed after unmap");
	}
}


void test_main(void)
{
	ztest_test_suite(adsp_mem,
			 ztest_unit_test(test_adsp_mem));

	ztest_run_test_suite(adsp_mem);
}
