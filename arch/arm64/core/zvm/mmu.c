/*
 * Copyright 2024-2025 HNU-ESNL: Guoqi Xie, Chenglai Xiong, Xingyu Hu and etc.
 * Copyright 2024-2025 openEuler SIG-Zephyr
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <kernel_arch_func.h>
#include <kernel_arch_interface.h>
#include <kernel_internal.h>
#include <zephyr/logging/log.h>
#include <zephyr/arch/arm64/cpu.h>
#include <zephyr/arch/arm64/lib_helpers.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/mem_manage.h>
#include "../core/mmu.h"
#include <zephyr/zvm/vm.h>

LOG_MODULE_DECLARE(ZVM_MODULE_NAME);

static uint64_t vm_xlat_tables[CONFIG_MAX_VM_NUM]
		[CONFIG_ZVM_MAX_VM_XLAT_TABLES * Ln_XLAT_NUM_ENTRIES]
		__aligned(Ln_XLAT_NUM_ENTRIES * sizeof(uint64_t));
static int vm_xlat_use_count[CONFIG_MAX_VM_NUM][CONFIG_ZVM_MAX_VM_XLAT_TABLES];
static struct k_spinlock vm_xlat_lock;

/*
 * @brief Gets a description of the virtual machine memory.
 */
static uint64_t get_vm_region_desc(uint32_t attrs)
{
	unsigned int mem_type;
	uint64_t desc = 0U;

	/*
	 * AP bits for EL0/EL1 RW permission on S2

	 *   AP[2:1]   EL0/EL1
	 * +--------------------+
	 *	 00	   NULL
	 *	 01		RO
	 *	 10		WO
	 *	 11		RW
	 */

	/* AP_R bits for Data access permission */
	desc |= (attrs & MT_S2_R) ? S2_PTE_BLOCK_DESC_AP_RO : S2_PTE_BLOCK_DESC_AP_NO_RW;

	/* AP_W bits for Data access permission */
	desc |= (attrs & MT_S2_W) ? S2_PTE_BLOCK_DESC_AP_WO : S2_PTE_BLOCK_DESC_AP_NO_RW;

	/* The access flag */
	desc |= (attrs & MT_S2_ACCESS_OFF) ? 0 : S2_PTE_BLOCK_DESC_AF;

	mem_type = MT_S2_TYPE(attrs);

	switch (mem_type) {
	case MT_S2_DEVICE_nGnRnE:
	case MT_S2_DEVICE_nGnRE:
	case MT_S2_DEVICE_GRE:
		desc |= S2_PTE_BLOCK_DESC_OUTER_SHARE;
		/* Map device memory as execute-never */
		desc |= S2_PTE_BLOCK_DESC_PU_XN;
		break;
	case MT_S2_NORMAL_WT:
	case MT_S2_NORMAL_NC:
	case MT_S2_NORMAL:
		/* Make Normal RW memory as execute */
		if ((attrs & (MT_S2_R | MT_S2_W))) {
			desc |= S2_PTE_BLOCK_DESC_NO_XN;
		}

		if (mem_type == MT_NORMAL) {
			desc |= S2_PTE_BLOCK_DESC_INNER_SHARE;
		} else {
			desc |= S2_PTE_BLOCK_DESC_OUTER_SHARE;
		}
		/*
		 * When VM thread use atomic operation, stage-2 attributes must be
		 * Normal memory, Outer Write-Back Cacheable & Inner Write-Back
		 * Cacheable.
		 */
		desc |= (S2_PTE_BLOCK_DESC_O_WB_CACHE | S2_PTE_BLOCK_DESC_I_WB_CACHE);
		break;
	}

	return desc;
}

/*
 * @brief Mapping virtual memory to physical memory.
 */
