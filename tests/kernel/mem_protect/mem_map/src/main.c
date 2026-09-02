/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/toolchain.h>
#include <mmu.h>
#include <zephyr/linker/sections.h>
#include <zephyr/cache.h>

#ifdef CONFIG_DEMAND_PAGING
#include <zephyr/kernel/mm/demand_paging.h>
#endif /* CONFIG_DEMAND_PAGING */

/* 32-bit IA32 page tables have no mechanism to restrict execution */
#if defined(CONFIG_X86) && !defined(CONFIG_X86_64) && !defined(CONFIG_X86_PAE)
#define SKIP_EXECUTE_TESTS
#endif

#define BASE_FLAGS	(K_MEM_CACHE_WB)
volatile bool expect_fault;

/* k_mem_map_phys_bare() doesn't have alignment requirements, any oddly-sized buffer
 * can get mapped. BUF_SIZE has a odd size to make sure the mapped buffer
 * spans multiple pages.
 */
#define BUF_SIZE	(CONFIG_MMU_PAGE_SIZE + 907)
#define BUF_OFFSET	1238

#define TEST_PAGE_SZ	ROUND_UP(BUF_OFFSET + BUF_SIZE, CONFIG_MMU_PAGE_SIZE)

static uint8_t __aligned(CONFIG_MMU_PAGE_SIZE) test_page[TEST_PAGE_SZ];

void k_sys_fatal_error_handler(unsigned int reason, const struct arch_esf *pEsf)
{
	printk("Caught system error -- reason %d\n", reason);

	if (expect_fault && reason == 0) {
		expect_fault = false;
		ztest_test_pass();
	} else {
		printk("Unexpected fault during test\n");
		TC_END_REPORT(TC_FAIL);
		k_fatal_halt(reason);
	}
}

/**
 * @brief Verify that k_mem_map_phys_bare() honours size, aliasing and the RW
 *        flag.
 *
 * @ingroup kernel_memprotect_tests
 *
 * @details
 * Mapping the same unaligned physical buffer twice -- once writable, once
 * read-only -- must produce two virtual aliases of the same memory: bytes
 * written through the writable alias have to appear in the backing buffer and
 * through the read-only alias, and a write through the read-only alias has to
 * fault. The suite's fatal error handler passes the case when the expected
 * fault arrives, so falling through the write is the failure.
 *
 * Test steps:
 * - Map an unaligned buffer with K_MEM_PERM_RW and again with read-only
 *   permissions.
 * - Fill the writable alias and compare the backing buffer and the read-only
 *   alias against what was written, with cache flushes where configured.
 * - Write through the read-only alias.
 *
 * Expected result:
 * - Both aliases show the written data and the read-only write faults.
 *
 * @see k_mem_map_phys_bare()
 */
