/*
 * Copyright (c) 2018 Linaro Limited.
 * Copyright (c) 2018 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ASMLANGUAGE

/* Convenience macros to represent the ARMv8-M-specific
 * configuration for memory access permission and
 * cache-ability attribution.
 */
#if defined(CONFIG_AARCH32_ARMV8_R)
#define MPU_IR_REGION_Msk       (0xFFU)
#define MPU_IR_REGION_Pos       8U
/* MPU RBAR Register attribute msk Definitions */
#define MPU_RBAR_BASE_Pos       6U
#define MPU_RBAR_BASE_Msk       (0x3FFFFFFFFFFFFFFUL << MPU_RBAR_BASE_Pos)
#define MPU_RBAR_SH_Pos         3U
#define MPU_RBAR_SH_Msk         (0x3UL << MPU_RBAR_SH_Pos)
#define MPU_RBAR_AP_Pos         1U
#define MPU_RBAR_AP_Msk         (0x3UL << MPU_RBAR_AP_Pos)
/* RBAR XN */
#define MPU_RBAR_XN_Pos         0U
#define MPU_RBAR_XN_Msk         (0x1UL << MPU_RBAR_XN_Pos)

/* MPU PLBAR Register Definitions */
#define MPU_RLAR_LIMIT_Pos      6U
#define MPU_RLAR_LIMIT_Msk      (0x3FFFFFFFFFFFFFFUL << MPU_RLAR_LIMIT_Pos)
#define MPU_RLAR_AttrIndx_Pos   1U
#define MPU_RLAR_AttrIndx_Msk   (0x7UL << MPU_RLAR_AttrIndx_Pos)
#define MPU_RLAR_EN_Msk         (0x1UL)
#else
#include <cmsis_core.h>
#endif

/* Privileged No Access, Unprivileged No Access */
/*#define NO_ACCESS       0x0 */
/*#define NO_ACCESS_Msk   ((NO_ACCESS << MPU_RASR_AP_Pos) & MPU_RASR_AP_Msk) */
/* Privileged No Access, Unprivileged No Access */
/*#define P_NA_U_NA       0x0 */
/*#define P_NA_U_NA_Msk   ((P_NA_U_NA << MPU_RASR_AP_Pos) & MPU_RASR_AP_Msk) */
/* Privileged Read Write, Unprivileged No Access */
#define P_RW_U_NA       0x0
#define P_RW_U_NA_Msk   ((P_RW_U_NA << MPU_RBAR_AP_Pos) & MPU_RBAR_AP_Msk)
/* Privileged Read Write, Unprivileged Read Only */
/*#define P_RW_U_RO       0x2 */
/*#define P_RW_U_RO_Msk   ((P_RW_U_RO << MPU_RASR_AP_Pos) & MPU_RASR_AP_Msk)*/
/* Privileged Read Write, Unprivileged Read Write */
#define P_RW_U_RW       0x1
#define P_RW_U_RW_Msk   ((P_RW_U_RW << MPU_RBAR_AP_Pos) & MPU_RBAR_AP_Msk)
/* Privileged Read Write, Unprivileged Read Write */
#define FULL_ACCESS     0x1
#define FULL_ACCESS_Msk ((FULL_ACCESS << MPU_RBAR_AP_Pos) & MPU_RBAR_AP_Msk)
/* Privileged Read Only, Unprivileged No Access */
#define P_RO_U_NA       0x2
#define P_RO_U_NA_Msk   ((P_RO_U_NA << MPU_RBAR_AP_Pos) & MPU_RBAR_AP_Msk)
/* Privileged Read Only, Unprivileged Read Only */
#define P_RO_U_RO       0x3
#define P_RO_U_RO_Msk   ((P_RO_U_RO << MPU_RBAR_AP_Pos) & MPU_RBAR_AP_Msk)
/* Privileged Read Only, Unprivileged Read Only */
#define RO              0x3
#define RO_Msk          ((RO << MPU_RBAR_AP_Pos) & MPU_RBAR_AP_Msk)

/* Attribute flag for not-allowing execution (eXecute Never) */
#define NOT_EXEC MPU_RBAR_XN_Msk

/* Attribute flags for share-ability */
#define NON_SHAREABLE       0x0
#define NON_SHAREABLE_Msk \
	((NON_SHAREABLE << MPU_RBAR_SH_Pos) & MPU_RBAR_SH_Msk)