static void arch_vm_mmap_pre(uintptr_t virt_addr, uintptr_t phys_addr, size_t size, uint32_t flags)
{
	uintptr_t aligned_phys, addr_offset;
	size_t aligned_size, align_boundary;
	k_spinlock_key_t key;
	uint8_t *dest_addr;

	ARG_UNUSED(key);

	/* get the align address of this page */
	addr_offset = k_mem_region_align(&aligned_phys, &aligned_size,
						 phys_addr, size, CONFIG_MMU_PAGE_SIZE);
	__ASSERT(aligned_size != 0U, "0-length mapping at 0x%lx", aligned_phys);
	__ASSERT(aligned_phys < (aligned_phys + (aligned_size - 1)),
		 "wraparound for physical address 0x%lx (size %zu)",
		 aligned_phys, aligned_size);

	align_boundary = CONFIG_MMU_PAGE_SIZE;

	/* Obtain an appropriately sized chunk of virtual memory */
	dest_addr = (uint8_t *)virt_addr;

	/* If this fails there's something amiss with virt_region_get */
	__ASSERT((uintptr_t)dest_addr < ((uintptr_t)dest_addr + (size - 1)),
		 "wraparound for virtual address %p (size %zu)",
		 dest_addr, size);

}

static uint64_t *vm_new_table(uint32_t vmid)
{
	unsigned int i;

	/* Look for a free table. */
	for (i = 0U; i < CONFIG_ZVM_MAX_VM_XLAT_TABLES; i++) {
		if (vm_xlat_use_count[vmid][i] == 0U) {
			vm_xlat_use_count[vmid][i] = 1U;
			/* each table assign 512 entries */
			return &vm_xlat_tables[vmid][i * Ln_XLAT_NUM_ENTRIES];
		}
	}

	return NULL;
}

static inline bool vm_is_desc_block_aligned(uint64_t desc, unsigned int level_size)
{
	uint64_t mask = GENMASK(47, PAGE_SIZE_SHIFT);
	bool aligned = !((desc & mask) & (level_size - 1));

	return aligned;
}

static inline bool vm_is_desc_superset(uint64_t desc1, uint64_t desc2, unsigned int level)
{
	uint64_t mask = DESC_ATTRS_MASK | GENMASK(47, LEVEL_TO_VA_SIZE_SHIFT(level));

	return (desc1 & mask) == (desc2 & mask);
}

static inline bool vm_is_free_desc(uint64_t desc)
{
	return (desc & PTE_DESC_TYPE_MASK) == PTE_INVALID_DESC;
}

static inline uint64_t *vm_pte_desc_table(uint64_t desc)
{
	uint64_t address = desc & GENMASK(47, PAGE_SIZE_SHIFT);

	return (uint64_t *)address;
}

static inline bool vm_is_table_desc(uint64_t desc, unsigned int level)
{
	return level != XLAT_LAST_LEVEL && (desc & PTE_DESC_TYPE_MASK) == PTE_TABLE_DESC;
}

static inline bool vm_is_block_desc(uint64_t desc)
{
	return (desc & PTE_DESC_TYPE_MASK) == PTE_BLOCK_DESC;
}

static void vm_set_pte_block_desc(uint64_t *pte, uint64_t desc, unsigned int level)
{
	if (desc) {
		desc |= (level == XLAT_LAST_LEVEL) ? PTE_PAGE_DESC : PTE_BLOCK_DESC;
	}
	*pte = desc;
}

static void vm_set_pte_table_desc(uint64_t *pte, uint64_t *table, unsigned int level)
{
	/* Point pte to new table */
	*pte = PTE_TABLE_DESC | (uint64_t)table;
}

static inline unsigned int vm_table_index(uint64_t *pte, uint32_t vmid)
{
	unsigned int i;

	i = (pte - &vm_xlat_tables[vmid][0]) / Ln_XLAT_NUM_ENTRIES;
	__ASSERT(i < CONFIG_ZVM_MAX_VM_XLAT_TABLES, "table %p out of range", pte);

	return i;
}

/* Makes a table free for reuse. */
static void vm_free_table(uint64_t *table, uint32_t vmid)
{
	unsigned int i = vm_table_index(table, vmid);

	__ASSERT(vm_xlat_use_count[vmid][i] == 1U, "table still in use");
	vm_xlat_use_count[vmid][i] = 0U;
}