ZTEST(mem_map, test_k_mem_map_phys_bare_rw)
{
	uint8_t *mapped_rw, *mapped_ro;
	uint8_t *buf = test_page + BUF_OFFSET;
	uintptr_t aligned_addr;
	size_t aligned_size;
	size_t aligned_offset;

	expect_fault = false;

	if (IS_ENABLED(CONFIG_DCACHE)) {
		/* Flush everything and invalidating all addresses to
		 * prepare fot comparison test below.
		 */
		sys_cache_data_flush_and_invd_all();
	}

	/* Map in a page that allows writes */
	k_mem_map_phys_bare(&mapped_rw, k_mem_phys_addr(buf),
			    BUF_SIZE, BASE_FLAGS | K_MEM_PERM_RW);

	/* Map again this time only allowing reads */
	k_mem_map_phys_bare(&mapped_ro, k_mem_phys_addr(buf),
			    BUF_SIZE, BASE_FLAGS);

	/* Initialize read-write buf with some bytes */
	for (int i = 0; i < BUF_SIZE; i++) {
		mapped_rw[i] = (uint8_t)(i % 256);
	}

	if (IS_ENABLED(CONFIG_DCACHE)) {
		/* Flush the data to memory after write. */
		aligned_offset =
			k_mem_region_align(&aligned_addr, &aligned_size, (uintptr_t)mapped_rw,
					   BUF_SIZE, CONFIG_MMU_PAGE_SIZE);
		zassert_equal(aligned_offset, BUF_OFFSET,
			      "unexpected mapped_rw aligned offset: %zu != %u", aligned_offset,
			      BUF_OFFSET);
		sys_cache_data_flush_and_invd_range((void *)aligned_addr, aligned_size);
	}

	/* Check that the backing buffer contains the expected data. */
	for (int i = 0; i < BUF_SIZE; i++) {
		uint8_t expected_val = (uint8_t)(i % 256);

		zassert_equal(expected_val, buf[i],
			      "unexpected byte at buffer index %d (%u != %u)",
			      i, expected_val, buf[i]);

		zassert_equal(buf[i], mapped_rw[i],
			      "unequal byte at RW index %d (%u != %u)",
			      i, buf[i], mapped_rw[i]);
	}

	/* Check that the read-only mapped area contains the expected data. */
	for (int i = 0; i < BUF_SIZE; i++) {
		uint8_t expected_val = (uint8_t)(i % 256);

		zassert_equal(expected_val, mapped_ro[i],
			      "unexpected byte at RO index %d (%u != %u)",
			      i, expected_val, mapped_ro[i]);

		zassert_equal(buf[i], mapped_ro[i],
			      "unequal byte at RO index %d (%u != %u)",
			      i, buf[i], mapped_ro[i]);
	}

	/* This should explode since writes are forbidden */
	expect_fault = true;
	mapped_ro[0] = 42;

	printk("shouldn't get here\n");
	ztest_test_fail();
}

#ifndef SKIP_EXECUTE_TESTS
extern char __test_mem_map_start[];
extern char __test_mem_map_end[];

__in_section_unique(test_mem_map) __used
static void transplanted_function(bool *executed)
{
	*executed = true;
}
#endif

/**
 * @brief Verify that K_MEM_PERM_EXEC controls execution from a mapping.
 *
 * @ingroup kernel_memprotect_tests
 *
 * @details
 * The same function body is mapped twice: with execute permission, calling
 * into the mapping must work, and without it, the call must fault. This is
 * the mapping-level half of execute protection -- the region is the same, so
 * only the permission decides. Skipped where the platform cannot fault
 * execution.
 *
 * Test steps:
 * - Map the page holding a transplanted function with K_MEM_PERM_EXEC and
 *   call it through the mapping.
 * - Map the same page without execute permission and call it again.
 *
 * Expected result:
 * - The executable mapping runs the function; the non-executable one faults.
 *
 * @see k_mem_map_phys_bare()
 */
ZTEST(mem_map, test_k_mem_map_phys_bare_exec)
{
#ifndef SKIP_EXECUTE_TESTS
	uint8_t *mapped_exec, *mapped_ro;
	bool executed = false;
	void (*func)(bool *executed);

	expect_fault = false;

	/*
	 * Need to reference the function or else linker would
	 * garbage collected it.
	 */
	func = transplanted_function;

	/* Now map with execution enabled and try to run the copied fn */
	k_mem_map_phys_bare(&mapped_exec, k_mem_phys_addr(__test_mem_map_start),
			    (uintptr_t)(__test_mem_map_end - __test_mem_map_start),
			    BASE_FLAGS | K_MEM_PERM_EXEC);

	func = (void (*)(bool *executed))mapped_exec;
	func(&executed);
	zassert_true(executed, "function did not execute");

	/* Now map without execution and execution should now fail */
	k_mem_map_phys_bare(&mapped_ro, k_mem_phys_addr(__test_mem_map_start),
			    (uintptr_t)(__test_mem_map_end - __test_mem_map_start),
			    BASE_FLAGS);

	func = (void (*)(bool *executed))mapped_ro;
	expect_fault = true;
	func(&executed);

	printk("shouldn't get here\n");
	ztest_test_fail();
#else
	ztest_test_skip();
#endif /* SKIP_EXECUTE_TESTS */
}

