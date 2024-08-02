/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/drivers/mm/mm_drv_bank.h>

#define BANK_PAGES  64

#define EXPECTED(x)  ((x) * CONFIG_MM_DRV_PAGE_SIZE)

static struct sys_mm_drv_bank bank_data = {0x123, 0x234, 0x345};

static void test_stats(const char *error_string,
		       struct sys_memory_stats *stats,
		       struct sys_memory_stats *expected)
{
	zassert_equal(stats->free_bytes, expected->free_bytes,
		      "%s: [free_bytes] = %u, not %u\n", error_string,
		      stats->free_bytes, expected->free_bytes);
	zassert_equal(stats->allocated_bytes, expected->allocated_bytes,
		      "%s: [allocated_bytes] = %u, not %u\n", error_string,
		      stats->allocated_bytes, expected->allocated_bytes);
	zassert_equal(stats->max_allocated_bytes, expected->max_allocated_bytes,
		      "%s: [max_allocated_bytes] = %u, not %u\n", error_string,
		      stats->max_allocated_bytes,
		      expected->max_allocated_bytes);
}

ZTEST(sys_mm_bank_api, test_sys_mm_drv_bank)
{
	struct sys_memory_stats stats;
	struct sys_memory_stats expected;
	uint32_t  count;
	uint32_t index;

	/* Verify that the initialization routine works as expected. */
	/* It set mapped state for all pages                         */
	sys_mm_drv_bank_init(&bank_data, BANK_PAGES);
	expected.free_bytes = EXPECTED(0);
	expected.allocated_bytes = EXPECTED(BANK_PAGES);
	expected.max_allocated_bytes = EXPECTED(BANK_PAGES);
	sys_mm_drv_bank_stats_get(&bank_data, &stats);
	test_stats("MM Bank Init Error", &stats, &expected);

	/* Now unmap all pages */
	for (index = 0; index < BANK_PAGES; index++) {
		sys_mm_drv_bank_page_unmapped(&bank_data);
	}
	sys_mm_drv_bank_stats_reset_max(&bank_data);

	expected.free_bytes = EXPECTED(BANK_PAGES);
	expected.allocated_bytes = EXPECTED(0);
	expected.max_allocated_bytes = EXPECTED(0);
	sys_mm_drv_bank_stats_get(&bank_data, &stats);
	test_stats("MM Bank Init Error", &stats, &expected);

	/* Verify mapped pages are counted correctly */
	count = sys_mm_drv_bank_page_mapped(&bank_data);
	zassert_equal(count, 1,
		      "MM Page Mapped Error: 1st mapped = %u, not %u\n",
		      count, 1);

	count = sys_mm_drv_bank_page_mapped(&bank_data);
	zassert_equal(count, 2,
		      "MM Page Mapped Error: 2nd mapped = %u, not %u\n",
		      count, 2);


	expected.free_bytes = EXPECTED(BANK_PAGES - 2);
	expected.allocated_bytes = EXPECTED(2);
	expected.max_allocated_bytes = EXPECTED(2);
	sys_mm_drv_bank_stats_get(&bank_data, &stats);
	test_stats("MM Bank Mapped Error", &stats, &expected);

	/* Verify unmapped pages are counted correctly */

	count = sys_mm_drv_bank_page_unmapped(&bank_data);
	zassert_equal(count, BANK_PAGES - 1,
		      "MM Page Unmapped Error: Pages mapped = %u, not 1\n",
		      count, BANK_PAGES - 1);
	expected.free_bytes = EXPECTED(BANK_PAGES - 1);
	expected.allocated_bytes = EXPECTED(1);
	expected.max_allocated_bytes = EXPECTED(2);
	sys_mm_drv_bank_stats_get(&bank_data, &stats);
	test_stats("MM Bank Unmapped Error", &stats, &expected);

	/* Verify max mapped pages are counted correctly */

	count = sys_mm_drv_bank_page_mapped(&bank_data);
	zassert_equal(count, 2,
		      "MM Page Mapped Error: 3rd mapped = %u, not %u\n",
		      count, 2);
	expected.free_bytes = EXPECTED(BANK_PAGES - 2);
	expected.allocated_bytes = EXPECTED(2);
	expected.max_allocated_bytes = EXPECTED(2);
	sys_mm_drv_bank_stats_get(&bank_data, &stats);
	test_stats("MM Bank 1st Max Mapped Error", &stats, &expected);

	count = sys_mm_drv_bank_page_mapped(&bank_data);
	zassert_equal(count, 3,
		      "MM Page Mapped Error: 3rd mapped = %u, not %u\n",
		      count, 3);
	expected.free_bytes = EXPECTED(BANK_PAGES - 3);
	expected.allocated_bytes = EXPECTED(3);
	expected.max_allocated_bytes = EXPECTED(3);
	sys_mm_drv_bank_stats_get(&bank_data, &stats);
	test_stats("MM Bank 2nd Max Mapped Error", &stats, &expected);

	/* Verify sys_mm_drv_bank_stats_reset_max works correctly */

	count = sys_mm_drv_bank_page_unmapped(&bank_data);
	zassert_equal(count, BANK_PAGES - 2,
		      "MM Bank Reset Max Error: unmapped = %u, not %u\n",
		      count, BANK_PAGES - 2);
	expected.free_bytes = EXPECTED(BANK_PAGES - 2);
	expected.allocated_bytes = EXPECTED(2);
	expected.max_allocated_bytes = EXPECTED(3);
	sys_mm_drv_bank_stats_get(&bank_data, &stats);
	test_stats("MM Bank Reset Max Error", &stats, &expected);

	sys_mm_drv_bank_stats_reset_max(&bank_data);
	expected.free_bytes = EXPECTED(BANK_PAGES - 2);
	expected.allocated_bytes = EXPECTED(2);
	expected.max_allocated_bytes = EXPECTED(2);
	sys_mm_drv_bank_stats_get(&bank_data, &stats);
	test_stats("MM Bank Reset Max Error", &stats, &expected);
}

ZTEST_SUITE(sys_mm_bank_api, NULL, NULL, NULL, NULL, NULL);
