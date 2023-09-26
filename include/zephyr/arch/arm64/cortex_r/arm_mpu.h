/*
 * Copyright (c) 2018 Linaro Limited.
 * Copyright (c) 2018 Nordic Semiconductor ASA.
 * Copyright (c) 2021-2023 Arm Limited (or its affiliates). All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_ARCH_ARM64_CORTEX_R_MPU_ARM_MPU_H_
#define ZEPHYR_INCLUDE_ARCH_ARM64_CORTEX_R_MPU_ARM_MPU_H_

/*
 * Convenience macros to represent the ARMv8-R64-specific configuration
 * for memory access permission and cache-ability attribution.
 */
/* MPU MPUIR Register Definitions */
#define MPU_IR_REGION_Msk	(0xFFU)
/* MPU RBAR Register attribute msk Definitions */
#define MPU_RBAR_BASE_Pos	6U
#define MPU_RBAR_BASE_Msk	(0x3FFFFFFFFFFFFFFUL << MPU_RBAR_BASE_Pos)
#define MPU_RBAR_SH_Pos		4U
#define MPU_RBAR_SH_Msk		(0x3UL << MPU_RBAR_SH_Pos)
#define MPU_RBAR_AP_Pos		2U
#define MPU_RBAR_AP_Msk		(0x3UL << MPU_RBAR_AP_Pos)
/* RBAR_EL1 XN */
#define MPU_RBAR_XN_Pos		1U
#define MPU_RBAR_XN_Msk		(0x1UL << MPU_RBAR_XN_Pos)

/* MPU PLBAR_ELx Register Definitions */
#define MPU_RLAR_LIMIT_Pos	6U
#define MPU_RLAR_LIMIT_Msk	(0x3FFFFFFFFFFFFFFUL << MPU_RLAR_LIMIT_Pos)
#define MPU_RLAR_AttrIndx_Pos	1U
#define MPU_RLAR_AttrIndx_Msk	(0x7UL << MPU_RLAR_AttrIndx_Pos)
#define MPU_RLAR_EN_Msk		(0x1UL)

/* PRBAR_ELx: Attribute flag for not-allowing execution (eXecute Never) */
#define NOT_EXEC	MPU_RBAR_XN_Msk /* PRBAR_EL1 */

/* PRBAR_ELx: Attribute flag for access permissions */
/* Privileged Read Write, Unprivileged No Access */
#define P_RW_U_NA	0x0U
#define P_RW_U_NA_Msk	((P_RW_U_NA << MPU_RBAR_AP_Pos) & MPU_RBAR_AP_Msk)
/* Privileged Read Write, Unprivileged Read Write */
#define P_RW_U_RW	0x1U
#define P_RW_U_RW_Msk	((P_RW_U_RW << MPU_RBAR_AP_Pos) & MPU_RBAR_AP_Msk)
/* Privileged Read Only, Unprivileged No Access */
#define P_RO_U_NA	0x2U
#define P_RO_U_NA_Msk	((P_RO_U_NA << MPU_RBAR_AP_Pos) & MPU_RBAR_AP_Msk)
/* Privileged Read Only, Unprivileged Read Only */
#define P_RO_U_RO	0x3U
#define P_RO_U_RO_Msk	((P_RO_U_RO << MPU_RBAR_AP_Pos) & MPU_RBAR_AP_Msk)

/* PRBAR_ELx: Attribute flags for share-ability */
#define NON_SHAREABLE	0x0U
#define NON_SHAREABLE_Msk	\
	((NON_SHAREABLE << MPU_RBAR_SH_Pos) & MPU_RBAR_SH_Msk)
#define OUTER_SHAREABLE 0x2U
#define OUTER_SHAREABLE_Msk	\
	((OUTER_SHAREABLE << MPU_RBAR_SH_Pos) & MPU_RBAR_SH_Msk)
#define INNER_SHAREABLE 0x3U
#define INNER_SHAREABLE_Msk	\
	((INNER_SHAREABLE << MPU_RBAR_SH_Pos) & MPU_RBAR_SH_Msk)

/* MPIR_ELx Attribute flags for cache-ability */

/* Memory Attributes for Device Memory
 * 1.Gathering (G/nG)
 *   Determines whether multiple accesses can be merged into a single
 *   bus transaction.
 *   nG: Number/size of accesses on the bus = number/size of accesses
 *   in code.
 *
 * 2.Reordering (R/nR)
 *   Determines whether accesses to the same device can be reordered.
 *   nR: Accesses to the same IMPLEMENTATION DEFINED block size will
 *   appear on the bus in program order.
 *
 * 3 Early Write Acknowledgment (E/nE)
 *   Indicates to the memory system whether a buffer can send
 *   acknowledgements.
 *   nE: The response should come from the end slave, not buffering in
 *   the interconnect.
 */