/* Adjusts usage count and returns current count. */
static int vm_table_usage(uint64_t *table, int adjustment, uint32_t vmid)
{
	unsigned int i, table_use;

	i = vm_table_index(table, vmid);

	vm_xlat_use_count[vmid][i] += adjustment;
	table_use = vm_xlat_use_count[vmid][i];
	__ASSERT(vm_xlat_use_count[vmid][i] > 0, "usage count underflow");

	return table_use;
}

static inline void vm_dec_table_ref(uint64_t *table, uint32_t vmid)
{
	int ref_unit = 0xFFFFFFFF;

	vm_table_usage(table, -ref_unit, vmid);
}

static inline bool vm_is_table_unused(uint64_t *table, uint32_t vmid)
{
	return vm_table_usage(table, 0, vmid) == 1;
}

static uint64_t *vm_expand_to_table(uint64_t *pte, unsigned int level, uint32_t vmid)
{
	uint64_t *table;

	if (level >= XLAT_LAST_LEVEL) {
		__ASSERT(level < XLAT_LAST_LEVEL, "can't expand last level");
	}

	table = vm_new_table(vmid);

	if (!table) {
		return NULL;
	}

	if (!vm_is_free_desc(*pte)) {
		/*
		 * If entry at current level was already populated
		 * then we need to reflect that in the new table.
		 */
		uint64_t desc = *pte;
		unsigned int i, stride_shift;

		__ASSERT(vm_is_block_desc(desc), "");

		if (level + 1 == XLAT_LAST_LEVEL) {
			desc |= PTE_PAGE_DESC;
		}

		stride_shift = LEVEL_TO_VA_SIZE_SHIFT(level + 1);
		for (i = 0U; i < Ln_XLAT_NUM_ENTRIES; i++) {
			table[i] = desc | (i << stride_shift);
		}
		vm_table_usage(table, Ln_XLAT_NUM_ENTRIES, vmid);
	} else {
		/*
		 * Adjust usage count for parent table's entry
		 * that will no longer be free.
		 */
		vm_table_usage(pte, 1, vmid);
	}

	/* Link the new table in place of the pte it replaces */
	vm_set_pte_table_desc(pte, table, level);

	return table;
}

static int vm_set_mapping(struct arm_mmu_ptables *ptables,
			   uintptr_t virt, size_t size,
			   uint64_t desc, bool may_overwrite, uint32_t vmid)
{
	uint64_t *pte, *ptes[XLAT_LAST_LEVEL + 1];
	uint64_t level_size;
	uint64_t *table = ptables->base_xlat_table;
	unsigned int level = BASE_XLAT_LEVEL;
	int ret = 0;

	while (size) {
		__ASSERT(level <= XLAT_LAST_LEVEL,
			 "max translation table level exceeded\n");

		/* Locate PTE for given virtual address and page table level */
		pte = &table[XLAT_TABLE_VA_IDX(virt, level)];
		ptes[level] = pte;

		if (vm_is_table_desc(*pte, level)) {
			/* Move to the next translation table level */
			level++;
			table = vm_pte_desc_table(*pte);
			continue;
		}

		if (!may_overwrite && !vm_is_free_desc(*pte)) {
			/* the entry is already allocated */
			ret = -EBUSY;
			break;
		}

		level_size = 1ULL << LEVEL_TO_VA_SIZE_SHIFT(level);

		if (vm_is_desc_superset(*pte, desc, level)) {
			/* This block already covers our range */
			level_size -= (virt & (level_size - 1));
			if (level_size > size) {
				level_size = size;
			}
			goto move_on;
		}

		if ((size < level_size) || (virt & (level_size - 1)) ||
			!vm_is_desc_block_aligned(desc, level_size)) {
			/* Range doesn't fit, create subtable */
			table = vm_expand_to_table(pte, level, vmid);
			if (!table) {
				ret = -ENOMEM;
				break;
			}
			level++;
			continue;
		}

		/* Adjust usage count for corresponding table */
		if (vm_is_free_desc(*pte)) {
			vm_table_usage(pte, 1, vmid);
		}
		if (!desc) {
			vm_table_usage(pte, -1, vmid);
		}
		/* Create (or erase) block/page descriptor */
		vm_set_pte_block_desc(pte, desc, level);

		/* recursively free unused tables if any */
		while (level != BASE_XLAT_LEVEL &&
			   vm_is_table_unused(pte, vmid)) {
			vm_free_table(pte, vmid);
			pte = ptes[--level];
			vm_set_pte_block_desc(pte, 0, level);
			vm_table_usage(pte, -1, vmid);
		}

move_on:
		virt += level_size;
		desc += desc ? level_size : 0;
		size -= level_size;

		/* Range is mapped, start again for next range */
		table = ptables->base_xlat_table;
		level = BASE_XLAT_LEVEL;

	}

	return ret;
}