/**
 * @brief Verify that a new mapping leaves existing memory untouched.
 *
 * @ingroup kernel_memprotect_tests
 *
 * @details
 * Creating a mapping must not disturb anything already in memory: the data of
 * this test, the page being mapped, and the kernel's own state have to be
 * exactly as they were. The test seeds a buffer, maps it, and checks the
 * contents through the original address rather than the new mapping.
 *
 * Test steps:
 * - Fill the test page with a known pattern.
 * - Map it read-only with k_mem_map_phys_bare().
 * - Compare the buffer through its original address against the pattern.
 *
 * Expected result:
 * - The buffer is unchanged by the act of mapping it.
 *
 * @see k_mem_map_phys_bare()
 */
ZTEST(mem_map, test_k_mem_map_phys_bare_side_effect)
{
	uint8_t *mapped;

	expect_fault = false;

	/* k_mem_map_phys_bare() is supposed to always create fresh mappings.
	 * Show that by mapping test_page to an RO region, we can still
	 * modify test_page.
	 */
	k_mem_map_phys_bare(&mapped, k_mem_phys_addr(test_page),
			    sizeof(test_page), BASE_FLAGS);

	/* Should NOT fault */
	test_page[0] = 42;

	/* Should fault */
	expect_fault = true;
	mapped[0] = 42;
	printk("shouldn't get here\n");
	ztest_test_fail();
}

/**
 * @brief Verify that k_mem_unmap_phys_bare() revokes access to the mapping.
 *
 * @ingroup kernel_memprotect_tests
 *
 * @details
 * After an unmap the virtual address must no longer reach the memory: a read
 * through it has to fault rather than return stale data. The mapping is first
 * exercised to show it worked, so the later fault can only come from the
 * unmap.
 *
 * Test steps:
 * - Map the test page read-only and read through the mapping.
 * - Unmap it with k_mem_unmap_phys_bare().
 * - Read through the now-stale virtual address.
 *
 * Expected result:
 * - The read after the unmap faults; the code after it is never reached.
 *
 * @see k_mem_unmap_phys_bare()
 */
ZTEST(mem_map, test_k_mem_unmap_phys_bare)
{
	uint8_t *mapped;

	expect_fault = false;

	/* Map in a page that allows writes */
	k_mem_map_phys_bare(&mapped, k_mem_phys_addr(test_page),
			    sizeof(test_page), BASE_FLAGS | K_MEM_PERM_RW);

	/* Should NOT fault */
	mapped[0] = 42;

	/* Unmap the memory */
	k_mem_unmap_phys_bare(mapped, sizeof(test_page));

	/* Should fault since test_page is no longer accessible */
	expect_fault = true;
	mapped[0] = 42;
	printk("shouldn't get here\n");
	ztest_test_fail();
}

/**
 * @brief Verify that an unmapped virtual region can be mapped again.
 *
 * @ingroup kernel_memprotect_tests
 *
 * @details
 * Unmapping has to return the virtual region to the allocator, otherwise
 * repeated map/unmap cycles leak address space. Mapping, unmapping and
 * mapping again must eventually hand back the same virtual address, which is
 * what shows the region was reclaimed rather than abandoned.
 *
 * Test steps:
 * - Map the test page and record the virtual address.
 * - Unmap it, then map the same page repeatedly, unmapping after each try,
 *   until the recorded address comes back or an attempt limit is reached.
 *
 * Expected result:
 * - The originally returned virtual address is handed out again, showing the
 *   unmap reclaimed the region.
 *
 * @see k_mem_map_phys_bare()
 * @see k_mem_unmap_phys_bare()
 */
ZTEST(mem_map, test_k_mem_map_phys_bare_unmap_reclaim_addr)
{
	uint8_t *mapped, *mapped_old;
	uint8_t *buf = test_page + BUF_OFFSET;

	/* Map the buffer the first time. */
	k_mem_map_phys_bare(&mapped, k_mem_phys_addr(buf),
			    BUF_SIZE, BASE_FLAGS);

	printk("Mapped (1st time): %p\n", mapped);

	/* Store the pointer for later comparison. */
	mapped_old = mapped;

	/*
	 * Unmap the buffer.
	 * This should reclaim the bits in virtual region tracking,
	 * so that the next time k_mem_map_phys_bare() is called with
	 * the same arguments, it will return the same address.
	 */
	k_mem_unmap_phys_bare(mapped, BUF_SIZE);

	/*
	 * Map again the same buffer using same parameters.
	 * It should give us back the same virtual address
	 * as above when it is mapped the first time.
	 */
	k_mem_map_phys_bare(&mapped, k_mem_phys_addr(buf), BUF_SIZE, BASE_FLAGS);

	printk("Mapped (2nd time): %p\n", mapped);

	zassert_equal(mapped, mapped_old, "Virtual memory region not reclaimed!");
}

