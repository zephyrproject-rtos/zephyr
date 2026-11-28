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

#if defined(CONFIG_ARM64) && defined(CONFIG_USERSPACE)
	if (p2 != NULL) {
		/*
		 * Simulate the cross-CPU domain-migration window after
		 * arch_mem_domain_thread_add() updates thread->arch.ptables
		 * before z_arm64_mem_cfg_ipi() switches hardware TTBR0_EL1.
		 */
		k_current_get()->arch.ptables = p2;
	}
#else
	ARG_UNUSED(p2);
#endif
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

static void run_ro_write_fault_test(uint32_t thread_options, void *stale_ptables)
{
	struct k_mem_domain *domain = k_current_get()->mem_domain_info.mem_domain;
	int join_ret;

	zassert_not_null(domain, "test thread has no memory domain");
	k_thread_create(&ro_bad_thread, ro_bad_stack, K_THREAD_STACK_SIZEOF(ro_bad_stack),
			ro_bad_entry, ro_part_page, stale_ptables, NULL, K_PRIO_PREEMPT(1),
			thread_options, K_FOREVER);
	zassert_equal(k_mem_domain_add_thread(domain, &ro_bad_thread), 0,
		      "k_mem_domain_add_thread failed");

#if defined(CONFIG_ARM64)
	last_fault_reason = UINT_MAX;
	last_fault_spsr = 0ULL;
	last_fault_far = 0ULL;
#endif
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
	zassert_false(ro_write_succeeded, "read-only partition accepted a write");

#if defined(CONFIG_ARM64)
	zassert_equal(last_fault_reason, K_ERR_CPU_EXCEPTION, "unexpected fatal reason %u",
		      last_fault_reason);
	if ((thread_options & K_USER) != 0U) {
		zassert_equal(last_fault_spsr & SPSR_MODE_MASK, SPSR_MODE_EL0T,
			      "expected EL0 fault, got SPSR %#llx",
			      (unsigned long long)last_fault_spsr);
	} else {
		zassert_not_equal(last_fault_spsr & SPSR_MODE_MASK, SPSR_MODE_EL0T,
				  "expected EL1 fault, got SPSR %#llx",
				  (unsigned long long)last_fault_spsr);
	}
	zassert_equal(last_fault_far, (uint64_t)(uintptr_t)&ro_part_page[1],
		      "unexpected fault FAR %#llx (expected write at %p)",
		      (unsigned long long)last_fault_far, (void *)&ro_part_page[1]);
	zassert_false(arch_page_info_get(ro_part_page, NULL, false) & ARCH_DATA_PAGE_DIRTY,
		      "read-only partition write marked the page dirty");
#endif
}

ZTEST(demand_paging_read_only, test_user_write_to_read_only_partition_is_fatal)
{
	run_ro_write_fault_test(K_USER, NULL);
}

ZTEST(demand_paging_read_only, test_el1_write_to_read_only_partition_is_fatal)
{
	if (!IS_ENABLED(CONFIG_ARM64)) {
		/* x86 supervisor mode does not trap ring-0 writes on P_RO_U_RO partitions. */
		ztest_test_skip();
	}

	run_ro_write_fault_test(0U, NULL);
}

ZTEST(demand_paging_read_only, test_el1_write_uses_hw_ttbr0_over_stale_ptables)
{
#if defined(CONFIG_ARM64) && defined(CONFIG_USERSPACE)
	static struct k_mem_domain alt_domain;

	zassert_ok(k_mem_domain_init(&alt_domain, 0U, NULL), "k_mem_domain_init failed");
	run_ro_write_fault_test(0U, &alt_domain.arch.ptables);
	zassert_ok(k_mem_domain_deinit(&alt_domain), "k_mem_domain_deinit failed");
#else
	ztest_test_skip();
#endif
}

#if defined(CONFIG_ARM64) && defined(CONFIG_USERSPACE)
static char hw_rw_part_page[CONFIG_MMU_PAGE_SIZE] __aligned(CONFIG_MMU_PAGE_SIZE);
K_MEM_PARTITION_DEFINE(hw_rw_part, hw_rw_part_page, sizeof(hw_rw_part_page),
		       K_MEM_PARTITION_P_RW_U_RW);
K_MEM_PARTITION_DEFINE(hw_ro_stale_part, hw_rw_part_page, sizeof(hw_rw_part_page),
		       K_MEM_PARTITION_P_RO_U_RO);

static void hw_ttbr0_recover_entry(void *p1, void *p2, void *p3)
{
	volatile char *page = p1;
	void *orig_ptables = k_current_get()->arch.ptables;

	ARG_UNUSED(p3);

	k_current_get()->arch.ptables = p2;
	page[1] = 0x5c;
	k_current_get()->arch.ptables = orig_ptables;
	ro_write_succeeded = true;
}
#endif