static void vm_del_mapping(uint64_t *table, uintptr_t virt, size_t size,
			unsigned int level, uint32_t vmid)
{
	size_t step, level_size = 1ULL << LEVEL_TO_VA_SIZE_SHIFT(level);
	uint64_t *pte, *subtable;

	for ( ; size; virt += step, size -= step) {
		step = level_size - (virt & (level_size - 1));
		if (step > size) {
			step = size;
		}
		pte = &table[XLAT_TABLE_VA_IDX(virt, level)];

		if (vm_is_free_desc(*pte)) {
			continue;
		}

		if (step != level_size && vm_is_block_desc(*pte)) {
			/* need to split this block mapping */
			vm_expand_to_table(pte, level, vmid);
		}

		if (vm_is_table_desc(*pte, level)) {
			subtable = vm_pte_desc_table(*pte);
			vm_del_mapping(subtable, virt, step, level + 1, vmid);
			if (!vm_is_table_unused(subtable, vmid)) {
				continue;
			}
			vm_dec_table_ref(subtable, vmid);
		}

		/* free this entry */
		*pte = 0;
		vm_table_usage(pte, -1, vmid);
	}
}

/*
 * @brief un_map the vm's page table entry.
 */
static int vm_remove_dev_map(struct arm_mmu_ptables *ptables, const char *name,
			 uintptr_t phys, uintptr_t virt, size_t size, uint32_t attrs, uint32_t vmid)
{
	int ret = 0;
	k_spinlock_key_t key;

	ARG_UNUSED(attrs);

	__ASSERT(((virt | size) & (CONFIG_MMU_PAGE_SIZE - 1)) == 0,
		 "address/size are not page aligned\n");

	key = k_spin_lock(&vm_xlat_lock);
	ret = vm_set_mapping(ptables, virt, size, 0, true, vmid);
	k_spin_unlock(&vm_xlat_lock, key);
	return ret;
}

static int vm_add_dev_map(struct arm_mmu_ptables *ptables, const char *name,
			 uintptr_t phys, uintptr_t virt, size_t size, uint32_t attrs, uint32_t vmid)
{
	int ret;
	uint64_t desc;
	bool may_overwrite;
	k_spinlock_key_t key;

	/*TODO: Need a stage-2 attribution set*/
	may_overwrite = false;
	desc = phys;

	__ASSERT(((virt | phys | size) & (CONFIG_MMU_PAGE_SIZE - 1)) == 0,
		 "address/size are not page aligned\n");
	key = k_spin_lock(&vm_xlat_lock);

	ret = vm_set_mapping(ptables, virt, size, desc, may_overwrite, vmid);
	k_spin_unlock(&vm_xlat_lock, key);
	return ret;
}

static int vm_add_map(struct arm_mmu_ptables *ptables, const char *name,
		   uintptr_t phys, uintptr_t virt, size_t size, uint32_t attrs, uint32_t vmid)
{
	bool may_overwrite = !(attrs & MT_NO_OVERWRITE);
	uint64_t desc = get_vm_region_desc(attrs);
	k_spinlock_key_t key;
	int ret;

	desc |= phys;

	key = k_spin_lock(&vm_xlat_lock);

	/* size aligned to page size */
	size = ALIGN_TO_PAGE(size);
	__ASSERT(((virt | phys | size) & (CONFIG_MMU_PAGE_SIZE - 1)) == 0,
		 "address/size are not page aligned\n");
	ret = vm_set_mapping(ptables, virt, size, desc, may_overwrite, vmid);

	k_spin_unlock(&vm_xlat_lock, key);
	return ret;
}