/**
 * @brief Verify that k_mem_map() provides usable anonymous memory and
 *        k_mem_unmap() takes it back.
 *
 * @ingroup kernel_memprotect_tests
 *
 * @details
 * The anonymous mapping API must hand out zeroed, writable pages backed by
 * real frames, and unmapping must release both the frames and the virtual
 * region so a subsequent map can reuse them. Default behaviour only; the
 * K_MEM_MAP_* control flags are not exercised here.
 *
 * Test steps:
 * - Map a page with k_mem_map(), check it reads back as zeroes, and write a
 *   pattern into it.
 * - Check the number of free page frames dropped by the mapping's worth.
 * - Unmap it, check the free frame count is restored, and map again to see
 *   the region reused.
 *
 * Expected result:
 * - The page is zero-filled and writable, and unmapping returns both the
 *   frames and the virtual region.
 *
 * @see k_mem_map()
 * @see k_mem_unmap()
 */
ZTEST(mem_map_api, test_k_mem_map_unmap)
{
	size_t free_mem, free_mem_after_map, free_mem_after_unmap;
	char *mapped, *last_mapped;
	int i, repeat;

	expect_fault = false;
	last_mapped = NULL;

	free_mem = k_mem_free_get();
	zassert_not_equal(free_mem, 0, "no free memory");
	printk("Free memory: %zu\n", free_mem);

	/* Repeat a couple times to make sure everything still works */
	for (repeat = 1; repeat <= 10; repeat++) {
		mapped = k_mem_map(CONFIG_MMU_PAGE_SIZE, K_MEM_PERM_RW);
		zassert_not_null(mapped, "failed to map memory");
		printk("mapped a page to %p\n", mapped);

		if (last_mapped != NULL) {
			zassert_equal(mapped, last_mapped,
				      "should have mapped at same address");
		}
		last_mapped = mapped;

		if (IS_ENABLED(CONFIG_DCACHE)) {
			sys_cache_data_flush_and_invd_range((void *)mapped, CONFIG_MMU_PAGE_SIZE);
		}

		/* Page should be zeroed */
		for (i = 0; i < CONFIG_MMU_PAGE_SIZE; i++) {
			zassert_equal(mapped[i], '\x00', "page not zeroed");
		}

		free_mem_after_map = k_mem_free_get();
		printk("Free memory after mapping: %zu\n", free_mem_after_map);
		zassert_equal(free_mem, free_mem_after_map + CONFIG_MMU_PAGE_SIZE,
			"incorrect free memory accounting");

		/* Show we can write to page without exploding */
		(void)memset(mapped, '\xFF', CONFIG_MMU_PAGE_SIZE);

		if (IS_ENABLED(CONFIG_DCACHE)) {
			sys_cache_data_flush_and_invd_range((void *)mapped, CONFIG_MMU_PAGE_SIZE);
		}

		for (i = 0; i < CONFIG_MMU_PAGE_SIZE; i++) {
			zassert_true(mapped[i] == '\xFF',
				"incorrect value 0x%hhx read at index %d",
				mapped[i], i);
		}

		k_mem_unmap(mapped, CONFIG_MMU_PAGE_SIZE);

		free_mem_after_unmap = k_mem_free_get();
		printk("Free memory after unmapping: %zu\n", free_mem_after_unmap);
		zassert_equal(free_mem, free_mem_after_unmap,
			"k_mem_unmap has not freed physical memory");

		if (repeat == 10) {
			/* Should fault since mapped is no longer accessible */
			expect_fault = true;
			mapped[0] = 42;
			printk("shouldn't get here\n");
			ztest_test_fail();
		}
	}
}

