/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * x86-specific tests for MMU features and page tables
 */

#include <zephyr.h>
#include <ztest.h>
#include <tc_util.h>
#include <arch/x86/mmustructs.h>
#include <x86_mmu.h>
#include <linker/linker-defs.h>
#include "main.h"

#define VM_BASE		((uint8_t *)CONFIG_KERNEL_VM_BASE)
#define VM_LIMIT	(VM_BASE + CONFIG_KERNEL_RAM_SIZE)

#ifdef CONFIG_X86_64
#define PT_LEVEL	3
#elif CONFIG_X86_PAE
#define PT_LEVEL	2
#else
#define PT_LEVEL	1
#endif

/* Set of flags whose state we will check. Ignore Accessed/Dirty
 * At leaf level PS bit indicates PAT, but regardless we don't set it
 */
#define FLAGS_MASK	(MMU_P | MMU_RW | MMU_US | MMU_PWT | MMU_PCD | \
			 MMU_G | MMU_PS | MMU_XD)

#define LPTR(name, suffix)	((uint8_t *)&_CONCAT(name, suffix))
#define LSIZE(name, suffix)	((size_t)&_CONCAT(name, suffix))
#define IN_REGION(name, virt) \
	(virt >= LPTR(name, _start) && \
	 virt < (LPTR(name, _start) + LSIZE(name, _size)))

#ifdef CONFIG_X86_64
extern char _locore_start[];
extern char _locore_size[];
extern char _lorodata_start[];
extern char _lorodata_size[];
#endif

#ifdef CONFIG_COVERAGE_GCOV
extern char __gcov_bss_start[];
extern char __gcov_bss_size[];
#endif

/**
 * Test that MMU flags on RAM virtual address range are set properly
 *
 * @ingroup kernel_memprotect_tests
 */
void test_ram_perms(void)
{
	uint8_t *pos;

	for (pos = VM_BASE; pos < VM_LIMIT; pos += CONFIG_MMU_PAGE_SIZE) {
		int level;
		pentry_t entry, flags, expected;

		if (pos == NULL) {
			/* We have another test specifically for NULL page */
			continue;
		}

		z_x86_pentry_get(&level, &entry, z_x86_page_tables_get(), pos);

		zassert_true((entry & MMU_P) != 0,
			     "non-present RAM entry");
		zassert_equal(level, PT_LEVEL, "bigpage found");
		flags = entry & FLAGS_MASK;

		if (!IS_ENABLED(CONFIG_SRAM_REGION_PERMISSIONS)) {
			expected = MMU_P | MMU_RW;
		} else if (IN_REGION(_image_text, pos)) {
			expected = MMU_P | MMU_US;
		} else if (IN_REGION(_image_rodata, pos)) {
			expected = MMU_P | MMU_US | MMU_XD;
#ifdef CONFIG_COVERAGE_GCOV
		} else if (IN_REGION(__gcov_bss, pos)) {
			expected = MMU_P | MMU_RW | MMU_US | MMU_XD;
#endif
#ifdef CONFIG_X86_64
		} else if (IN_REGION(_locore, pos)) {
			if (IS_ENABLED(CONFIG_X86_KPTI)) {
				expected = MMU_P | MMU_US;
			} else {
				expected = MMU_P;
			}
		} else if (IN_REGION(_lorodata, pos)) {
			if (IS_ENABLED(CONFIG_X86_KPTI)) {
				expected = MMU_P | MMU_US | MMU_XD;
			} else {
				expected = MMU_P | MMU_XD;
			}
#endif /* CONFIG_X86_64 */
#if !defined(CONFIG_X86_KPTI) && !defined(CONFIG_X86_COMMON_PAGE_TABLE)
		} else if (IN_REGION(_app_smem, pos)) {
			/* If KPTI is not enabled, then the default memory
			 * domain affects our page tables even though we are
			 * in supervisor mode. We'd expect everything in
			 * the _app_smem region to have US set since all the
			 * partitions within it would be active in
			 * k_mem_domain_default (ztest_partition and any libc
			 * partitions)
			 *
			 * If we have a common page table, no thread has
			 * entered user mode yet and no domain regions
			 * will be programmed.
			 */
			expected = MMU_P | MMU_US | MMU_RW | MMU_XD;
#endif /* CONFIG_X86_KPTI */
		} else {
			/* We forced CONFIG_HW_STACK_PROTECTION off otherwise
			 * guard pages will have RW cleared. We can relax this
			 * once we start memory-mapping stacks.
			 */
			expected = MMU_P | MMU_RW | MMU_XD;
		}

		zassert_equal(flags, expected,
			      "bad flags " PRI_ENTRY " at %p, expected "
			      PRI_ENTRY, flags, pos, expected);
	}

}

/**
 * Test that the NULL virtual page is always non-present
 *
 * @ingroup kernel_memprotect_tests
 */
void test_null_map(void)
{
	int level;
	pentry_t entry;

	/* The NULL page must always be non-present */
	z_x86_pentry_get(&level, &entry, z_x86_page_tables_get(), NULL);
	zassert_true((entry & MMU_P) == 0, "present NULL entry");
}

void z_impl_dump_my_ptables(void)
{
	struct k_thread *cur = k_current_get();

	printk("Page tables for thread %p\n", cur);
	z_x86_dump_page_tables(z_x86_thread_page_tables_get(cur));
}

void z_vrfy_dump_my_ptables(void)
{
	z_impl_dump_my_ptables();
}
#include <syscalls/dump_my_ptables_mrsh.c>

/**
 * Dump kernel's page tables to console
 *
 * We don't verify any specific output, but this shouldn't crash
 *
 * @ingroup kernel_memprotect_tests
 */
void test_dump_ptables(void)
{
#if CONFIG_SRAM_SIZE > (32 << 10)
	/*
	 * Takes too long to dump page table, so skip dumping
	 * if memory size is larger than 32MB.
	 */
	ztest_test_skip();
#else
	dump_my_ptables();
#endif
}

void test_main(void)
{
	ztest_test_suite(x86_pagetables,
			 ztest_unit_test(test_ram_perms),
			 ztest_unit_test(test_null_map),
			 ztest_unit_test(test_dump_ptables),
			 ztest_user_unit_test(test_dump_ptables)
			 );
	ztest_run_test_suite(x86_pagetables);
}