#define DEVICE_nGnRnE	0x0U
#define DEVICE_nGnRE	0x4U
#define DEVICE_nGRE	0x8U
#define DEVICE_GRE	0xCU

/* Read/Write Allocation Configurations for Cacheable Memory */
#define R_NON_W_NON	0x0U	/* Do not allocate Read/Write */
#define R_NON_W_ALLOC	0x1U	/* Do not allocate Read, Allocate Write */
#define R_ALLOC_W_NON	0x2U	/* Allocate Read, Do not allocate Write */
#define R_ALLOC_W_ALLOC 0x3U	/* Allocate Read/Write */

/* Memory Attributes for Normal Memory */
#define NORMAL_O_WT_NT	0x80U	/* Normal, Outer Write-through non-transient */
#define NORMAL_O_WB_NT	0xC0U	/* Normal, Outer Write-back non-transient */
#define NORMAL_O_NON_C	0x40U	/* Normal, Outer Non-Cacheable	*/

#define NORMAL_I_WT_NT	0x08U	/* Normal, Inner Write-through non-transient */
#define NORMAL_I_WB_NT	0x0CU	/* Normal, Inner Write-back non-transient */
#define NORMAL_I_NON_C	0x04U	/* Normal, Inner Non-Cacheable	*/

/* Global MAIR configurations */
#define MPU_MAIR_INDEX_DEVICE		0U
#define MPU_MAIR_ATTR_DEVICE		(DEVICE_nGnRnE)

#define MPU_MAIR_INDEX_FLASH		1U
#define MPU_MAIR_ATTR_FLASH				\
	((NORMAL_O_WT_NT | (R_ALLOC_W_NON << 4)) |	\
	 (NORMAL_I_WT_NT | R_ALLOC_W_NON))

#define MPU_MAIR_INDEX_SRAM		2U
#define MPU_MAIR_ATTR_SRAM				\
	((NORMAL_O_WB_NT | (R_ALLOC_W_ALLOC << 4)) |	\
	 (NORMAL_I_WB_NT | R_ALLOC_W_ALLOC))

#define MPU_MAIR_INDEX_SRAM_NOCACHE	3U
#define MPU_MAIR_ATTR_SRAM_NOCACHE			\
	((NORMAL_O_NON_C | (R_NON_W_NON << 4)) |	\
	 (NORMAL_I_NON_C | R_NON_W_NON))

#define MPU_MAIR_ATTRS						 \
	((MPU_MAIR_ATTR_DEVICE << (MPU_MAIR_INDEX_DEVICE * 8)) | \
	 (MPU_MAIR_ATTR_FLASH << (MPU_MAIR_INDEX_FLASH * 8)) |	 \
	 (MPU_MAIR_ATTR_SRAM << (MPU_MAIR_INDEX_SRAM * 8)) |	 \
	 (MPU_MAIR_ATTR_SRAM_NOCACHE << (MPU_MAIR_INDEX_SRAM_NOCACHE * 8)))

/* Some helper defines for common regions.
 *
 * Note that the ARMv8-R MPU architecture requires that the
 * enabled MPU regions are non-overlapping. Therefore, it is
 * recommended to use these helper defines only for configuring
 * fixed MPU regions at build-time.
 */
#define REGION_IO_ATTR						      \
	{							      \
		/* AP, XN, SH */				      \
		.rbar = NOT_EXEC | P_RW_U_NA_Msk | NON_SHAREABLE_Msk, \
		/* Cache-ability */				      \
		.mair_idx = MPU_MAIR_INDEX_DEVICE,		      \
	}

#define REGION_RAM_ATTR							\
	{								\
		/* AP, XN, SH */					\
		.rbar = NOT_EXEC | P_RW_U_NA_Msk | OUTER_SHAREABLE_Msk, \
		/* Cache-ability */					\
		.mair_idx = MPU_MAIR_INDEX_SRAM,			\
	}

#define REGION_RAM_NOCACHE_ATTR					      \
	{							      \
		/* AP, XN, SH */				      \
		.rbar = NOT_EXEC | P_RW_U_NA_Msk | NON_SHAREABLE_Msk, \
		/* Cache-ability */				      \
		.mair_idx = MPU_MAIR_INDEX_SRAM_NOCACHE,	      \
	}