static int vm_remove_map(struct arm_mmu_ptables *ptables, const char *name,
				uintptr_t virt, size_t size, uint32_t vmid)
{
	k_spinlock_key_t key;
	int ret = 0;

	key = k_spin_lock(&vm_xlat_lock);
	vm_del_mapping(ptables->base_xlat_table, virt, size, BASE_XLAT_LEVEL, vmid);
	k_spin_unlock(&vm_xlat_lock, key);

	return ret;
}

int arch_mmap_vpart_to_block(uintptr_t phys, uintptr_t virt, size_t size, uint32_t attrs)
{
	int ret;
	uintptr_t dest_virt = virt;

	ARG_UNUSED(ret);
	arch_vm_mmap_pre(dest_virt, phys, size,  attrs);
	return 0;
}

int arch_unmap_vpart_to_block(uintptr_t virt, size_t size)
{
	uintptr_t dest_virt = virt;

	ARG_UNUSED(dest_virt);
	return 0;
}

int arch_vm_dev_domain_unmap(uint64_t pbase, uint64_t vbase, uint64_t size,
	char *name, uint16_t vmid, struct arm_mmu_ptables *ptables)
{
	return vm_remove_dev_map(ptables, name, pbase, vbase, size, 0, vmid);
}

int arch_vm_dev_domain_map(uint64_t pbase, uint64_t vbase, uint64_t size,
	char *name, uint16_t vmid, struct arm_mmu_ptables *ptables)
{
	uint32_t mem_attrs;

	mem_attrs = MT_DEVICE_nGnRnE | MT_P_RW_U_NA | MT_DEFAULT_SECURE_STATE;
	return vm_add_dev_map(ptables, name, pbase, vbase, size, mem_attrs | MT_NO_OVERWRITE, vmid);
}

int arch_vm_mem_domain_partition_add(
	struct k_mem_domain *domain,
	uint32_t partition_id, uintptr_t phys_start, uint32_t vmid)
{
	struct arm_mmu_ptables *domain_ptables = &domain->arch.ptables;
	struct k_mem_partition *ptn = &domain->partitions[partition_id];

	ZVM_LOG_INFO("PART_ADD: phys_start 0x%lx, virt_start 0x%lx, size 0x%lx.\n",
		phys_start, ptn->start, ptn->size);
	return vm_add_map(domain_ptables, "vm-mmio-space", phys_start,
				ptn->start, ptn->size, ptn->attr.attrs, vmid);
}

int arch_vm_mem_domain_partition_remove(
	struct k_mem_domain *domain,
	uint32_t partition_id,
	uint32_t vmid)
{
	int ret;
	struct arm_mmu_ptables *domain_ptables = &domain->arch.ptables;
	struct k_mem_partition *ptn = &domain->partitions[partition_id];

	ZVM_LOG_INFO("PART_ADD: virt_start 0x%lx, size 0x%lx.\n", ptn->start, ptn->size);
	ret =  vm_remove_map(domain_ptables, "vm-mmio-space", ptn->start, ptn->size, vmid);
	return ret;
}

void arch_vm_mem_domain_partitions_clean(
	struct k_mem_domain *domain,
	uint32_t partitions_num,
	uint32_t vmid)
{
	k_spinlock_key_t key;
	uint32_t p_idx;

	ARG_UNUSED(domain);

	key = k_spin_lock(&vm_xlat_lock);
	for (p_idx = 0; p_idx < partitions_num; p_idx++) {
		vm_xlat_use_count[vmid][p_idx] = 0;
	}
	k_spin_unlock(&vm_xlat_lock, key);

}

int arch_vm_mem_domain_init(struct k_mem_domain *domain, uint32_t vmid)
{
	struct arm_mmu_ptables *domain_ptables = &domain->arch.ptables;
	k_spinlock_key_t key;

	key = k_spin_lock(&vm_xlat_lock);
	domain_ptables->base_xlat_table = vm_new_table(vmid);
	k_spin_unlock(&vm_xlat_lock, key);
	if (!domain_ptables->base_xlat_table) {
		return -ENOMEM;
	}
	return 0;
	}