#define OUTER_SHAREABLE 0x2
#define OUTER_SHAREABLE_Msk \
	((OUTER_SHAREABLE << MPU_RBAR_SH_Pos) & MPU_RBAR_SH_Msk)
#define INNER_SHAREABLE 0x3
#define INNER_SHAREABLE_Msk \
	((INNER_SHAREABLE << MPU_RBAR_SH_Pos) & MPU_RBAR_SH_Msk)

/* Helper define to calculate the region limit address. */
#define REGION_LIMIT_ADDR(base, size) \
	(((base & MPU_RBAR_BASE_Msk) + size - 1) & MPU_RLAR_LIMIT_Msk)

/* Attribute flags for cache-ability */
#if defined(CONFIG_AARCH32_ARMV8_R)
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
#endif

/* Read/Write Allocation Configurations for Cacheable Memory */
#define R_NON_W_NON     0x0 /* Do not allocate Read/Write */
#define R_NON_W_ALLOC   0x1 /* Do not allocate Read, Allocate Write */
#define R_ALLOC_W_NON   0x2 /* Allocate Read, Do not allocate Write */
#define R_ALLOC_W_ALLOC 0x3 /* Allocate Read/Write */

/* Memory Attributes for Normal Memory */
#define NORMAL_O_WT_NT  0x80 /* Normal, Outer Write-through non-transient */
#define NORMAL_O_WB_NT  0xC0 /* Normal, Outer Write-back non-transient */
#define NORMAL_O_NON_C  0x40 /* Normal, Outer Non-Cacheable  */

#define NORMAL_I_WT_NT  0x08 /* Normal, Inner Write-through non-transient */
#define NORMAL_I_WB_NT  0x0C /* Normal, Inner Write-back non-transient */
#define NORMAL_I_NON_C  0x04 /* Normal, Inner Non-Cacheable  */

#define NORMAL_OUTER_INNER_WRITE_THROUGH_READ_ALLOCATE_NON_TRANS \
	((NORMAL_O_WT_NT | (R_ALLOC_W_NON << 4)) \
	 | \
	 (NORMAL_I_WT_NT | R_ALLOC_W_NON)) \

#define NORMAL_OUTER_INNER_WRITE_BACK_WRITE_READ_ALLOCATE_NON_TRANS \
	((NORMAL_O_WB_NT | (R_ALLOC_W_ALLOC << 4)) \
	 | \
	 (NORMAL_I_WB_NT | R_ALLOC_W_ALLOC))

#define NORMAL_OUTER_INNER_NON_CACHEABLE \
	((NORMAL_O_NON_C | (R_NON_W_NON << 4)) \
	 | \
	 (NORMAL_I_NON_C | R_NON_W_NON))

/* Common cache-ability configuration for Flash, SRAM regions */
#define MPU_CACHE_ATTRIBUTES_FLASH \
	NORMAL_OUTER_INNER_WRITE_THROUGH_READ_ALLOCATE_NON_TRANS
#define MPU_CACHE_ATTRIBUTES_SRAM \
	NORMAL_OUTER_INNER_WRITE_BACK_WRITE_READ_ALLOCATE_NON_TRANS
#define MPU_CACHE_ATTRIBUTES_SRAM_NOCACHE \
	NORMAL_OUTER_INNER_NON_CACHEABLE

/* Global MAIR configurations */
#define MPU_MAIR_ATTR_FLASH         MPU_CACHE_ATTRIBUTES_FLASH
#define MPU_MAIR_INDEX_FLASH        0
#define MPU_MAIR_ATTR_SRAM          MPU_CACHE_ATTRIBUTES_SRAM
#define MPU_MAIR_INDEX_SRAM         1
#define MPU_MAIR_ATTR_SRAM_NOCACHE  MPU_CACHE_ATTRIBUTES_SRAM_NOCACHE
#define MPU_MAIR_INDEX_SRAM_NOCACHE 2

#if defined(CONFIG_AARCH32_ARMV8_R)
#define MPU_MAIR_ATTR_DEVICE        DEVICE_nGnRnE
#define MPU_MAIR_INDEX_DEVICE       3
/* Flash region(s): Attribute-0
 * SRAM region(s): Attribute-1
 * SRAM no cache-able regions(s): Attribute-2
 * DEVICE no cache-able regions(s): Attribute-3
 */
