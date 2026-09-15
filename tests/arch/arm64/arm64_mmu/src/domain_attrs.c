/*
 * Copyright (c) 2026 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * A domain-private mapping is built over whatever the kernel already has
 * mapped for the range. Which bits come from the partition and which are
 * kept from the kernel entry is the contract these tests pin down: the
 * partition grants access, the entry says what the memory is and where it
 * lives.
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <kernel_arch_interface.h>
#include <arch/arm64/core/mmu.h>

/* test hooks in arch/arm64/core/mmu.c */
extern int arm64_mmu_pte_get(struct arm_mmu_ptables *ptables, uintptr_t virt, uint64_t *desc,
			     unsigned int *level);

/*
 * Arbitrary addresses well away from anything the image maps, with the
 * virtual address deliberately different from the physical one.
 */
#define TEST_VIRT	0x556560000
#define TEST_PHYS	0x223230000
#define TEST_SIZE	CONFIG_MMU_PAGE_SIZE

#define MEMTYPE_MASK	(PTE_BLOCK_DESC_MEMTYPE(7) | PTE_BLOCK_DESC_INNER_SHARE)

/* somewhere else again, to soak up the translation table pool */
#define HOG_VIRT	0x700000000
#define HOG_PHYS	0x323230000
#define HOG_LIMIT	(4 * CONFIG_MAX_XLAT_TABLES)

static struct k_mem_domain test_domain;

static uint64_t kernel_pte(uintptr_t virt)
{
	unsigned int level;
	uint64_t desc;

	zassert_ok(arm64_mmu_pte_get(NULL, virt, &desc, &level),
		   "no kernel entry for %#lx", virt);
	return desc;
}

static uint64_t domain_pte(uintptr_t virt)
{
	unsigned int level;
	uint64_t desc;

	zassert_ok(arm64_mmu_pte_get(&test_domain.arch.ptables, virt, &desc, &level),
		   "no domain entry for %#lx", virt);
	return desc;
}

static struct k_mem_partition test_part;

static void add_partition(uintptr_t virt, size_t size, k_mem_partition_attr_t attr)
{
	test_part.start = virt;
	test_part.size = size;
	test_part.attr = attr;

	zassert_ok(k_mem_domain_add_partition(&test_domain, &test_part),
		   "k_mem_domain_add_partition(%#lx) failed", virt);
}

static void before(void *unused)
{
	ARG_UNUSED(unused);
	test_part.size = 0;
	zassert_ok(k_mem_domain_init(&test_domain, 0, NULL), "k_mem_domain_init() failed");
}

static void after(void *unused)
{
	ARG_UNUSED(unused);

	if (test_part.size != 0) {
		(void)k_mem_domain_remove_partition(&test_domain, &test_part);
	}
	(void)k_mem_domain_deinit(&test_domain);
	(void)arch_mem_unmap((void *)TEST_VIRT, TEST_SIZE);
}

/*
 * The partition grants access to what the kernel entry maps, so the output
 * address stays the kernel's physical address rather than becoming the
 * virtual one.
 */
ZTEST(arm64_mmu_domain, test_partition_keeps_output_address)
{
	zassert_ok(arch_mem_map((void *)TEST_VIRT, TEST_PHYS, TEST_SIZE, K_MEM_PERM_RW));

	add_partition(TEST_VIRT, TEST_SIZE, K_MEM_PARTITION_P_RW_U_RW);

	uint64_t desc = domain_pte(TEST_VIRT);

	zassert_equal(desc & PTE_PHYSADDR_MASK, TEST_PHYS,
		      "output address %#llx, expected %#lx", desc & PTE_PHYSADDR_MASK, TEST_PHYS);
	zassert_true((desc & PTE_BLOCK_DESC_AP_ELx) != 0, "EL0 access not granted");
	zassert_true((desc & PTE_BLOCK_DESC_NG) != 0, "private mapping must be non-global");
	zassert_true((desc & PTE_BLOCK_DESC_AF) != 0, "access flag lost");
}

