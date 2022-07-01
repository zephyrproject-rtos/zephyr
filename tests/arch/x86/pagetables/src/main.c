/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * x86-specific tests for MMU features and page tables
 */

#include <zephyr/zephyr.h>
#include <ztest.h>
#include <tc_util.h>
#include <zephyr/arch/x86/mmustructs.h>
#include <x86_mmu.h>
#include <zephyr/linker/linker-defs.h>
#include <mmu.h>
#include "main.h"

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
extern char _lodata_end[];

#define LOCORE_START	((uint8_t *)&_locore_start)
#define LOCORE_END	((uint8_t *)&_lodata_end)
#endif

#ifdef CONFIG_COVERAGE_GCOV
extern char __gcov_bss_start[];
extern char __gcov_bss_size[];
#endif

static pentry_t get_entry(pentry_t *flags, void *addr)
{
	int level;
	pentry_t entry;

	z_x86_pentry_get(&level, &entry, z_x86_page_tables_get(), addr);

	zassert_true((entry & MMU_P) != 0,
		     "non-present RAM entry");
	zassert_equal(level, PT_LEVEL, "bigpage found");
	*flags = entry & FLAGS_MASK;

	return entry;
}

/**
 * Test that MMU flags on RAM virtual address range are set properly
 *
 * @ingroup kernel_memprotect_tests
 */
ZTEST(x86_pagetables, test_ram_perms)
{
	uint8_t *pos;

	pentry_t entry, flags, expected;

#ifdef CONFIG_LINKER_GENERIC_SECTIONS_PRESENT_AT_BOOT
	const uint8_t *mem_range_end = Z_KERNEL_VIRT_END;
#else
	const uint8_t *mem_range_end = (uint8_t *)lnkr_pinned_end;
#endif /* CONFIG_LINKER_GENERIC_SECTIONS_PRESENT_AT_BOOT */

	for (pos = Z_KERNEL_VIRT_START; pos < mem_range_end;
	     pos += CONFIG_MMU_PAGE_SIZE) {
		if (pos == NULL) {
			/* We have another test specifically for NULL page */
			continue;
		}

		entry = get_entry(&flags, pos);

		if (!IS_ENABLED(CONFIG_SRAM_REGION_PERMISSIONS)) {
			expected = MMU_P | MMU_RW;
		} else if (IN_REGION(__text_region, pos)) {
			expected = MMU_P | MMU_US;
		} else if (IN_REGION(__rodata_region, pos)) {
			expected = MMU_P | MMU_US | MMU_XD;
#ifdef CONFIG_COVERAGE_GCOV
		} else if (IN_REGION(__gcov_bss, pos)) {
			expected = MMU_P | MMU_RW | MMU_US | MMU_XD;
#endif
#if !defined(CONFIG_X86_KPTI) && !defined(CONFIG_X86_COMMON_PAGE_TABLE) && \
				  defined(CONFIG_USERSPACE)
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
#ifdef CONFIG_LINKER_USE_BOOT_SECTION
		} else if (IN_REGION(lnkr_boot_text, pos)) {
			expected = MMU_P | MMU_US;
		} else if (IN_REGION(lnkr_boot_rodata, pos)) {
			expected = MMU_P | MMU_US | MMU_XD;
#endif
#ifdef CONFIG_LINKER_USE_PINNED_SECTION
		} else if (IN_REGION(lnkr_pinned_text, pos)) {
			expected = MMU_P | MMU_US;
		} else if (IN_REGION(lnkr_pinned_rodata, pos)) {
			expected = MMU_P | MMU_US | MMU_XD;
#endif
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

#ifdef CONFIG_X86_64
	/* Check the locore too */
	for (pos = LOCORE_START; pos < LOCORE_END;
	     pos += CONFIG_MMU_PAGE_SIZE) {
		if (pos == NULL) {
			/* We have another test specifically for NULL page */
			continue;
		}

		entry = get_entry(&flags, pos);

		if (IN_REGION(_locore, pos)) {
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
		} else {
			expected = MMU_P | MMU_RW | MMU_XD;
		}
		zassert_equal(flags, expected,
			      "bad flags " PRI_ENTRY " at %p, expected "
			      PRI_ENTRY, flags, pos, expected);
	}
#endif /* CONFIG_X86_64 */

#ifdef CONFIG_ARCH_MAPS_ALL_RAM
	/* All RAM page frame entries aside from 0x0 must have a mapping.
	 * We currently identity-map on x86, no conversion necessary other than a cast
	 */
	for (pos = (uint8_t *)Z_PHYS_RAM_START; pos < (uint8_t *)Z_PHYS_RAM_END;
	     pos += CONFIG_MMU_PAGE_SIZE) {
		if (pos == NULL) {
			continue;
		}

		entry = get_entry(&flags, pos);
		zassert_true((flags & MMU_P) != 0,
			     "address %p isn't mapped", pos);
	}
#endif
}

/**
 * Test that the NULL virtual page is always non-present
 *
 * @ingroup kernel_memprotect_tests
 */
ZTEST(x86_pagetables, test_null_map)
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

#ifdef CONFIG_USERSPACE
void z_vrfy_dump_my_ptables(void)
{
	z_impl_dump_my_ptables();
}
#include <syscalls/dump_my_ptables_mrsh.c>
#endif /* CONFIG_USERSPACE */

void dump_pagetables(void)
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

/**
 * Dump kernel's page tables to console
 *
 * We don't verify any specific output, but this shouldn't crash
 *
 * @ingroup kernel_memprotect_tests
 */
ZTEST_USER(x86_pagetables, test_dump_ptables_user)
{
	dump_pagetables();
}

ZTEST(x86_pagetables, test_dump_ptables)
{
	dump_pagetables();
}

ZTEST_SUITE(x86_pagetables, NULL, NULL, NULL, NULL, NULL);