#define MPU_MAIR_ATTRS							     \
	((MPU_MAIR_ATTR_FLASH << (MPU_MAIR_INDEX_FLASH * 8)) |		     \
	 (MPU_MAIR_ATTR_SRAM << (MPU_MAIR_INDEX_SRAM * 8)) |		     \
	 (MPU_MAIR_ATTR_SRAM_NOCACHE << (MPU_MAIR_INDEX_SRAM_NOCACHE * 8)) | \
	 (MPU_MAIR_ATTR_DEVICE << (MPU_MAIR_INDEX_DEVICE * 8)))
#else
/* Flash region(s): Attribute-0
 * SRAM region(s): Attribute-1
 * SRAM no cache-able regions(s): Attribute-2
 */
#define MPU_MAIR_ATTRS								\
	(((MPU_MAIR_ATTR_FLASH << MPU_MAIR0_Attr0_Pos) & MPU_MAIR0_Attr0_Msk) |	\
	 ((MPU_MAIR_ATTR_SRAM << MPU_MAIR0_Attr1_Pos) & MPU_MAIR0_Attr1_Msk)  |	\
	 ((MPU_MAIR_ATTR_SRAM_NOCACHE << MPU_MAIR0_Attr2_Pos) &			\
	  MPU_MAIR0_Attr2_Msk))
#endif

/* Some helper defines for common regions.
 *
 * Note that the ARMv8-M/R MPU architecture requires that the
 * enabled MPU regions are non-overlapping. Therefore, it is
 * recommended to use these helper defines only for configuring
 * fixed MPU regions at build-time (i.e. regions that are not
 * expected to be re-programmed or re-adjusted at run-time so
 * that they do not overlap with other MPU regions).
 */
#if defined(CONFIG_AARCH32_ARMV8_R)

#define ARM_MPU_REGION_INIT(p_name, p_base, p_size, p_attr)	\
	{ .name = p_name,					\
	  .base = p_base,					\
	  .attr = p_attr(p_base + p_size),			\
	}

#define REGION_RAM_ATTR(limit)						    \
	{								    \
		.rbar = NOT_EXEC |					    \
			P_RW_U_NA_Msk | NON_SHAREABLE_Msk, /* AP, XN, SH */ \
		/* Cache-ability */					    \
		.mair_idx = MPU_MAIR_INDEX_SRAM,			    \
		.r_limit = limit - 1,  /* Region Limit */		    \
	}

#define REGION_RAM_TEXT_ATTR(limit)					    \
	{								    \
		.rbar = P_RO_U_RO_Msk | NON_SHAREABLE_Msk, /* AP, XN, SH */ \
		/* Cache-ability */					    \
		.mair_idx = MPU_MAIR_INDEX_SRAM,			    \
		.r_limit = limit - 1,  /* Region Limit */		    \
	}

#define REGION_RAM_RO_ATTR(limit)					    \
	{								    \
		.rbar = NOT_EXEC |					    \
			P_RO_U_RO_Msk | NON_SHAREABLE_Msk, /* AP, XN, SH */ \
		/* Cache-ability */					    \
		.mair_idx = MPU_MAIR_INDEX_SRAM,			    \
		.r_limit = limit - 1,  /* Region Limit */		    \
	}
#define REGION_RAM_NOCACHE_ATTR(limit)					    \
	{								    \
		.rbar = NOT_EXEC |					    \
			P_RW_U_NA_Msk | NON_SHAREABLE_Msk, /* AP, XN, SH */ \
		/* Cache-ability */					    \
		.mair_idx = MPU_MAIR_INDEX_SRAM_NOCACHE,		    \
		.r_limit = limit - 1,  /* Region Limit */		    \
	}
#if defined(CONFIG_MPU_ALLOW_FLASH_WRITE)
/* Note that the access permissions allow for un-privileged writes, contrary
 * to ARMv7-M where un-privileged code has Read-Only permissions.
 */
#define REGION_FLASH_ATTR(limit)					    \
	{								    \
		.rbar = P_RW_U_RW_Msk | NON_SHAREABLE_Msk, /* AP, XN, SH */ \
		/* Cache-ability */					    \
		.mair_idx = MPU_MAIR_INDEX_FLASH,			    \
		.r_limit = limit - 1,  /* Region Limit */		    \
	}
#else /* CONFIG_MPU_ALLOW_FLASH_WRITE */
#define REGION_FLASH_ATTR(limit)				     \
	{							     \
		.rbar = RO_Msk | NON_SHAREABLE_Msk, /* AP, XN, SH */ \
		/* Cache-ability */				     \
		.mair_idx = MPU_MAIR_INDEX_FLASH,		     \
		.r_limit = limit - 1,  /* Region Limit */	     \
	}