/**
 * @brief Verify that k_mem_map() places a guard page before the mapping.
 *
 * @ingroup kernel_memprotect_tests
 *
 * @details
 * Anonymous mappings are bracketed by unmapped guard pages, so an underrun
 * off the start of the region faults immediately instead of touching a
 * neighbouring mapping. The fatal error handler passes the case when the
 * expected fault arrives.
 *
 * Test steps:
 * - Map a page with k_mem_map() and write to its first byte, which must
 *   succeed.
 * - Write just before the start of the mapping.
 *
 * Expected result:
 * - The in-bounds write succeeds and the underrun faults in the guard page;
 *   the code after it is never reached.
 *
 * @see k_mem_map()
 */
ZTEST(mem_map_api, test_k_mem_map_guard_before)
{
	uint8_t *mapped;

	expect_fault = false;

	mapped = k_mem_map(CONFIG_MMU_PAGE_SIZE, K_MEM_PERM_RW);
	zassert_not_null(mapped, "failed to map memory");
	printk("mapped a page: %p - %p\n", mapped,
		mapped + CONFIG_MMU_PAGE_SIZE);

	/* Should NOT fault */
	mapped[0] = 42;

	/* Should fault here in the guard page location */
	expect_fault = true;
	mapped -= sizeof(void *);

	printk("trying to access %p\n", mapped);

	mapped[0] = 42;
	printk("shouldn't get here\n");
	ztest_test_fail();
}

/**
 * @brief Verify that k_mem_map() places a guard page after the mapping.
 *
 * @ingroup kernel_memprotect_tests
 *
 * @details
 * The counterpart of the "before" case: an overrun off the end of the region
 * must fault in the trailing guard page rather than reach whatever is mapped
 * next.
 *
 * Test steps:
 * - Map a page with k_mem_map() and write to its last byte, which must
 *   succeed.
 * - Write just past the end of the mapping.
 *
 * Expected result:
 * - The in-bounds write succeeds and the overrun faults in the guard page;
 *   the code after it is never reached.
 *
 * @see k_mem_map()
 */
ZTEST(mem_map_api, test_k_mem_map_guard_after)
{
	uint8_t *mapped;

	expect_fault = false;

	mapped = k_mem_map(CONFIG_MMU_PAGE_SIZE, K_MEM_PERM_RW);
	zassert_not_null(mapped, "failed to map memory");
	printk("mapped a page: %p - %p\n", mapped,
		mapped + CONFIG_MMU_PAGE_SIZE);

	/* Should NOT fault */
	mapped[0] = 42;

	/* Should fault here in the guard page location */
	expect_fault = true;
	mapped += CONFIG_MMU_PAGE_SIZE + sizeof(void *);

	printk("trying to access %p\n", mapped);

	mapped[0] = 42;
	printk("shouldn't get here\n");
	ztest_test_fail();
}

/**
 * @brief Verify that k_mem_map() fails cleanly when memory is exhausted.
 *
 * @ingroup kernel_memprotect_tests
 *
 * @details
 * Whichever runs out first -- page frames or virtual address space -- the
 * mapping call has to return NULL rather than fault or hand out a partial
 * region, and everything mapped before the failure must still be released
 * cleanly. The test maps pages until failure, then unmaps them all, which
 * also demonstrates the exhaustion is recoverable.
 *
 * Test steps:
 * - Record the free memory figure, then map pages one at a time until
 *   k_mem_map() returns NULL, writing into each mapped page.
 * - Compare the number of pages obtained and the free memory remaining
 *   against what the initial figure predicted.
 * - Unmap every page and compare the free memory against the starting
 *   figure.
 *
 * Expected result:
 * - Exhaustion is reported as NULL after exactly the predicted number of
 *   pages, and unmapping restores the free memory to its initial value.
 *
 * @see k_mem_map()
 * @see k_mem_unmap()
 */
