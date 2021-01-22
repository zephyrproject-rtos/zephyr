/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <sys/mem_manage.h>

/* 32-bit IA32 page tables have no mechanism to restrict execution */
#if defined(CONFIG_X86) && !defined(CONFIG_X86_64) && !defined(CONFIG_X86_PAE)
#define SKIP_EXECUTE_TESTS
#endif

/* Skip the memory map executing case when coverage enabled in x86_64,
 * because it will crash due to incorrect address accessing for gcov variables.
 * See issue#30434 for more details.
 */
#if defined(CONFIG_X86_64) && defined(CONFIG_COVERAGE)
#define SKIP_EXECUTE_TESTS
#endif


#define BASE_FLAGS	(K_MEM_CACHE_WB)
volatile bool expect_fault;

static uint8_t __aligned(CONFIG_MMU_PAGE_SIZE)
			test_page[2 * CONFIG_MMU_PAGE_SIZE];

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
}

#ifndef SKIP_EXECUTE_TESTS
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
	uint8_t *mapped_rw, *mapped_exec, *mapped_ro;
	bool executed = false;
	void (*func)(bool *executed);

	expect_fault = false;

	/* Map with write permissions and copy the function into the page */
	z_phys_map(&mapped_rw, (uintptr_t)test_page,
		   sizeof(test_page), BASE_FLAGS | K_MEM_PERM_RW);

	memcpy(mapped_rw, (void *)&transplanted_function, CONFIG_MMU_PAGE_SIZE);

	/* Now map with execution enabled and try to run the copied fn */
	z_phys_map(&mapped_exec, (uintptr_t)test_page,
		   sizeof(test_page), BASE_FLAGS | K_MEM_PERM_EXEC);

	func = (void (*)(bool *executed))mapped_exec;
	func(&executed);
	zassert_true(executed, "function did not execute");

	/* Now map without execution and execution should now fail */
	z_phys_map(&mapped_ro, (uintptr_t)test_page,
		   sizeof(test_page), BASE_FLAGS);

	func = (void (*)(bool *executed))mapped_ro;
	expect_fault = true;
	func(&executed);

	printk("shouldn't get here\n");
	ztest_test_fail();
}
#else
void test_z_phys_map_exec(void)
{
	ztest_test_skip();
}
#endif /* SKIP_EXECUTE_TESTS */

/**
 * Show that memory mapping doesn't have unintended side effects
 *
 * @ingroup kernel_memprotect_tests
 */
void test_z_phys_map_side_effect(void)
{
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
}

/* ztest main entry*/
void test_main(void)
{
	ztest_test_suite(test_mem_map,
			ztest_unit_test(test_z_phys_map_rw),
			ztest_unit_test(test_z_phys_map_exec),
			ztest_unit_test(test_z_phys_map_side_effect)
			);
	ztest_run_test_suite(test_mem_map);
}
