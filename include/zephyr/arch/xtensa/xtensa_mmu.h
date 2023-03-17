/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_XTENSA_XTENSA_MMU_H
#define ZEPHYR_INCLUDE_ARCH_XTENSA_XTENSA_MMU_H

#define Z_XTENSA_MMU_X  BIT(0)
#define Z_XTENSA_MMU_W  BIT(1)
#define Z_XTENSA_MMU_XW  (BIT(1) | BIT(0))

#define Z_XTENSA_MMU_CACHED_WB BIT(2)
#define Z_XTENSA_MMU_CACHED_WT BIT(3)

#define K_MEM_PARTITION_IS_EXECUTABLE(attr)	(((attr) & Z_XTENSA_MMU_X) != 0)
#define K_MEM_PARTITION_IS_WRITABLE(attr)	(((attr) & Z_XENSA_MMU_W) != 0)

/* Read-Write access permission attributes */
#define K_MEM_PARTITION_P_RW_U_RW ((k_mem_partition_attr_t) \
			{Z_XTENSA_MMU_W})
#define K_MEM_PARTITION_P_RW_U_NA ((k_mem_partition_attr_t) \
			{0})
#define K_MEM_PARTITION_P_RO_U_RO ((k_mem_partition_attr_t) \
			{0})
#define K_MEM_PARTITION_P_RO_U_NA ((k_mem_partition_attr_t) \
			{0})
#define K_MEM_PARTITION_P_NA_U_NA ((k_mem_partition_attr_t) \
	{0})

/* Execution-allowed attributes */
#define K_MEM_PARTITION_P_RX_U_RX ((k_mem_partition_attr_t) \
	{Z_XTENSA_MMU_X})

/*
 * This BIT tells the mapping code whether the uncached pointer should
 * be shared between all threads. That is not used in the HW, it is
 * just for the implementation.
 *
 * The pte mapping this memory will use an ASID that is set in the
 * ring 4 spot in RASID.
 */
#define Z_XTENSA_MMU_MAP_SHARED  BIT(30)

#define Z_XTENSA_MMU_ILLEGAL (BIT(3) | BIT(2))

/* Struct used to map a memory region */
struct xtensa_mmu_range {
	const char *name;
	const uint32_t start;
	const uint32_t end;
	const uint32_t attrs;
};

typedef uint32_t k_mem_partition_attr_t;

extern const struct xtensa_mmu_range xtensa_soc_mmu_ranges[];
extern int xtensa_soc_mmu_ranges_num;

void z_xtensa_mmu_init(void);

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
void z_xtensa_mmu_tlb_ipi(void);

/**
 * @brief Invalidate cache to page tables and flush TLBs.
 *
 * This invalidates cache to all page tables and flush TLBs
 * as they may have been modified by other processors.
 */
void z_xtensa_mmu_tlb_shootdown(void);

#endif /* ZEPHYR_INCLUDE_ARCH_XTENSA_XTENSA_MMU_H */