ZTEST(demand_paging_read_only, test_el1_write_recovers_with_hw_ttbr0_over_ro_ptables)
{
#if defined(CONFIG_ARM64) && defined(CONFIG_USERSPACE)
	static struct k_mem_domain rw_domain;
	static struct k_mem_domain ro_stale_domain;
	struct k_mem_partition *rw_ptn = &hw_rw_part;
	struct k_mem_partition *ro_ptn = &hw_ro_stale_part;
	int join_ret;

	zassert_ok(k_mem_domain_init(&rw_domain, 1U, &rw_ptn), "rw k_mem_domain_init failed");
	zassert_ok(k_mem_domain_init(&ro_stale_domain, 1U, &ro_ptn), "ro k_mem_domain_init failed");

	hw_rw_part_page[0] = 0x5a;
	hw_rw_part_page[1] = 0;
	k_mem_unpin(hw_rw_part_page, CONFIG_MMU_PAGE_SIZE);
	zassert_equal(k_mem_page_out(hw_rw_part_page, CONFIG_MMU_PAGE_SIZE), 0,
		      "k_mem_page_out failed");
	k_mem_page_in(hw_rw_part_page, CONFIG_MMU_PAGE_SIZE);

	k_thread_create(&ro_bad_thread, ro_bad_stack, K_THREAD_STACK_SIZEOF(ro_bad_stack),
			hw_ttbr0_recover_entry, hw_rw_part_page, &ro_stale_domain.arch.ptables,
			NULL, K_PRIO_COOP(0), 0U, K_FOREVER);
	zassert_equal(k_mem_domain_add_thread(&rw_domain, &ro_bad_thread), 0,
		      "k_mem_domain_add_thread failed");

	last_fault_reason = UINT_MAX;
	ro_write_succeeded = false;
	expect_fault = false;
	k_thread_start(&ro_bad_thread);
	join_ret = k_thread_join(&ro_bad_thread, K_SECONDS(10));
	if (join_ret != 0) {
		k_thread_abort(&ro_bad_thread);
		(void)k_thread_join(&ro_bad_thread, K_SECONDS(10));
	}

	zassert_ok(k_mem_domain_deinit(&rw_domain), "rw k_mem_domain_deinit failed");
	zassert_ok(k_mem_domain_deinit(&ro_stale_domain), "ro k_mem_domain_deinit failed");

	zassert_ok(join_ret, "EL1 write recovery thread failed to join");
	zassert_true(ro_write_succeeded, "EL1 write did not recover via active hardware TTBR0_EL1");
	zassert_equal(last_fault_reason, UINT_MAX, "unexpected fatal fault (%u)",
		      last_fault_reason);
	zassert_equal(hw_rw_part_page[1], 0x5c, "written byte lost (%#x)",
		      (unsigned int)(uint8_t)hw_rw_part_page[1]);
	zassert_true((arch_page_info_get(hw_rw_part_page, NULL, false) & ARCH_DATA_PAGE_DIRTY) !=
			     0U,
		     "recovered EL1 write did not mark the page dirty");
#else
	ztest_test_skip();
#endif
}

#if defined(CONFIG_ARM64)
static void sttr_bad_entry(void *p1, void *p2, void *p3)
{
	volatile uint32_t *target = p1;
	uint32_t val = 0xa5U;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	__asm__ volatile("sttr %w0, [%1]" : : "r"(val), "r"(target) : "memory");
	ro_write_succeeded = true;
}

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

ZTEST(demand_paging_read_only, test_el1_sttr_to_kernel_only_page_is_fatal)
{
#if defined(CONFIG_ARM64)
	char *sttr_page = k_mem_map(CONFIG_MMU_PAGE_SIZE, K_MEM_PERM_RW);
	int join_ret;
	uint32_t written;

	zassert_not_null(sttr_page, "k_mem_map failed");
	sttr_page[0] = 0x5a;
	*(volatile uint32_t *)&sttr_page[4] = 0U;

	k_thread_create(&ro_bad_thread, ro_bad_stack, K_THREAD_STACK_SIZEOF(ro_bad_stack),
			sttr_bad_entry, &sttr_page[4], NULL, NULL, K_PRIO_PREEMPT(1), 0U,
			K_FOREVER);

	last_fault_reason = UINT_MAX;
	last_fault_spsr = 0ULL;
	last_fault_far = 0ULL;
	ro_write_succeeded = false;
	expect_fault = true;
	k_thread_start(&ro_bad_thread);
	join_ret = k_thread_join(&ro_bad_thread, K_SECONDS(10));
	expect_fault = false;
	if (join_ret != 0) {
		k_thread_abort(&ro_bad_thread);
		(void)k_thread_join(&ro_bad_thread, K_SECONDS(10));
	}

	written = *(volatile uint32_t *)&sttr_page[4];
	k_mem_unmap(sttr_page, CONFIG_MMU_PAGE_SIZE);

	zassert_ok(join_ret, "STTR faulting thread was not aborted");
	zassert_false(ro_write_succeeded, "STTR to EL1-only resident page succeeded");
	zassert_equal(written, 0U, "STTR modified target word");
	zassert_equal(last_fault_reason, K_ERR_CPU_EXCEPTION, "unexpected fatal reason %u",
		      last_fault_reason);
	zassert_not_equal(last_fault_spsr & SPSR_MODE_MASK, SPSR_MODE_EL0T,
			  "expected EL1 fault, got SPSR %#llx",
			  (unsigned long long)last_fault_spsr);
	zassert_equal(last_fault_far, (uint64_t)(uintptr_t)&sttr_page[4],
		      "unexpected STTR fault FAR %#llx", (unsigned long long)last_fault_far);
#else
	ztest_test_skip();
#endif
}

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
