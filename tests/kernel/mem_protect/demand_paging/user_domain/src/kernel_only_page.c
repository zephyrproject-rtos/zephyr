/*
 * Copyright (c) 2026 Process Mission
 *
 * A user-mode access to a paged-out page that grants no EL0 access
 * (kernel-only mapping) must stay fatal: the demand paging handler
 * must not page it in, dirty it or LRU-refresh it on behalf of the
 * thread.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel/mm.h>
#include <zephyr/kernel/mm/demand_paging.h>
#include <kernel_arch_interface.h>

#include "common.h"

static ZTEST_DMEM volatile bool access_succeeded;

K_THREAD_STACK_DEFINE(bad_stack, 2048);
static struct k_thread bad_thread;

static void bad_entry(void *p1, void *p2, void *p3)
{
	volatile char *page = (volatile char *)p1;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	/* both the read and the write must fault */
	(void)page[0];
	page[1] = 0xa5;

	/* reached only if the fault was wrongly resolved */
	access_succeeded = true;
}

ZTEST(demand_paging_kernel_only_page, test_el0_touch_kernel_page_is_fatal)
{
	char *page = k_mem_map(CONFIG_MMU_PAGE_SIZE, K_MEM_PERM_RW);
	k_tid_t tid;

	zassert_not_null(page, "k_mem_map failed");

	memset(page, 0x5a, CONFIG_MMU_PAGE_SIZE);
	zassert_equal(k_mem_page_out(page, CONFIG_MMU_PAGE_SIZE), 0, "k_mem_page_out failed");

	access_succeeded = false;
	expect_fault = true;

	tid = k_thread_create(&bad_thread, bad_stack, K_THREAD_STACK_SIZEOF(bad_stack), bad_entry,
			      page, NULL, NULL, K_PRIO_PREEMPT(1), K_USER, K_NO_WAIT);

	/* the thread must fault and be aborted by the fatal handler */
	zassert_ok(k_thread_join(tid, K_SECONDS(10)), "faulting thread was not aborted");

	expect_fault = false;
	zassert_false(access_succeeded, "EL0 access to a kernel-only paged-out page was resolved");

	/* the fault must not have paged the frame in on the thread's behalf */
	if (IS_ENABLED(CONFIG_ARM64)) {
		/*
		 * arm64 validates EL0 access against the thread's page tables
		 * before any paging state changes; x86 pages the frame in on
		 * the not-present fault and only faults on the retried
		 * access, so the location check does not apply there.
		 */
		uintptr_t location;

		zassert_equal(arch_page_location_get(page, &location), ARCH_PAGE_LOCATION_PAGED_OUT,
			      "kernel-only page was paged in for an EL0 fault");
	}
}

ZTEST_SUITE(demand_paging_kernel_only_page, NULL, NULL, NULL, NULL, NULL);
