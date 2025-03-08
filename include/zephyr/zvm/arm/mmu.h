/*
 * Copyright 2024-2025 HNU-ESNL: Guoqi Xie, Chenglai Xiong, Xingyu Hu and etc.
 * Copyright 2024-2025 openEuler SIG-Zephyr
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZVM_ARM_MM_H_
#define ZEPHYR_INCLUDE_ZVM_ARM_MM_H_

#include <zephyr/kernel.h>
#include <stdint.h>
#include <zephyr/sys/dlist.h>
#include <zephyr/sys/util.h>
#include <zephyr/arch/arm64/mm.h>
#include <zephyr/arch/arm64/arm_mmu.h>
#include <zephyr/zvm/vm_mm.h>

/**
 * stage-2 Memory types supported through MAIR.
 */
#define MT_S2_TYPE_MASK			0xFU
#define MT_S2_TYPE(attr)		(attr & MT_TYPE_MASK)
#define MT_S2_DEVICE_nGnRnE		0U
#define MT_S2_DEVICE_nGnRE		1U
#define MT_S2_DEVICE_GRE		2U
#define MT_S2_NORMAL_NC			3U
#define MT_S2_NORMAL			4U
#define MT_S2_NORMAL_WT			5U
#define MT_S2_NORMAL_WB			6U

/* Reuse host's mair for configure */
#define MEMORY_S2_ATTRIBUTES	((0x00 << (MT_S2_DEVICE_nGnRnE * 8)) |	\
				(0x04 << (MT_S2_DEVICE_nGnRE * 8))   |	\
				(0x0c << (MT_S2_DEVICE_GRE * 8))     |	\
				(0x44 << (MT_S2_NORMAL_NC * 8))      |	\
				(0xffUL << (MT_S2_NORMAL * 8))	  |	\
				(0xbbUL << (MT_S2_NORMAL_WT * 8)))

/* More flags from user's perpective are supported using remaining bits
 * of "attrs" field, i.e. attrs[31:4], underlying code will take care
 * of setting PTE fields correctly.
 */
#define  MT_S2_PERM_R_SHIFT			4U
#define  MT_S2_PERM_W_SHIFT			5U
#define  MT_S2_EXECUTE_SHIFT		6U
#define	 MT_S2_NOACCESS_SHIFT		7U
#define MT_S2_NR				(0U << MT_S2_PERM_R_SHIFT)
#define MT_S2_R					(1U << MT_S2_PERM_R_SHIFT)
#define MT_S2_NW				(0U << MT_S2_PERM_W_SHIFT)
#define MT_S2_W					(1U << MT_S2_PERM_W_SHIFT)
#define MT_S2_EXECUTE_NEVER		(0U << MT_S2_EXECUTE_SHIFT)
#define MT_S2_EXECUTE			(1U << MT_S2_EXECUTE_SHIFT)
#define MT_S2_ACCESS_ON			(0U << MT_S2_NOACCESS_SHIFT)
#define MT_S2_ACCESS_OFF		(1U << MT_S2_NOACCESS_SHIFT)
#define MT_S2_P_RW_U_RW_NXN		(MT_S2_R | MT_S2_W | MT_S2_EXECUTE)
#define MT_S2_P_RW_U_RW_XN		(MT_S2_R | MT_S2_W | MT_S2_EXECUTE_NEVER)

#define MT_VM_NORMAL_MEM		(MT_S2_P_RW_U_RW_NXN | MT_S2_NORMAL)
#define MT_VM_DEVICE_MEM		(MT_S2_P_RW_U_RW_XN | MT_S2_DEVICE_GRE)


/*
 * Block and Page descriptor attributes fields for stage-2
 */
#define S2_PTE_BLOCK_DESC_MEMTYPE(x)	(x << 2)
#define S2_PTE_BLOCK_DESC_I_DEV_CACHE	(0ULL << 2)
#define S2_PTE_BLOCK_DESC_I_NO_CACHE	(1ULL << 2)
#define S2_PTE_BLOCK_DESC_I_WT_CACHE	(2ULL << 2)
#define S2_PTE_BLOCK_DESC_I_WB_CACHE	(3ULL << 2)
#define S2_PTE_BLOCK_DESC_O_DEV_CACHE	(0ULL << 4)
#define S2_PTE_BLOCK_DESC_O_NO_CACHE	(1ULL << 4)
#define S2_PTE_BLOCK_DESC_O_WT_CACHE	(2ULL << 4)
#define S2_PTE_BLOCK_DESC_O_WB_CACHE	(3ULL << 4)
#define S2_PTE_BLOCK_DESC_AP_NO_RW		(0ULL << 6)
#define S2_PTE_BLOCK_DESC_AP_RO			(1ULL << 6)
#define S2_PTE_BLOCK_DESC_AP_WO			(2ULL << 6)
#define S2_PTE_BLOCK_DESC_AP_RW			(3ULL << 6)
#define S2_PTE_BLOCK_DESC_NON_SHARE		(0ULL << 8)
#define S2_PTE_BLOCK_DESC_OUTER_SHARE	(2ULL << 8)
#define S2_PTE_BLOCK_DESC_INNER_SHARE	(3ULL << 8)
#define S2_PTE_BLOCK_DESC_AF			(1ULL << 10)
#define S2_PTE_BLOCK_DESC_XS			(0ULL << 11)
#define S2_PTE_BLOCK_DESC_NO_XN			(0ULL << 53)
#define S2_PTE_BLOCK_DESC_P_XN			(1ULL << 53)
#define S2_PTE_BLOCK_DESC_PU_XN			(2ULL << 53)
#define S2_PTE_BLOCK_DESC_U_XN			(3ULL << 53)

/* aliged memory size to page */
#define ALIGN_TO_PAGE(size)	\
	(((size) + (CONFIG_MMU_PAGE_SIZE - 1)) & ~(CONFIG_MMU_PAGE_SIZE - 1))

/**
 * @brief Mapping vpart to physical block address.
 */
int arch_mmap_vpart_to_block(uintptr_t phys, uintptr_t virt, size_t size, uint32_t attrs);
int arch_unmap_vpart_to_block(uintptr_t virt, size_t size);

int arch_vm_dev_domain_unmap(
	uint64_t pbase, uint64_t vbase, uint64_t size,
	char *name, uint16_t vmid, struct arm_mmu_ptables *ptables);

int arch_vm_dev_domain_map(
	uint64_t pbase, uint64_t vbase, uint64_t size,
	char *name, uint16_t vmid, struct arm_mmu_ptables *ptables);

/**
 * @brief map vma to physical block address:
 * this function aim to translate virt address to phys address by setting the
 * hyp page table.
 */
int arch_mmap_vma_to_block(uintptr_t phys, uintptr_t virt, size_t size, uint32_t attrs);
int arch_unmap_vma_to_block(uintptr_t virt, size_t size);

/**
 * @brief Add a partition to the vm virtual memory domain.
 */
int arch_vm_mem_domain_partition_add(struct k_mem_domain *domain,
				  uint32_t partition_id, uintptr_t phys_start, uint32_t vmid);

/**
 * @brief remove a partition from the vm virtual memory domain.
 */
int arch_vm_mem_domain_partition_remove(struct k_mem_domain *domain,
					uint32_t partition_id, uint32_t vmid);

/**
 * @brief Architecture-specific hook for vm domain initialization.
 */
int arch_mem_domain_init(struct k_mem_domain *domain);

#endif /* ZEPHYR_INCLUDE_ZVM_ARM_MM_H_ */
