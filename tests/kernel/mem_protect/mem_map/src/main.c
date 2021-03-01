/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <sys/mem_manage.h>
#include <toolchain.h>

/* 32-bit IA32 page tables have no mechanism to restrict execution */
#if defined(CONFIG_X86) && !defined(CONFIG_X86_64) && !defined(CONFIG_X86_PAE)
#define SKIP_EXECUTE_TESTS
#endif

#define BASE_FLAGS	(K_MEM_CACHE_WB)
volatile bool expect_fault;

#ifndef CONFIG_KERNEL_LINK_IN_VIRT
static uint8_t __aligned(CONFIG_MMU_PAGE_SIZE)
			test_page[2 * CONFIG_MMU_PAGE_SIZE];
#endif

void k_sys_fatal_error_handler(unsigned int reason, const z_arch_esf_t *pEsf)
{
	printk("Caught system error -- reason %d\n", reason);

	if (expect_fault && reason == 0) {
		expect_fault = false;
		ztest_test_pass();
	} else {
		printk("Unexpected fault during test");
		k_fatal_halt(reason);
	}
}


/* z_phys_map() doesn't have alignment requirements, any oddly-sized buffer
 * can get mapped. This will span two pages.
 */
#define BUF_SIZE	5003
#define BUF_OFFSET	1238

/**
 * Show that mapping an irregular size buffer works and RW flag is respected
 *
 * @ingroup kernel_memprotect_tests
 */
void test_z_phys_map_rw(void)
{
#ifndef CONFIG_KERNEL_LINK_IN_VIRT
	uint8_t *mapped_rw, *mapped_ro;
	uint8_t *buf = test_page + BUF_OFFSET;

	expect_fault = false;

	/* Map in a page that allows writes */
	z_phys_map(&mapped_rw, (uintptr_t)buf,
		   BUF_SIZE, BASE_FLAGS | K_MEM_PERM_RW);

	/* Initialize buf with some bytes */
	for (int i = 0; i < BUF_SIZE; i++) {
		mapped_rw[i] = (uint8_t)(i % 256);
	}

	/* Map again this time only allowing reads */
	z_phys_map(&mapped_ro, (uintptr_t)buf,
		   BUF_SIZE, BASE_FLAGS);

	/* Check that the mapped area contains the expected data. */
	for (int i = 0; i < BUF_SIZE; i++) {
		zassert_equal(buf[i], mapped_ro[i],
			      "unequal byte at index %d", i);
	}

	/* This should explode since writes are forbidden */
	expect_fault = true;
	mapped_ro[0] = 42;

	printk("shouldn't get here\n");
	ztest_test_fail();
#else
	ztest_test_skip();
#endif /* CONFIG_KERNEL_LINK_IN_VIRT */
}

#if defined(SKIP_EXECUTE_TESTS) || defined(CONFIG_KERNEL_LINK_IN_VIRT)
void test_z_phys_map_exec(void)
{
	ztest_test_skip();
}
#else
extern char __test_mem_map_start[];
extern char __test_mem_map_end[];
extern char __test_mem_map_size[];

__in_section_unique(test_mem_map) __used
static void transplanted_function(bool *executed)
{
	*executed = true;
}

/**
 * Show that mapping with/withour K_MEM_PERM_EXEC works as expected
 *
 * @ingroup kernel_memprotect_tests
 */
void test_z_phys_map_exec(void)
{
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
	z_phys_map(&mapped_exec, (uintptr_t)__test_mem_map_start,
		   (uintptr_t)__test_mem_map_size,
		   BASE_FLAGS | K_MEM_PERM_EXEC);

	func = (void (*)(bool *executed))mapped_exec;
	func(&executed);
	zassert_true(executed, "function did not execute");

	/* Now map without execution and execution should now fail */
	z_phys_map(&mapped_ro, (uintptr_t)__test_mem_map_start,
		   (uintptr_t)__test_mem_map_size, BASE_FLAGS);

	func = (void (*)(bool *executed))mapped_ro;
	expect_fault = true;
	func(&executed);

	printk("shouldn't get here\n");
	ztest_test_fail();
}
#endif /* SKIP_EXECUTE_TESTS || CONFIG_KERNEL_LINK_IN_VIRT */

/**
 * Show that memory mapping doesn't have unintended side effects
 *
 * @ingroup kernel_memprotect_tests
 */
void test_z_phys_map_side_effect(void)
{
#ifndef CONFIG_KERNEL_LINK_IN_VIRT
	uint8_t *mapped;

	expect_fault = false;

	/* z_phys_map() is supposed to always create fresh mappings.
	 * Show that by mapping test_page to an RO region, we can still
	 * modify test_page.
	 */
	z_phys_map(&mapped, (uintptr_t)test_page,
		   sizeof(test_page), BASE_FLAGS);

	/* Should NOT fault */
	test_page[0] = 42;

	/* Should fault */
	expect_fault = true;
	mapped[0] = 42;
	printk("shouldn't get here\n");
	ztest_test_fail();
#else
	ztest_test_skip();
#endif /* CONFIG_KERNEL_LINK_IN_VIRT */
}

/**
 * Basic k_mem_map() functionality
 *
 * Does not exercise K_MEM_MAP_* control flags, just default behavior
 */
void test_k_mem_map(void)
{
	size_t free_mem, free_mem_after_map;
	char *mapped;
	int i;

	free_mem = k_mem_free_get();
	zassert_not_equal(free_mem, 0, "no free memory");
	printk("Free memory: %zu\n", free_mem);

	mapped = k_mem_map(CONFIG_MMU_PAGE_SIZE, K_MEM_PERM_RW);
	zassert_not_null(mapped, "failed to map memory");
	printk("mapped a page to %p\n", mapped);

	/* Page should be zeroed */
	for (i = 0; i < CONFIG_MMU_PAGE_SIZE; i++) {
		zassert_equal(mapped[i], '\x00', "page not zeroed");
	}

	free_mem_after_map = k_mem_free_get();
	printk("Free memory now: %zu\n", free_mem_after_map);
	zassert_equal(free_mem, free_mem_after_map + CONFIG_MMU_PAGE_SIZE,
		      "incorrect free memory accounting");

	/* Show we can write to page without exploding */
	(void)memset(mapped, '\xFF', CONFIG_MMU_PAGE_SIZE);
	for (i = 0; i < CONFIG_MMU_PAGE_SIZE; i++) {
		zassert_true(mapped[i] == '\xFF',
			     "incorrect value 0x%hhx read at index %d",
			     mapped[i], i);
	}

	/* TODO: Un-map the mapped page to clean up, once we have that
	 * capability
	 */
}

/* ztest main entry*/
void test_main(void)
{
#ifdef CONFIG_DEMAND_PAGING
	/* This test sets up multiple mappings of RAM pages, which is only
	 * allowed for pinned memory
	 */
	k_mem_pin(test_page, sizeof(test_page));
#endif
	ztest_test_suite(test_mem_map,
			ztest_unit_test(test_z_phys_map_rw),
			ztest_unit_test(test_z_phys_map_exec),
			ztest_unit_test(test_z_phys_map_side_effect),
			ztest_unit_test(test_k_mem_map)
			);
	ztest_run_test_suite(test_mem_map);
}
