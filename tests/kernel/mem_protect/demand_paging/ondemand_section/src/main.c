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
