/*
 * Copyright (c) 2026 Process Mission
 *
 * Demand paging from user mode: accessing a paged-out anonymous page
 * from a user thread must page it back in, with its contents intact,
 * and the page must be writable again once paged in.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel/mm.h>
#include <zephyr/kernel/mm/demand_paging.h>

static ZTEST_DMEM char *user_page;

static void user_fault_before(void *data)
{
	ARG_UNUSED(data);

	user_page = k_mem_map(CONFIG_MMU_PAGE_SIZE, K_MEM_PERM_RW | K_MEM_PERM_USER);
	zassert_not_null(user_page, "k_mem_map failed");

	/* fault the page in from kernel context, then evict it */
	memset(user_page, 0x5a, CONFIG_MMU_PAGE_SIZE);
	zassert_equal(k_mem_page_out(user_page, CONFIG_MMU_PAGE_SIZE), 0, "k_mem_page_out failed");
}

ZTEST_USER(demand_paging_user_fault, test_user_touch_after_page_out)
{
	/* read of an evicted page must page it back in, contents intact */
	zassert_equal(user_page[0], 0x5a, "paged-out contents lost, got 0x%hhx", user_page[0]);

	/* the paged-in page must be writable */
	user_page[1] = 0xa5;
	zassert_equal(user_page[1], (char)0xa5, "write to paged-in page lost");

	/* restore for potential re-runs */
	user_page[0] = 0;
	user_page[1] = 0;
}

ZTEST_SUITE(demand_paging_user_fault, NULL, NULL, user_fault_before, NULL, NULL);
