/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#ifndef ZEPHYR_INCLUDE_ARCH_XTENSA_XTENSA_MMU_H
#define ZEPHYR_INCLUDE_ARCH_XTENSA_XTENSA_MMU_H

/**
 * @defgroup xtensa_mmu_apis Xtensa Memory Management Unit (MMU) APIs
 * @ingroup xtensa_apis
 * @{
 */

/**
 * @name Memory region permission and caching mode.
 * @{
 */

/** Memory region is executable. */
#define XTENSA_MMU_PERM_X		BIT(0)

/** Memory region is writable. */
#define XTENSA_MMU_PERM_W		BIT(1)

/** Memory region is both executable and writable */
#define XTENSA_MMU_PERM_WX		(XTENSA_MMU_PERM_W | XTENSA_MMU_PERM_X)

/** Memory region has write-back cache. */
#define XTENSA_MMU_CACHED_WB		BIT(2)

/** Memory region has write-through cache. */
#define XTENSA_MMU_CACHED_WT		BIT(3)

/**
 * @}
 */

/**
 * @name Memory domain and partitions
 * @{
 */

typedef uint32_t k_mem_partition_attr_t;

#define K_MEM_PARTITION_IS_EXECUTABLE(attr)	(((attr) & XTENSA_MMU_PERM_X) != 0)
#define K_MEM_PARTITION_IS_WRITABLE(attr)	(((attr) & XTENSA_MMU_PERM_W) != 0)
#define K_MEM_PARTITION_IS_USER(attr)		(((attr) & XTENSA_MMU_MAP_USER) != 0)

/* Read-Write access permission attributes */
#define K_MEM_PARTITION_P_RW_U_RW \
	((k_mem_partition_attr_t) {XTENSA_MMU_PERM_W | XTENSA_MMU_MAP_USER})
#define K_MEM_PARTITION_P_RW_U_NA \
	((k_mem_partition_attr_t) {0})
#define K_MEM_PARTITION_P_RO_U_RO \
	((k_mem_partition_attr_t) {XTENSA_MMU_MAP_USER})
#define K_MEM_PARTITION_P_RO_U_NA \
	((k_mem_partition_attr_t) {0})
#define K_MEM_PARTITION_P_NA_U_NA \
	((k_mem_partition_attr_t) {0})

/* Execution-allowed attributes */
#define K_MEM_PARTITION_P_RX_U_RX \
	((k_mem_partition_attr_t) {XTENSA_MMU_PERM_X})

/**
 * @}
 */

/**
 * @brief Software only bit to indicate a memory region can be accessed by user thread(s).
 *
 * This BIT tells the mapping code which ring PTE entries to use.
 */
#define XTENSA_MMU_MAP_USER		BIT(4)

/**
 * @brief Software only bit to indicate a memory region is shared by all threads.
 *
 * This BIT tells the mapping code whether the memory region should
 * be shared between all threads. That is not used in the HW, it is
 * just for the implementation.
 *
 * The PTE mapping this memory will use an ASID that is set in the
 * ring 4 spot in RASID.
 */
#define XTENSA_MMU_MAP_SHARED		BIT(30)

/**
 * Struct used to map a memory region.
 */
struct xtensa_mmu_range {
	/** Name of the memory region. */
	const char *name;

	/** Start address of the memory region. */
	const uint32_t start;

	/** End address of the memory region. */
	const uint32_t end;

	/** Attributes for the memory region. */
	const uint32_t attrs;
};

/**
 * @brief Additional memory regions required by SoC.
 *
 * These memory regions will be setup by MMU initialization code at boot.
 */
extern const struct xtensa_mmu_range xtensa_soc_mmu_ranges[];

/** Number of SoC additional memory regions. */
extern int xtensa_soc_mmu_ranges_num;

/**
 * @brief Initialize hardware MMU.
 *
 * This initializes the MMU hardware and setup the memory regions at boot.
 */
void xtensa_mmu_init(void);

/**
 * @brief Re-initialize hardware MMU.
 *
 * This configures the MMU hardware when the cpu lost context and has
 * re-started.
 *
 * It assumes that the page table is already created and accessible in memory.
 */
void xtensa_mmu_reinit(void);

/**
 * @brief Tell other processors to flush TLBs.
 *
 * This sends IPI to other processors to telling them to
 * invalidate cache to page tables and flush TLBs. This is
 * needed when one processor is updating page tables that
 * may affect threads running on other processors.
 *
 * @note This needs to be implemented in the SoC layer.
 */
void xtensa_mmu_tlb_ipi(void);

/**
 * @brief Invalidate cache to page tables and flush TLBs.
 *
 * This invalidates cache to all page tables and flush TLBs
 * as they may have been modified by other processors.
 */
void xtensa_mmu_tlb_shootdown(void);

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_ARCH_XTENSA_XTENSA_MMU_H */