#endif /* CONFIG_MPU_ALLOW_FLASH_WRITE */

#define REGION_DEVICE_ATTR(limit)				      \
	{							      \
		/* AP, XN, SH */				      \
		.rbar = NOT_EXEC | P_RW_U_NA_Msk | NON_SHAREABLE_Msk, \
		/* Cache-ability */				      \
		.mair_idx = MPU_MAIR_INDEX_DEVICE,		      \
		/* Region Limit */				      \
		.r_limit = limit - 1,				      \
	}
#else

#define ARM_MPU_REGION_INIT(p_name, p_base, p_size, p_attr)	\
	{ .name = p_name,					\
	  .base = p_base,					\
	  .attr = p_attr(p_base, p_size),			\
	}

/* On Cortex-M, we can only set the XN bit when CONFIG_XIP=y. When
 * CONFIG_XIP=n, the entire image will be linked to SRAM, so we need to keep
 * the SRAM region XN bit clear or the application code will not be executable.
 */
#define REGION_RAM_ATTR(base, size) \
	{\
		.rbar = IF_ENABLED(CONFIG_XIP, (NOT_EXEC |)) \
			P_RW_U_NA_Msk | NON_SHAREABLE_Msk, /* AP, XN, SH */ \
		/* Cache-ability */ \
		.mair_idx = MPU_MAIR_INDEX_SRAM, \
		.r_limit = REGION_LIMIT_ADDR(base, size),  /* Region Limit */ \
	}

#define REGION_RAM_NOCACHE_ATTR(base, size) \
	{\
		.rbar = NOT_EXEC | \
			P_RW_U_NA_Msk | NON_SHAREABLE_Msk, /* AP, XN, SH */ \
		/* Cache-ability */ \
		.mair_idx = MPU_MAIR_INDEX_SRAM_NOCACHE, \
		.r_limit = REGION_LIMIT_ADDR(base, size),  /* Region Limit */ \
	}

#if defined(CONFIG_MPU_ALLOW_FLASH_WRITE)
/* Note that the access permissions allow for un-privileged writes, contrary
 * to ARMv7-M where un-privileged code has Read-Only permissions.
 */
#define REGION_FLASH_ATTR(base, size) \
	{\
		.rbar = P_RW_U_RW_Msk | NON_SHAREABLE_Msk, /* AP, XN, SH */ \
		/* Cache-ability */ \
		.mair_idx = MPU_MAIR_INDEX_FLASH, \
		.r_limit = REGION_LIMIT_ADDR(base, size),  /* Region Limit */ \
	}
#else /* CONFIG_MPU_ALLOW_FLASH_WRITE */
#define REGION_FLASH_ATTR(base, size) \
	{\
		.rbar = RO_Msk | NON_SHAREABLE_Msk, /* AP, XN, SH */ \
		/* Cache-ability */ \
		.mair_idx = MPU_MAIR_INDEX_FLASH, \
		.r_limit = REGION_LIMIT_ADDR(base, size),  /* Region Limit */ \
	}
#endif /* CONFIG_MPU_ALLOW_FLASH_WRITE */

#endif

struct arm_mpu_region_attr {
	/* Attributes belonging to RBAR */
	uint8_t rbar: 5;
	/* MAIR index for attribute indirection */
	uint8_t mair_idx: 3;
	/* Region Limit Address value to be written to the RLAR register. */
	uint32_t r_limit;
};

typedef struct arm_mpu_region_attr arm_mpu_region_attr_t;

/* Typedef for the k_mem_partition attribute */
typedef struct {
	uint16_t rbar;
	uint16_t mair_idx;
} k_mem_partition_attr_t;

/* Kernel macros for memory attribution
 * (access permissions and cache-ability).
 *
 * The macros are to be stored in k_mem_partition_attr_t
 * objects. The format of a k_mem_partition_attr_t object
 * is as follows: field <rbar> contains a direct mapping
 * of the <XN> and <AP> bit-fields of the RBAR register;
 * field <mair_idx> contains a direct mapping of AttrIdx
 * bit-field, stored in RLAR register.
 */

/* Read-Write access permission attributes */
#define K_MEM_PARTITION_P_RW_U_RW ((k_mem_partition_attr_t) \
	{(P_RW_U_RW_Msk | NOT_EXEC), MPU_MAIR_INDEX_SRAM})