ZTEST(mem_map_api, test_k_mem_map_exhaustion)
{
	/* With demand paging enabled, there is backing store
	 * which extends available memory. However, we don't
	 * have a way to figure out how much extra memory
	 * is available. So skip for now.
	 */
#if !defined(CONFIG_DEMAND_PAGING)
	uint8_t *addr;
	size_t free_mem, free_mem_now, free_mem_expected;
	size_t cnt, expected_cnt;
	uint8_t *last_mapped = NULL;

	free_mem = k_mem_free_get();
	printk("Free memory: %zu\n", free_mem);
	zassert_not_equal(free_mem, 0, "no free memory");

	/* Determine how many times we can map */
	expected_cnt = free_mem / CONFIG_MMU_PAGE_SIZE;

	/* Figure out how many pages we can map within
	 * the remaining virtual address space by:
	 *
	 * 1. Find out the top of available space. This can be
	 *    done by mapping one page, and use the returned
	 *    virtual address (plus itself and guard page)
	 *    to obtain the end address.
	 * 2. Calculate how big this region is from
	 *    K_MEM_VM_FREE_START to end address.
	 * 3. Calculate how many times we can call k_mem_map().
	 *    Remember there are two guard pages for every
	 *    mapping call (hence 1 + 2 == 3).
	 */
	addr = k_mem_map(CONFIG_MMU_PAGE_SIZE, K_MEM_PERM_RW);
	zassert_not_null(addr, "fail to map memory");
	k_mem_unmap(addr, CONFIG_MMU_PAGE_SIZE);

	cnt = POINTER_TO_UINT(addr) + CONFIG_MMU_PAGE_SIZE * 2;
	cnt -= POINTER_TO_UINT(K_MEM_VM_FREE_START);
	cnt /= CONFIG_MMU_PAGE_SIZE * 3;

	/* If we are limited by virtual address space... */
	if (cnt < expected_cnt) {
		expected_cnt = cnt;
	}

	/* Now k_mem_map() until it fails */
	free_mem_expected = free_mem - (expected_cnt * CONFIG_MMU_PAGE_SIZE);
	cnt = 0;
	do {
		addr = k_mem_map(CONFIG_MMU_PAGE_SIZE, K_MEM_PERM_RW);

		if (addr != NULL) {
			*((uintptr_t *)addr) = POINTER_TO_UINT(last_mapped);
			last_mapped = addr;
			cnt++;
		}
	} while (addr != NULL);

	printk("Mapped %zu pages\n", cnt);
	zassert_equal(cnt, expected_cnt,
		      "number of pages mapped: expected %zu, got %zu",
		      expected_cnt, cnt);

	free_mem_now = k_mem_free_get();
	printk("Free memory now: %zu\n", free_mem_now);
	zassert_equal(free_mem_now, free_mem_expected,
		      "free memory should be %zu", free_mem_expected);

	/* Now free all of them */
	cnt = 0;
	while (last_mapped != NULL) {
		addr = last_mapped;
		last_mapped = UINT_TO_POINTER(*((uintptr_t *)addr));
		k_mem_unmap(addr, CONFIG_MMU_PAGE_SIZE);

		cnt++;
	}

	printk("Unmapped %zu pages\n", cnt);
	zassert_equal(cnt, expected_cnt,
		      "number of pages unmapped: expected %zu, got %zu",
		      expected_cnt, cnt);

	free_mem_now = k_mem_free_get();
	printk("Free memory now: %zu\n", free_mem_now);
	zassert_equal(free_mem_now, free_mem,
		      "free memory should be %zu", free_mem);
#else
	ztest_test_skip();
#endif /* !CONFIG_DEMAND_PAGING */
}

#ifdef CONFIG_USERSPACE
#define USER_STACKSIZE	(128)

struct k_thread user_thread;
K_THREAD_STACK_DEFINE(user_stack, USER_STACKSIZE);

K_APPMEM_PARTITION_DEFINE(default_part);
K_APP_DMEM(default_part) uint8_t *mapped;

static void user_function(void *p1, void *p2, void *p3)
{
	mapped[0] = 42;
}
#endif /* CONFIG_USERSPACE */

