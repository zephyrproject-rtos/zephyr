/*
 * Copyright (c) 2024 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel/mm.h>
#include <zephyr/kernel/mm/demand_paging.h>
#include <zephyr/linker/sections.h>
#include <mmu.h>

static const char __ondemand_rodata message[] = "was evicted";

static void __ondemand_func evictable_function(void)
{
	static int count;

	printk("This %s code, count=%d\n", message, ++count);
}

/**
 * @brief Verify that code in an on-demand section is paged in and evictable.
 *
 * @ingroup kernel_memprotect_tests
 *
 * @details
 * Code and data placed in an on-demand linker section are not resident at
 * boot: the first access has to fault them in, they stay resident until
 * evicted, and they can be evicted and fetched back explicitly. The kernel's
 * page fault counter is what makes each transition observable, since it is
 * sampled either side of every call to the evictable function rather than
 * inferred from the call succeeding.
 *
 * Test steps:
 * - Call the on-demand function and confirm the fault count rose.
 * - Call it again and confirm the count did not move, so it stayed resident.
 * - Evict its page with k_mem_page_out() and call it again, expecting a fault.
 * - Evict it once more, then fetch it back with k_mem_page_in().
 * - Call it a final time and confirm no fault was taken.
 *
 * Expected result:
 * - The first call and the call after each eviction fault; the call while
 *   resident and the call after an explicit page-in do not.
 *
 * @see k_mem_page_out()
 * @see k_mem_page_in()
 * @see k_mem_num_pagefaults_get()
 */
ZTEST(ondemand_section, test_ondemand_basic)
{
	unsigned long faults_before, faults_after;
	void *addr = (void *)ROUND_DOWN(&evictable_function, CONFIG_MMU_PAGE_SIZE);

	printk("About to call unpaged code\n");
	faults_before = k_mem_num_pagefaults_get();
	evictable_function();
	faults_after = k_mem_num_pagefaults_get();
	zassert_not_equal(faults_before, faults_after, "should have faulted");

	printk("Code should be resident on second call\n");
	faults_before = k_mem_num_pagefaults_get();
	evictable_function();
	faults_after = k_mem_num_pagefaults_get();
	zassert_equal(faults_before, faults_after, "should not have faulted");

	printk("Forcefully evicting it from memory\n");
	zassert_ok(k_mem_page_out(addr, CONFIG_MMU_PAGE_SIZE), "");

	printk("Calling it again\n");
	faults_before = k_mem_num_pagefaults_get();
	evictable_function();
	faults_after = k_mem_num_pagefaults_get();
	zassert_not_equal(faults_before, faults_after, "should have faulted");

	printk("Forcefully evicting it from memory again\n");
	zassert_ok(k_mem_page_out(addr, CONFIG_MMU_PAGE_SIZE), "");

	printk("Preemptively fetching it back in\n");
	/* strangely, k_mem_page_in() returns void */
	k_mem_page_in(addr, CONFIG_MMU_PAGE_SIZE);

	printk("Code should be resident\n");
	faults_before = k_mem_num_pagefaults_get();
	evictable_function();
	faults_after = k_mem_num_pagefaults_get();
	zassert_equal(faults_before, faults_after, "should not have faulted");
}

ZTEST_SUITE(ondemand_section, NULL, NULL, NULL, NULL, NULL);
