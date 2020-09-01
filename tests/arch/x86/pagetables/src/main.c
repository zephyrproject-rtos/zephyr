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

#define VM_BASE		((uint8_t *)CONFIG_SRAM_BASE_ADDRESS)
#define VM_LIMIT	(VM_BASE + KB((size_t)CONFIG_SRAM_SIZE))

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
		} else {
			/* We forced CONFIG_HW_STACK_PROTECTION off otherwise
			 * guard pages will have RW cleared. We can relax this
			 * once we start memory-mapping stacks.
			 */
			expected = MMU_P | MMU_RW | MMU_XD;
		}

		zassert_equal(flags, expected,
				"bad flags " PRI_ENTRY " at %p",
				flags, pos);
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

/**
 * Dump kernel's page tables to console
 *
 * We don't verify any specific output, but this shouldn't crash
 *
 * @ingroup kernel_memprotect_tests
 */
void test_dump_ptables(void)
{
	z_x86_dump_page_tables(z_x86_page_tables_get());
}

void test_main(void)
{
	ztest_test_suite(x86_pagetables,
			 ztest_unit_test(test_ram_perms),
			 ztest_unit_test(test_null_map),
			 ztest_unit_test(test_dump_ptables)
			 );
	ztest_run_test_suite(x86_pagetables);
}
