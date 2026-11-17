/*
 * Copyright (c) 2026 Jiaming Zhao
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/kernel/mm.h>
#include <zephyr/ztest.h>
#include <kernel_arch_interface.h>
#include <mmu.h>

static uint8_t test_pages[2 * CONFIG_MMU_PAGE_SIZE] __aligned(CONFIG_MMU_PAGE_SIZE);

ZTEST(arm_mmu, test_l2_table_release_clears_l1_reference)
{
	uint8_t *mapped[CONFIG_ARM_MMU_NUM_L2_TABLES];
	uintptr_t free_start = ROUND_UP((uintptr_t)K_MEM_VM_FREE_START, MB(1));
	uintptr_t free_end = (uintptr_t)K_MEM_VIRT_RAM_END - K_MEM_VM_RESERVED;
	uintptr_t phys = k_mem_phys_addr(test_pages);
	uintptr_t phys_replacement = k_mem_phys_addr(test_pages + CONFIG_MMU_PAGE_SIZE);
	size_t mapped_count = 0;
	size_t i;
	uintptr_t resolved;
	int ret;
	uint8_t *first;
	uint8_t *replacement;

	zassert_true(free_start < free_end, "no room for runtime mappings");
	zassert_true(free_start + CONFIG_ARM_MMU_NUM_L2_TABLES * MB(1) <= free_end,
		     "virtual memory is too small for %d L2 tables",
		     CONFIG_ARM_MMU_NUM_L2_TABLES);

	/* Use one 1 MB virtual region at a time so each mapping needs an L2 table. */
	for (i = 0; i < ARRAY_SIZE(mapped); i++) {
		uint8_t *va = (uint8_t *)(free_start + i * MB(1) + CONFIG_MMU_PAGE_SIZE);

		zassert_equal(arch_page_phys_get(va, &resolved), -EFAULT,
			      "test VA %p is already mapped", va);

		ret = arch_mem_map(va, phys, CONFIG_MMU_PAGE_SIZE,
				   K_MEM_CACHE_WB | K_MEM_PERM_RW);
		if (ret == -ENOMEM) {
			break;
		}

		zassert_ok(ret, "failed to map test VA %p", va);
		zassert_equal(arch_page_phys_get(va, &resolved), 0);
		zassert_equal(resolved, phys);
		mapped[mapped_count++] = va;
	}

	zassert_true(mapped_count > 0, "failed to allocate any L2 tables");
	zassert_true(i < ARRAY_SIZE(mapped), "did not exhaust the L2 table pool");

	/* Releasing the table must also remove the L1 descriptor referring to it. */
	first = mapped[0];
	zassert_ok(arch_mem_unmap(first, CONFIG_MMU_PAGE_SIZE));

	/* There are no free tables now, so this mapping has to reuse the released one. */
	replacement = (uint8_t *)(free_start + i * MB(1) + CONFIG_MMU_PAGE_SIZE);
	zassert_ok(arch_mem_map(replacement, phys_replacement, CONFIG_MMU_PAGE_SIZE,
				K_MEM_CACHE_WB | K_MEM_PERM_RW));

	zassert_equal(arch_page_phys_get(first, &resolved), -EFAULT,
		      "released L2 table is still reachable through a stale L1 entry");

	for (i = 0; i < mapped_count; i++) {
		if (mapped[i] != first) {
			zassert_ok(arch_mem_unmap(mapped[i], CONFIG_MMU_PAGE_SIZE));
		}
	}
	zassert_ok(arch_mem_unmap(replacement, CONFIG_MMU_PAGE_SIZE));
}

ZTEST_SUITE(arm_mmu, NULL, NULL, NULL, NULL, NULL);
