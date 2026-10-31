/*
 * Copyright (c) 2026 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/kernel/mm.h>
#include <zephyr/kernel/mm/demand_paging.h>
#include <string.h>

#include <mmu.h>

#define PARTITION_MARK 0xaa

static struct k_mem_domain paging_domain;
static struct k_mem_partition paging_partition;
static char *partition_page;

K_THREAD_STACK_DEFINE(domain_stack, 1024);
static struct k_thread domain_thread;

static volatile uint8_t domain_read;
static volatile unsigned long domain_faults;

static void domain_thread_entry(void *p1, void *p2, void *p3)
{
	unsigned long faults = k_mem_num_pagefaults_get();

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	domain_read = *(volatile uint8_t *)partition_page;
	domain_faults = k_mem_num_pagefaults_get() - faults;
}

/**
 * @brief Test a memory domain partition added over paged out memory
 *
 * @details Evict a page, then add a partition covering the evicted range
 * and read it from a thread in the domain. The read must fault, page the
 * range back in and return what was written before the eviction. A domain
 * mapping built from the virtual address alone would point at an unrelated
 * frame and answer without faulting at all.
 *
 * @ingroup kernel_memprotect_tests
 */
ZTEST(demand_paging_domain, test_partition_over_paged_out_page)
{
	k_tid_t tid;
	int ret;

	partition_page = k_mem_map(CONFIG_MMU_PAGE_SIZE, K_MEM_PERM_RW);
	zassert_not_null(partition_page, "cannot map a page");
	memset(partition_page, PARTITION_MARK, CONFIG_MMU_PAGE_SIZE);

	ret = k_mem_page_out(partition_page, CONFIG_MMU_PAGE_SIZE);
	zassert_ok(ret, "k_mem_page_out() failed with %d", ret);

	paging_partition.start = (uintptr_t)partition_page;
	paging_partition.size = CONFIG_MMU_PAGE_SIZE;
	paging_partition.attr = K_MEM_PARTITION_P_RW_U_RW;

	zassert_ok(k_mem_domain_init(&paging_domain, 0, NULL));
	zassert_ok(k_mem_domain_add_partition(&paging_domain, &paging_partition));

	tid = k_thread_create(&domain_thread, domain_stack,
			      K_THREAD_STACK_SIZEOF(domain_stack),
			      domain_thread_entry, NULL, NULL, NULL,
			      K_HIGHEST_APPLICATION_THREAD_PRIO, 0, K_FOREVER);
	zassert_ok(k_mem_domain_add_thread(&paging_domain, tid));
	k_thread_start(tid);
	zassert_ok(k_thread_join(tid, K_SECONDS(1)), "domain thread did not finish");

	zassert_equal(domain_read, PARTITION_MARK,
		      "read 0x%02x through the partition, expected 0x%02x",
		      domain_read, PARTITION_MARK);
	zassert_true(domain_faults >= 1,
		     "no page fault taken, the partition mapping bypassed demand paging");

	k_mem_unmap(partition_page, CONFIG_MMU_PAGE_SIZE);
}

ZTEST_SUITE(demand_paging_domain, NULL, NULL, NULL, NULL, NULL);
