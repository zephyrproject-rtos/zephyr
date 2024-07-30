/*
 * Copyright (c) 2024 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <kernel_arch_interface.h>

/*
 * Virtual and physical addresses used to exercize MMU page table recycling.
 * Those are completely arbitrary addresses away from any existing addresses
 * (no worry, the test will fail otherwise). Those addresses don't have
 * to be valid as we won't attempt any access to the mapped memory.
 */
#define TEST_VIRT_ADDR	0x456560000
#define TEST_PHYS_ADDR	0x123230000

/* special test hooks in arch/arm64/core/mmu.c for test purpose */
extern int arm64_mmu_nb_free_tables(void);
extern int arm64_mmu_tables_total_usage(void);

/* initial states to compare against */
static int initial_nb_free_tables;
static int initial_tables_usage;

static void *arm64_mmu_test_init(void)
{
	/* get initial states */
	initial_nb_free_tables = arm64_mmu_nb_free_tables();
	initial_tables_usage = arm64_mmu_tables_total_usage();

	TC_PRINT("  Total page tables:           %d\n", CONFIG_MAX_XLAT_TABLES);
	TC_PRINT("  Initial free tables:         %d\n", initial_nb_free_tables);
	TC_PRINT("  Initial total table usage:   %#x\n", initial_tables_usage);

	zassert_true(initial_nb_free_tables > 1,
		     "initial_nb_free_tables = %d", initial_nb_free_tables);
	zassert_true(initial_tables_usage > 1,
		     "initial_tables_usage = %d", initial_tables_usage);

	return NULL;
}

static int mem_map_test(uintptr_t virt_addr, uintptr_t phys_addr, size_t size)
{
	/*
	 * This is not defined to return any error but the implementation
	 * will call k_panic() if an error occurs.
	 */
	arch_mem_map((void *)virt_addr, phys_addr, size, K_MEM_ARM_NORMAL_NC);

	int mapped_nb_free_tables = arm64_mmu_nb_free_tables();
	int mapped_tables_usage = arm64_mmu_tables_total_usage();

	TC_PRINT("  After arch_mem_map:\n");
	TC_PRINT("   current free tables:        %d\n", mapped_nb_free_tables);
	TC_PRINT("   current total table usage:  %#x\n", mapped_tables_usage);

	zassert_true(mapped_nb_free_tables < initial_nb_free_tables,
		     "%d vs %d", mapped_nb_free_tables, initial_nb_free_tables);
	zassert_true(mapped_tables_usage > initial_tables_usage,
		     "%#x vs %#x", mapped_tables_usage > initial_tables_usage);

	arch_mem_unmap((void *)virt_addr, size);

	int unmapped_nb_free_tables = arm64_mmu_nb_free_tables();
	int unmapped_tables_usage = arm64_mmu_tables_total_usage();

	TC_PRINT("  After arch_mem_unmap:\n");
	TC_PRINT("   current free tables:        %d\n", unmapped_nb_free_tables);
	TC_PRINT("   current total table usage:  %#x\n", unmapped_tables_usage);

	zassert_true(unmapped_nb_free_tables == initial_nb_free_tables,
		     "%d vs %d", unmapped_nb_free_tables, initial_nb_free_tables);
	zassert_true(unmapped_tables_usage == initial_tables_usage,
		     "%#x vs %#x", unmapped_tables_usage > initial_tables_usage);

	int tables_used = unmapped_nb_free_tables - mapped_nb_free_tables;
	return tables_used;
}

ZTEST(arm64_mmu, test_arm64_mmu_01_single_page)
{
	/*
	 * Let's map a single page to start with. This will allocate
	 * multiple tables to reach the deepest level.
	 */
	uintptr_t virt = TEST_VIRT_ADDR;
	uintptr_t phys = TEST_PHYS_ADDR;
	size_t size    = CONFIG_MMU_PAGE_SIZE;

	int tables_used = mem_map_test(virt, phys, size);

	zassert_true(tables_used == 2, "used %d tables", tables_used);
}

ZTEST(arm64_mmu, test_arm64_mmu_02_single_block)
{
	/*
	 * Same thing as above, except that we expect a block mapping
	 * this time. Both addresses and the size must be properly aligned.
	 * Table allocation won't go as deep as for a page.
	 */
	int table_entries = CONFIG_MMU_PAGE_SIZE / sizeof(uint64_t);
	size_t block_size = table_entries * CONFIG_MMU_PAGE_SIZE;
	uintptr_t virt = TEST_VIRT_ADDR & ~(block_size - 1);
	uintptr_t phys = TEST_PHYS_ADDR & ~(block_size - 1);

	int tables_used = mem_map_test(virt, phys, block_size);

	zassert_true(tables_used == 1, "used %d tables", tables_used);
}

ZTEST(arm64_mmu, test_arm64_mmu_03_block_and_page)
{
	/*
	 * Same thing as above, except that we expect a block mapping
	 * followed by a page mapping to exercize range splitting.
	 * To achieve that we simply increase the size by one page and keep
	 * starting addresses aligned to a block.
	 */
	int table_entries = CONFIG_MMU_PAGE_SIZE / sizeof(uint64_t);
	size_t block_size = table_entries * CONFIG_MMU_PAGE_SIZE;
	uintptr_t virt = TEST_VIRT_ADDR & ~(block_size - 1);
	uintptr_t phys = TEST_PHYS_ADDR & ~(block_size - 1);
	size_t size = block_size + CONFIG_MMU_PAGE_SIZE;

	int tables_used = mem_map_test(virt, phys, size);

	zassert_true(tables_used == 2, "used %d tables", tables_used);
}

ZTEST(arm64_mmu, test_arm64_mmu_04_page_and_block)
{
	/*
	 * Same thing as above, except that we expect a page mapping
	 * followed by a block mapping to exercize range splitting.
	 * To achieve that we increase the size by one page and decrease
	 * starting addresses by one page below block alignment.
	 */
	int table_entries = CONFIG_MMU_PAGE_SIZE / sizeof(uint64_t);
	size_t block_size = table_entries * CONFIG_MMU_PAGE_SIZE;
	uintptr_t virt = (TEST_VIRT_ADDR & ~(block_size - 1)) - CONFIG_MMU_PAGE_SIZE;
	uintptr_t phys = (TEST_PHYS_ADDR & ~(block_size - 1)) - CONFIG_MMU_PAGE_SIZE;
	size_t size = block_size + CONFIG_MMU_PAGE_SIZE;

	int tables_used = mem_map_test(virt, phys, size);

	zassert_true(tables_used == 2, "used %d tables", tables_used);
}

ZTEST_SUITE(arm64_mmu, NULL, arm64_mmu_test_init, NULL, NULL, NULL);