/*
 * A partition says who may touch the memory, not what the memory is, so the
 * memory type and shareability stay those of the kernel entry.
 */
ZTEST(arm64_mmu_domain, test_partition_keeps_memory_type)
{
	zassert_ok(arch_mem_map((void *)TEST_VIRT, TEST_PHYS, TEST_SIZE, K_MEM_ARM_DEVICE_nGnRE));

	uint64_t kdesc = kernel_pte(TEST_VIRT);

	add_partition(TEST_VIRT, TEST_SIZE, K_MEM_PARTITION_P_RW_U_RW);

	uint64_t desc = domain_pte(TEST_VIRT);

	zassert_equal(desc & MEMTYPE_MASK, kdesc & MEMTYPE_MASK,
		      "memory type %#llx, expected %#llx", desc & MEMTYPE_MASK,
		      kdesc & MEMTYPE_MASK);
}

/* A read-only partition over writable memory is read-only to the domain. */
ZTEST(arm64_mmu_domain, test_partition_read_only)
{
	zassert_ok(arch_mem_map((void *)TEST_VIRT, TEST_PHYS, TEST_SIZE, K_MEM_PERM_RW));

	add_partition(TEST_VIRT, TEST_SIZE, K_MEM_PARTITION_P_RO_U_RO);

	uint64_t desc = domain_pte(TEST_VIRT);

	zassert_true((desc & PTE_BLOCK_DESC_AP_RO) != 0, "partition is not read-only");
	zassert_true((desc & PTE_SW_WRITABLE) == 0, "read-only partition marked writable");
	zassert_true((desc & PTE_BLOCK_DESC_AP_ELx) != 0, "EL0 access not granted");
}

/*
 * Access can only be granted to memory that is actually mapped: a partition
 * over a range the kernel has nothing for leaves the domain with nothing.
 */
ZTEST(arm64_mmu_domain, test_partition_over_unmapped_range)
{
	unsigned int level;
	uint64_t desc;

	zassert_equal(arm64_mmu_pte_get(NULL, TEST_VIRT, &desc, &level), -ENOENT,
		      "test address is already mapped");

	add_partition(TEST_VIRT, TEST_SIZE, K_MEM_PARTITION_P_RW_U_RW);

	zassert_equal(arm64_mmu_pte_get(&test_domain.arch.ptables, TEST_VIRT, &desc, &level),
		      -ENOENT, "unmapped range gained a domain entry %#llx", desc);
}

/*
 * Removing a partition hands the range back to what the kernel has for it,
 * so the domain ends up with the kernel entry again.
 */
ZTEST(arm64_mmu_domain, test_partition_remove_restores_entry)
{
	zassert_ok(arch_mem_map((void *)TEST_VIRT, TEST_PHYS, TEST_SIZE, K_MEM_PERM_RW));

	uint64_t kdesc = kernel_pte(TEST_VIRT);

	add_partition(TEST_VIRT, TEST_SIZE, K_MEM_PARTITION_P_RW_U_RW);
	zassert_not_equal(domain_pte(TEST_VIRT), kdesc, "partition did not change the entry");

	zassert_ok(k_mem_domain_remove_partition(&test_domain, &test_part));
	test_part.size = 0;

	zassert_equal(domain_pte(TEST_VIRT), kdesc,
		      "entry %#llx after removal, expected %#llx", domain_pte(TEST_VIRT), kdesc);
}

/*
 * A partition covering part of a block has to split it in the domain, and
 * only the covered part becomes reachable from EL0. The kernel keeps its
 * block: the split belongs to the domain.
 */