#define REGION_RAM_TEXT_ATTR					\
	{							\
		/* AP, XN, SH */				\
		.rbar = P_RO_U_RO_Msk | INNER_SHAREABLE_Msk,	\
		/* Cache-ability */				\
		.mair_idx = MPU_MAIR_INDEX_SRAM,		\
	}

#define REGION_RAM_RO_ATTR						\
	{								\
		/* AP, XN, SH */					\
		.rbar = NOT_EXEC | P_RO_U_RO_Msk | INNER_SHAREABLE_Msk, \
		/* Cache-ability */					\
		.mair_idx = MPU_MAIR_INDEX_SRAM,			\
	}

#if defined(CONFIG_MPU_ALLOW_FLASH_WRITE)
/* Note that the access permissions allow for un-privileged writes
 */
#define REGION_FLASH_ATTR						    \
	{								    \
		.rbar = P_RW_U_RW_Msk | NON_SHAREABLE_Msk, /* AP, XN, SH */ \
		/* Cache-ability */					    \
		.mair_idx = MPU_MAIR_INDEX_FLASH,			    \
	}
#else /* CONFIG_MPU_ALLOW_FLASH_WRITE */
#define REGION_FLASH_ATTR						    \
	{								    \
		.rbar = P_RO_U_RO_Msk | NON_SHAREABLE_Msk, /* AP, XN, SH */ \
		/* Cache-ability */					    \
		.mair_idx = MPU_MAIR_INDEX_FLASH,			    \
	}
#endif /* CONFIG_MPU_ALLOW_FLASH_WRITE */

#ifndef _ASMLANGUAGE

struct arm_mpu_region_attr {
	/* Attributes belonging to PRBAR */
	uint8_t rbar : 6;
	/* MAIR index for attribute indirection */
	uint8_t mair_idx : 3;
};

/* Region definition data structure */
struct arm_mpu_region {
	/* Region Base Address */
	uint64_t base;
	/* Region limit Address */
	uint64_t limit;
	/* Region Name */
	const char *name;
	/* Region Attributes */
	struct arm_mpu_region_attr attr;
};

/* MPU configuration data structure */
struct arm_mpu_config {
	/* Number of regions */
	uint32_t num_regions;
	/* Regions */
	const struct arm_mpu_region *mpu_regions;
};

#define MPU_REGION_ENTRY(_name, _base, _limit, _attr) \
	{					      \
		.name = _name,			      \
		.base = _base,			      \
		.limit = _limit,		      \
		.attr = _attr,			      \
	}

#define K_MEM_PARTITION_P_RW_U_RW ((k_mem_partition_attr_t) \
	{(P_RW_U_RW_Msk), MPU_MAIR_INDEX_SRAM})
#define K_MEM_PARTITION_P_RW_U_NA ((k_mem_partition_attr_t) \
	{(P_RW_U_NA_Msk), MPU_MAIR_INDEX_SRAM})
#define K_MEM_PARTITION_P_RO_U_RO ((k_mem_partition_attr_t) \
	{(P_RO_U_RO_Msk), MPU_MAIR_INDEX_SRAM})
#define K_MEM_PARTITION_P_RO_U_NA ((k_mem_partition_attr_t) \
	{(P_RO_U_NA_Msk), MPU_MAIR_INDEX_SRAM})

typedef struct arm_mpu_region_attr k_mem_partition_attr_t;

/* Reference to the MPU configuration.
 *
 * This struct is defined and populated for each SoC (in the SoC definition),
 * and holds the build-time configuration information for the fixed MPU
 * regions enabled during kernel initialization. Dynamic MPU regions (e.g.
 * for Thread Stack, Stack Guards, etc.) are programmed during runtime, thus,
 * not kept here.
 */
extern const struct arm_mpu_config mpu_config;

struct dynamic_region_info {
	int index;
	struct arm_mpu_region region_conf;
};

#define ARM64_MPU_MAX_DYNAMIC_REGIONS						\
	1 + /* data section */							\
	(CONFIG_MAX_DOMAIN_PARTITIONS + 2) +					\
	(IS_ENABLED(CONFIG_ARM64_STACK_PROTECTION) ? 2 : 0) +			\
	(IS_ENABLED(CONFIG_USERSPACE) ? 2 : 0)

#endif	/* _ASMLANGUAGE */

#endif	/* ZEPHYR_INCLUDE_ARCH_ARM64_CORTEX_R_MPU_ARM_MPU_H_ */
