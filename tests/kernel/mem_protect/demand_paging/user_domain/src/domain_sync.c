/*
 * Copyright (c) 2026 Process Mission
 *
 * Demand paging must preserve domain-private partition mappings: a
 * partition added to a memory domain with user permissions must remain
 * user-accessible after the kernel pages its backing frames out and
 * back in.
 *
 * Boot image pages are pinned at boot (kernel/mmu.c: z_mem_manage_init()),
 * so the partition is placed over a page-aligned static buffer that
 * k_mem_unpin() makes evictable.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/app_memory/mem_domain.h>
#include <zephyr/kernel/mm.h>
#include <zephyr/kernel/mm/demand_paging.h>
#include <kernel_arch_interface.h>

#include "common.h"

static char part_page[CONFIG_MMU_PAGE_SIZE] __aligned(CONFIG_MMU_PAGE_SIZE);
K_MEM_PARTITION_DEFINE(test_part, part_page, sizeof(part_page), K_MEM_PARTITION_P_RW_U_RW);

static void domain_sync_before(void *data)
{
	struct k_mem_domain *domain = k_current_get()->mem_domain_info.mem_domain;

	ARG_UNUSED(data);

	zassert_not_null(domain, "test thread has no memory domain");

	/* write a marker so we can verify the contents survive the cycle */
	part_page[0] = 0x5a;

	/* hand the page to the test domain as a private U_RW partition */
	zassert_equal(k_mem_domain_add_partition(domain, &test_part), 0,
		      "k_mem_domain_add_partition failed");

	/* boot pages are pinned; make this one evictable */
	k_mem_unpin(part_page, CONFIG_MMU_PAGE_SIZE);

	/* simulate an eviction + refill cycle, as demand paging would do */
	zassert_equal(k_mem_page_out(part_page, CONFIG_MMU_PAGE_SIZE), 0, "k_mem_page_out failed");
	k_mem_page_in(part_page, CONFIG_MMU_PAGE_SIZE);
}

ZTEST_USER(demand_paging_domain_sync, test_partition_perms_after_page_out_in)
{
	/* the private U_RW mapping must still be there */
	zassert_equal(part_page[0], 0x5a, "partition contents lost, got 0x%hhx", part_page[0]);

	part_page[1] = 0xa5;
	zassert_equal(part_page[1], (char)0xa5, "partition no longer writable");

	part_page[0] = 0;
	part_page[1] = 0;
}

ZTEST_SUITE(demand_paging_domain_sync, NULL, NULL, domain_sync_before, NULL, NULL);

static char ro_part_page[CONFIG_MMU_PAGE_SIZE] __aligned(CONFIG_MMU_PAGE_SIZE);
K_MEM_PARTITION_DEFINE(ro_test_part, ro_part_page, sizeof(ro_part_page),
		       K_MEM_PARTITION_P_RO_U_RO);
K_THREAD_STACK_DEFINE(ro_bad_stack, 2048);
static struct k_thread ro_bad_thread;
static volatile bool ro_write_succeeded;

static void ro_bad_entry(void *p1, void *p2, void *p3)
{
	volatile char *page = p1;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	(void)page[0];
	page[1] = 0xa5;
	ro_write_succeeded = true;
}

static void read_only_before(void *data)
{
	struct k_mem_domain *domain = k_current_get()->mem_domain_info.mem_domain;
	static bool ro_part_initialized;

	ARG_UNUSED(data);

	if (!ro_part_initialized) {
		ro_part_page[0] = 0x5a;
		zassert_equal(k_mem_domain_add_partition(domain, &ro_test_part), 0,
			      "k_mem_domain_add_partition failed");

		k_mem_unpin(ro_part_page, CONFIG_MMU_PAGE_SIZE);
		ro_part_initialized = true;
	}
	zassert_equal(k_mem_page_out(ro_part_page, CONFIG_MMU_PAGE_SIZE), 0,
		      "k_mem_page_out failed");
	k_mem_page_in(ro_part_page, CONFIG_MMU_PAGE_SIZE);
}

ZTEST(demand_paging_read_only, test_user_write_to_read_only_partition_is_fatal)
{
	struct k_mem_domain *domain = k_current_get()->mem_domain_info.mem_domain;
	int join_ret;

	zassert_not_null(domain, "test thread has no memory domain");
	k_thread_create(&ro_bad_thread, ro_bad_stack, K_THREAD_STACK_SIZEOF(ro_bad_stack),
			ro_bad_entry, ro_part_page, NULL, NULL, K_PRIO_PREEMPT(1), K_USER,
			K_FOREVER);
	zassert_equal(k_mem_domain_add_thread(domain, &ro_bad_thread), 0,
		      "k_mem_domain_add_thread failed");

	ro_write_succeeded = false;
	expect_fault = true;
	k_thread_start(&ro_bad_thread);
	join_ret = k_thread_join(&ro_bad_thread, K_SECONDS(10));
	expect_fault = false;
	if (join_ret != 0) {
		k_thread_abort(&ro_bad_thread);
		(void)k_thread_join(&ro_bad_thread, K_SECONDS(10));
	}
	zassert_ok(join_ret, "faulting thread was not aborted");
	zassert_false(ro_write_succeeded, "read-only partition accepted a user write");

#if defined(CONFIG_ARM64)
	zassert_false(arch_page_info_get(ro_part_page, NULL, false) & ARCH_DATA_PAGE_DIRTY,
		      "read-only partition write marked the page dirty");
#endif
}

#if defined(CONFIG_ARM64)
static void stale_syscall_oops_entry(void *p1, void *p2, void *p3)
{
	volatile char *page = p1;
	struct arch_esf dummy_esf = {0};

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	/*
	 * Trigger a valid EL1 demand page fault first so ESR_EL1 and FAR_EL1
	 * retain that resolved abort when arch_syscall_oops() runs.
	 */
	(void)page[0];
	arch_syscall_oops(&dummy_esf);
}
#endif

ZTEST(demand_paging_read_only, test_syscall_oops_with_stale_paging_regs)
{
#if defined(CONFIG_ARM64)
	char *oops_page = k_mem_map(CONFIG_MMU_PAGE_SIZE, K_MEM_PERM_RW);
	int join_ret;

	zassert_not_null(oops_page, "k_mem_map failed");
	oops_page[0] = 0x5a;
	zassert_equal(k_mem_page_out(oops_page, CONFIG_MMU_PAGE_SIZE), 0, "k_mem_page_out failed");

	k_thread_create(&ro_bad_thread, ro_bad_stack, K_THREAD_STACK_SIZEOF(ro_bad_stack),
			stale_syscall_oops_entry, oops_page, NULL, NULL, K_PRIO_PREEMPT(1), 0U,
			K_FOREVER);

	last_fault_reason = UINT_MAX;
	expect_fault = true;
	k_thread_start(&ro_bad_thread);
	join_ret = k_thread_join(&ro_bad_thread, K_SECONDS(10));
	expect_fault = false;
	if (join_ret != 0) {
		k_thread_abort(&ro_bad_thread);
		(void)k_thread_join(&ro_bad_thread, K_SECONDS(10));
	}

	k_mem_unmap(oops_page, CONFIG_MMU_PAGE_SIZE);

	zassert_ok(join_ret, "arch_syscall_oops thread was not aborted");
	zassert_equal(last_fault_reason, K_ERR_KERNEL_OOPS, "expected K_ERR_KERNEL_OOPS, got %u",
		      last_fault_reason);
#else
	ztest_test_skip();
#endif
}

ZTEST_SUITE(demand_paging_read_only, NULL, NULL, read_only_before, NULL, NULL);