ZTEST(arm64_mmu_domain, test_partition_over_part_of_a_block)
{
	size_t block_size = (CONFIG_MMU_PAGE_SIZE / sizeof(uint64_t)) * CONFIG_MMU_PAGE_SIZE;
	uintptr_t virt = ROUND_DOWN(TEST_VIRT, block_size);
	uintptr_t phys = ROUND_DOWN(TEST_PHYS, block_size);
	uintptr_t outside = virt + block_size - CONFIG_MMU_PAGE_SIZE;
	unsigned int klevel, level;
	uint64_t kdesc;

	zassert_ok(arch_mem_map((void *)virt, phys, block_size, K_MEM_PERM_RW));
	zassert_ok(arm64_mmu_pte_get(NULL, virt, &kdesc, &klevel));

	add_partition(virt, CONFIG_MMU_PAGE_SIZE, K_MEM_PARTITION_P_RW_U_RW);

	uint64_t covered, uncovered;

	zassert_ok(arm64_mmu_pte_get(&test_domain.arch.ptables, virt, &covered, &level));
	zassert_true(level > klevel, "block not split: level %u, kernel level %u", level, klevel);
	zassert_true((covered & PTE_BLOCK_DESC_AP_ELx) != 0, "covered page has no EL0 access");
	zassert_equal(covered & PTE_PHYSADDR_MASK, phys, "covered page moved");

	zassert_ok(arm64_mmu_pte_get(&test_domain.arch.ptables, outside, &uncovered, &level));
	zassert_true((uncovered & PTE_BLOCK_DESC_AP_ELx) == 0,
		     "EL0 access leaked to the rest of the block");
	zassert_equal(uncovered & PTE_PHYSADDR_MASK, phys + block_size - CONFIG_MMU_PAGE_SIZE,
		      "rest of the block moved");

	uint64_t kdesc_after;

	zassert_ok(arm64_mmu_pte_get(NULL, virt, &kdesc_after, &level));
	zassert_equal(kdesc_after, kdesc, "kernel entry changed by a domain partition");
	zassert_equal(level, klevel, "kernel block was split");

	(void)k_mem_domain_remove_partition(&test_domain, &test_part);
	test_part.size = 0;
	(void)arch_mem_unmap((void *)virt, block_size);
}

/*
 * Splitting a block needs a table. With the pool empty the partition cannot
 * be confined to its own range, and applying it to the whole block would
 * hand the domain access well past the partition, so the add has to fail.
 */
ZTEST(arm64_mmu_domain, test_partition_over_block_without_tables)
{
	size_t block_size = (CONFIG_MMU_PAGE_SIZE / sizeof(uint64_t)) * CONFIG_MMU_PAGE_SIZE;
	uintptr_t virt = ROUND_DOWN(TEST_VIRT, block_size);
	uintptr_t phys = ROUND_DOWN(TEST_PHYS, block_size);
	uintptr_t outside = virt + block_size - CONFIG_MMU_PAGE_SIZE;
	struct k_mem_partition part = {
		.start = virt,
		.size = CONFIG_MMU_PAGE_SIZE,
		.attr = K_MEM_PARTITION_P_RW_U_RW,
	};
	unsigned int level;
	uint64_t desc;
	int hogged = 0;

	zassert_ok(arch_mem_map((void *)virt, phys, block_size, K_MEM_PERM_RW));

	/* take what the table pool has left, a page per block so each needs its own */
	while (hogged < HOG_LIMIT &&
	       arch_mem_map((void *)(HOG_VIRT + (uintptr_t)hogged * block_size), HOG_PHYS,
			    CONFIG_MMU_PAGE_SIZE, K_MEM_PERM_RW) == 0) {
		hogged++;
	}
	zassert_true(hogged < HOG_LIMIT, "table pool never ran out");

	zassert_not_equal(k_mem_domain_add_partition(&test_domain, &part), 0,
			  "partition applied with no table to split the block with");

	zassert_ok(arm64_mmu_pte_get(&test_domain.arch.ptables, outside, &desc, &level));
	zassert_true((desc & PTE_BLOCK_DESC_AP_ELx) == 0,
		     "EL0 access granted to the whole block");

	while (hogged-- > 0) {
		(void)arch_mem_unmap((void *)(HOG_VIRT + (uintptr_t)hogged * block_size),
				     CONFIG_MMU_PAGE_SIZE);
	}
	(void)arch_mem_unmap((void *)virt, block_size);
}

ZTEST_SUITE(arm64_mmu_domain, NULL, NULL, before, after, NULL);