#define K_MEM_PARTITION_P_RW_U_NA ((k_mem_partition_attr_t) \
	{(P_RW_U_NA_Msk | NOT_EXEC), MPU_MAIR_INDEX_SRAM})
#define K_MEM_PARTITION_P_RO_U_RO ((k_mem_partition_attr_t) \
	{(P_RO_U_RO_Msk | NOT_EXEC), MPU_MAIR_INDEX_SRAM})
#define K_MEM_PARTITION_P_RO_U_NA ((k_mem_partition_attr_t) \
	{(P_RO_U_NA_Msk | NOT_EXEC), MPU_MAIR_INDEX_SRAM})

/* Execution-allowed attributes */
#define K_MEM_PARTITION_P_RWX_U_RWX ((k_mem_partition_attr_t) \
	{(P_RW_U_RW_Msk), MPU_MAIR_INDEX_SRAM})
#define K_MEM_PARTITION_P_RX_U_RX ((k_mem_partition_attr_t) \
	{(P_RO_U_RO_Msk), MPU_MAIR_INDEX_SRAM})

/*
 * @brief Evaluate Write-ability
 *
 * Evaluate whether the access permissions include write-ability.
 *
 * @param attr The k_mem_partition_attr_t object holding the
 *             MPU attributes to be checked against write-ability.
 */
#define K_MEM_PARTITION_IS_WRITABLE(attr) \
	({ \
		int __is_writable__; \
		switch (attr.rbar & MPU_RBAR_AP_Msk) { \
		case P_RW_U_RW_Msk: \
		case P_RW_U_NA_Msk: \
			__is_writable__ = 1; \
			break; \
		default: \
			__is_writable__ = 0; \
		} \
		__is_writable__; \
	})

/*
 * @brief Evaluate Execution allowance
 *
 * Evaluate whether the access permissions include execution.
 *
 * @param attr The k_mem_partition_attr_t object holding the
 *             MPU attributes to be checked against execution
 *             allowance.
 */
#define K_MEM_PARTITION_IS_EXECUTABLE(attr) \
	(!((attr.rbar) & (NOT_EXEC)))

/* Attributes for no-cache enabling (share-ability is selected by default) */

/* Read-Write access permission attributes */
#define K_MEM_PARTITION_P_RW_U_RW_NOCACHE ((k_mem_partition_attr_t) \
	{(P_RW_U_RW_Msk | NOT_EXEC | OUTER_SHAREABLE_Msk), \
		MPU_MAIR_INDEX_SRAM_NOCACHE})
#define K_MEM_PARTITION_P_RW_U_NA_NOCACHE ((k_mem_partition_attr_t) \
	{(P_RW_U_NA_Msk | NOT_EXEC | OUTER_SHAREABLE_Msk), \
		MPU_MAIR_INDEX_SRAM_NOCACHE})
#define K_MEM_PARTITION_P_RO_U_RO_NOCACHE ((k_mem_partition_attr_t) \
	{(P_RO_U_RO_Msk | NOT_EXEC | OUTER_SHAREABLE_Msk), \
		MPU_MAIR_INDEX_SRAM_NOCACHE})
#define K_MEM_PARTITION_P_RO_U_NA_NOCACHE ((k_mem_partition_attr_t) \
	{(P_RO_U_NA_Msk | NOT_EXEC | OUTER_SHAREABLE_Msk), \
		MPU_MAIR_INDEX_SRAM_NOCACHE})

/* Execution-allowed attributes */
#define K_MEM_PARTITION_P_RWX_U_RWX_NOCACHE ((k_mem_partition_attr_t) \
	{(P_RW_U_RW_Msk | OUTER_SHAREABLE_Msk), MPU_MAIR_INDEX_SRAM_NOCACHE})
#define K_MEM_PARTITION_P_RX_U_RX_NOCACHE ((k_mem_partition_attr_t) \
	{(P_RO_U_RO_Msk | OUTER_SHAREABLE_Msk), MPU_MAIR_INDEX_SRAM_NOCACHE})

#endif /* _ASMLANGUAGE */

#define _ARCH_MEM_PARTITION_ALIGN_CHECK(start, size) \
	BUILD_ASSERT((size > 0) && ((uint32_t)start % \
			CONFIG_ARM_MPU_REGION_MIN_ALIGN_AND_SIZE == 0U) && \
		((size) % CONFIG_ARM_MPU_REGION_MIN_ALIGN_AND_SIZE == 0), \
		" the start and size of the partition must align " \
		"with the minimum MPU region size.")