/**
 * @brief Verify that K_MEM_PERM_USER controls user access to a mapping.
 *
 * @ingroup kernel_memprotect_tests
 *
 * @details
 * A mapping created without K_MEM_PERM_USER must be invisible to user
 * threads, and one created with it must be usable from user mode. The same
 * physical page is mapped each way in turn and a user thread is spawned to
 * touch it; the access to the kernel-only mapping has to fault, which the
 * suite's fatal error handler accepts as the expected outcome. Skipped
 * without userspace.
 *
 * Test steps:
 * - Map the test page with K_MEM_PERM_USER, spawn a user thread to access it
 *   and join it.
 * - Unmap, then map the same page without K_MEM_PERM_USER.
 * - Spawn the user thread again to access the new mapping.
 *
 * Expected result:
 * - The access to the user-permitted mapping succeeds; the access to the
 *   kernel-only mapping faults.
 *
 * @see k_mem_map_phys_bare()
 */
ZTEST(mem_map_api, test_k_mem_map_user)
{
#ifdef CONFIG_USERSPACE
	int ret;

	ret = k_mem_domain_add_partition(&k_mem_domain_default, &default_part);
	if (ret != 0) {
		printk("Failed to add default memory partition (%d)\n", ret);
		k_oops();
	}

	/*
	 * Map the region using K_MEM_PERM_USER and try to access it from
	 * userspace
	 */
	expect_fault = false;

	k_mem_map_phys_bare(&mapped, k_mem_phys_addr(test_page), sizeof(test_page),
			    BASE_FLAGS | K_MEM_PERM_RW | K_MEM_PERM_USER);

	printk("mapped a page: %p - %p (with K_MEM_PERM_USER)\n", mapped,
		mapped + CONFIG_MMU_PAGE_SIZE);
	printk("trying to access %p from userspace\n", mapped);

	k_thread_create(&user_thread, user_stack, USER_STACKSIZE,
			user_function, NULL, NULL, NULL,
			-1, K_USER, K_NO_WAIT);
	k_thread_join(&user_thread, K_FOREVER);

	/* Unmap the memory */
	k_mem_unmap_phys_bare(mapped, sizeof(test_page));

	/*
	 * Map the region without using K_MEM_PERM_USER and try to access it
	 * from userspace. This should fault and fail.
	 */
	expect_fault = true;

	k_mem_map_phys_bare(&mapped, k_mem_phys_addr(test_page), sizeof(test_page),
			    BASE_FLAGS | K_MEM_PERM_RW);

	printk("mapped a page: %p - %p (without K_MEM_PERM_USER)\n", mapped,
		mapped + CONFIG_MMU_PAGE_SIZE);
	printk("trying to access %p from userspace\n", mapped);

	k_thread_create(&user_thread, user_stack, USER_STACKSIZE,
			user_function, NULL, NULL, NULL,
			-1, K_USER, K_NO_WAIT);
	k_thread_join(&user_thread, K_FOREVER);

	printk("shouldn't get here\n");
	ztest_test_fail();
#else
	ztest_test_skip();
#endif /* CONFIG_USERSPACE */
}

/* ztest main entry*/
void *mem_map_env_setup(void)
{
#ifdef CONFIG_DEMAND_PAGING
	/* This test sets up multiple mappings of RAM pages, which is only
	 * allowed for pinned memory
	 */
	k_mem_pin(test_page, sizeof(test_page));
#endif
	return NULL;
}

/* For CPUs with incoherent cache under SMP, the tests to read/write
 * buffer (... majority of tests here) may not work correctly if
 * the test thread jumps between CPUs. So use the test infrastructure
 * to limit the test to 1 CPU.
 */
#ifdef CONFIG_CPU_CACHE_INCOHERENT
#define FUNC_BEFORE ztest_simple_1cpu_before
#define FUNC_AFTER  ztest_simple_1cpu_after
#else
#define FUNC_BEFORE NULL
#define FUNC_AFTER  NULL
#endif

ZTEST_SUITE(mem_map, NULL, NULL, FUNC_BEFORE, FUNC_AFTER, NULL);
ZTEST_SUITE(mem_map_api, NULL, mem_map_env_setup, FUNC_BEFORE, FUNC_AFTER, NULL);
