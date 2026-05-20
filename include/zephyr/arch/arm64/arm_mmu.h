/*
 * Copyright 2019 Broadcom
 * The term "Broadcom" refers to Broadcom Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_ARCH_ARM64_ARM_MMU_H_
#define ZEPHYR_INCLUDE_ARCH_ARM64_ARM_MMU_H_

#include <zephyr/dt-bindings/memory-attr/memory-attr-arm64.h>

#ifndef _ASMLANGUAGE
#include <stdint.h>
#include <stdlib.h>
#endif

/* Following Memory types supported through MAIR encodings can be passed
 * by user through "attrs"(attributes) field of specified memory region.
 * As MAIR supports such 8 encodings, we will reserve attrs[2:0];
 * so that we can provide encodings upto 7 if needed in future.
 */
#define MT_TYPE_MASK		0x7U
#define MT_TYPE(attr)		(attr & MT_TYPE_MASK)
#define MT_DEVICE_nGnRnE	0U
#define MT_DEVICE_nGnRE		1U
#define MT_DEVICE_GRE		2U
#define MT_NORMAL_NC		3U
#define MT_NORMAL		4U
#define MT_NORMAL_WT		5U

#define MEMORY_ATTRIBUTES	((0x00 << (MT_DEVICE_nGnRnE * 8)) |	\
				(0x04 << (MT_DEVICE_nGnRE * 8))   |	\
				(0x0c << (MT_DEVICE_GRE * 8))     |	\
				(0x44 << (MT_NORMAL_NC * 8))      |	\
				(0xffUL << (MT_NORMAL * 8))	  |	\
				(0xbbUL << (MT_NORMAL_WT * 8)))

/* More flags from user's perspective are supported using remaining bits
 * of "attrs" field, i.e. attrs[31:3], underlying code will take care
 * of setting PTE fields correctly.
 *
 * current usage of attrs[31:3] is:
 * attrs[3] : Access Permissions
 * attrs[4] : Memory access from secure/ns state
 * attrs[5] : Execute Permissions privileged mode (PXN)
 * attrs[6] : Execute Permissions unprivileged mode (UXN)
 * attrs[7] : Mirror RO/RW permissions to EL0
 * attrs[8] : Overwrite existing mapping if any
 * attrs[9] : non-Global mapping (nG)
 * attrs[10]: Paged-out mapping
 *
 */
#define MT_PERM_SHIFT		3U
#define MT_SEC_SHIFT		4U
#define MT_P_EXECUTE_SHIFT	5U
#define MT_U_EXECUTE_SHIFT	6U
#define MT_RW_AP_SHIFT		7U
#define MT_NO_OVERWRITE_SHIFT	8U
#define MT_NON_GLOBAL_SHIFT	9U
#define MT_PAGED_OUT_SHIFT	10U

#define MT_RO			(0U << MT_PERM_SHIFT)
#define MT_RW			(1U << MT_PERM_SHIFT)

#define MT_RW_AP_ELx		(1U << MT_RW_AP_SHIFT)
#define MT_RW_AP_EL_HIGHER	(0U << MT_RW_AP_SHIFT)

#define MT_SECURE		(0U << MT_SEC_SHIFT)
#define MT_NS			(1U << MT_SEC_SHIFT)

#define MT_P_EXECUTE		(0U << MT_P_EXECUTE_SHIFT)
#define MT_P_EXECUTE_NEVER	(1U << MT_P_EXECUTE_SHIFT)

#define MT_U_EXECUTE		(0U << MT_U_EXECUTE_SHIFT)
#define MT_U_EXECUTE_NEVER	(1U << MT_U_EXECUTE_SHIFT)

#define MT_NO_OVERWRITE		(1U << MT_NO_OVERWRITE_SHIFT)

#define MT_G			(0U << MT_NON_GLOBAL_SHIFT)
#define MT_NG			(1U << MT_NON_GLOBAL_SHIFT)

#define MT_PAGED_OUT		(1U << MT_PAGED_OUT_SHIFT)

#define MT_P_RW_U_RW		(MT_RW | MT_RW_AP_ELx | MT_P_EXECUTE_NEVER | MT_U_EXECUTE_NEVER)
#define MT_P_RW_U_NA		(MT_RW | MT_RW_AP_EL_HIGHER  | MT_P_EXECUTE_NEVER | MT_U_EXECUTE_NEVER)
#define MT_P_RO_U_RO		(MT_RO | MT_RW_AP_ELx | MT_P_EXECUTE_NEVER | MT_U_EXECUTE_NEVER)
#define MT_P_RO_U_NA		(MT_RO | MT_RW_AP_EL_HIGHER  | MT_P_EXECUTE_NEVER | MT_U_EXECUTE_NEVER)
#define MT_P_RO_U_RX		(MT_RO | MT_RW_AP_ELx | MT_P_EXECUTE_NEVER | MT_U_EXECUTE)
#define MT_P_RX_U_RX		(MT_RO | MT_RW_AP_ELx | MT_P_EXECUTE | MT_U_EXECUTE)
#define MT_P_RX_U_NA		(MT_RO | MT_RW_AP_EL_HIGHER  | MT_P_EXECUTE | MT_U_EXECUTE_NEVER)

#ifdef CONFIG_ARMV8_A_NS
#define MT_DEFAULT_SECURE_STATE	MT_NS
#else
#define MT_DEFAULT_SECURE_STATE	MT_SECURE
#endif

/* Definitions used by arch_page_info_get() */
#define ARCH_DATA_PAGE_LOADED		BIT(0)
#define ARCH_DATA_PAGE_ACCESSED		BIT(1)
#define ARCH_DATA_PAGE_DIRTY		BIT(2)
#define ARCH_DATA_PAGE_NOT_MAPPED	BIT(3)

/*
 * Special unpaged "location" tags (highest possible descriptor physical
 * address values unlikely to conflict with backing store locations)
 */
#define ARCH_UNPAGED_ANON_ZERO		0x0000fffffffff000
#define ARCH_UNPAGED_ANON_UNINIT	0x0000ffffffffe000

#ifndef _ASMLANGUAGE

/* Region definition data structure */
struct arm_mmu_region {
	/* Region Base Physical Address */
	uintptr_t base_pa;
	/* Region Base Virtual Address */
	uintptr_t base_va;
	/* Region size */
	size_t size;
	/* Region Name */
	const char *name;
	/* Region Attributes */
	uint32_t attrs;
};

/* MMU configuration data structure */
struct arm_mmu_config {
	/* Number of regions */
	unsigned int num_regions;
	/* Regions */
	const struct arm_mmu_region *mmu_regions;
};

struct arm_mmu_ptables {
	uint64_t *base_xlat_table;
	uint64_t ttbr0;
};

/* Convenience macros to represent the ARMv8-A-specific
 * configuration for memory access permission and
 * cache-ability attribution.
 */

#define MMU_REGION_ENTRY(_name, _base_pa, _base_va, _size, _attrs) \
	{\
		.name = _name, \
		.base_pa = _base_pa, \
		.base_va = _base_va, \
		.size = _size, \
		.attrs = _attrs, \
	}

#define MMU_REGION_FLAT_ENTRY(name, adr, sz, attrs) \
	MMU_REGION_ENTRY(name, adr, adr, sz, attrs)

/*
 * @brief Auto generate mmu region entry for node_id
 *
 * Example usage:
 *
 * @code{.c}
 *      DT_FOREACH_STATUS_OKAY_VARGS(nxp_imx_gpio,
 *				  MMU_REGION_DT_FLAT_ENTRY,
 *				 (MT_DEVICE_nGnRnE | MT_P_RW_U_NA | MT_NS))
 * @endcode
 *
 * @note  Since devicetree_generated.h does not include
 *        node_id##_P_reg_FOREACH_PROP_ELEM* definitions,
 *        we can't automate dts node with multiple reg
 *        entries.
 */
#define MMU_REGION_DT_FLAT_ENTRY(node_id, attrs)  \
	MMU_REGION_FLAT_ENTRY(DT_NODE_FULL_NAME(node_id), \
				  DT_REG_ADDR(node_id), \
				  DT_REG_SIZE(node_id), \
				  attrs),

/*
 * @brief Auto generate mmu region entry for status = "okay"
 *        nodes compatible to a driver
 *
 * Example usage:
 *
 * @code{.c}
 *      MMU_REGION_DT_COMPAT_FOREACH_FLAT_ENTRY(nxp_imx_gpio,
 *				 (MT_DEVICE_nGnRnE | MT_P_RW_U_NA | MT_NS))
 * @endcode
 *
 * @note  This is a wrapper of @ref MMU_REGION_DT_FLAT_ENTRY
 */
#define MMU_REGION_DT_COMPAT_FOREACH_FLAT_ENTRY(compat, attr) \
	DT_FOREACH_STATUS_OKAY_VARGS(compat, \
	MMU_REGION_DT_FLAT_ENTRY, attr)

/**
 * @brief Extract ARM64 architecture bits from a DT memory attribute value.
 *
 * @param dt_attr The DT memory attribute value.
 */
#define _DT_MEM_ARM64_ARCH_BITS(dt_attr) \
	(DT_MEM_ARCH_ATTR_GET(dt_attr) >> DT_MEM_ARCH_ATTR_SHIFT)

/**
 * @brief Convert a DT zephyr,memory-attr value to ARM64 MT_* flags.
 *
 * The generic DT_MEM_CACHEABLE bit selects cacheable vs non-cacheable.
 * The arch-specific ATTR_ARM64_CACHE_WB bit selects write-back vs
 * write-through when cacheable.  Device memory must be mapped through
 * the MMIO device API instead.
 *
 * @param dt_attr The DT memory attribute value from zephyr,memory-attr.
 */
#define DT_MEM_ATTR_TO_MT(dt_attr)						\
	((DT_MEM_ATTR_GET(dt_attr) & DT_MEM_CACHEABLE) ?			\
		((_DT_MEM_ARM64_ARCH_BITS(dt_attr) & ATTR_ARM64_CACHE_WB) ?	\
			(MT_NORMAL | MT_P_RW_U_NA | MT_DEFAULT_SECURE_STATE) :	\
			(MT_NORMAL_WT | MT_P_RW_U_NA | MT_DEFAULT_SECURE_STATE)) : \
		(MT_NORMAL_NC | MT_P_RW_U_NA | MT_DEFAULT_SECURE_STATE))

/**
 * @brief Auto-generate MMU region entry from a DT node's zephyr,memory-attr.
 *
 * Unlike MMU_REGION_DT_FLAT_ENTRY which takes attrs as a parameter,
 * this macro reads the memory type from the node's zephyr,memory-attr
 * property and converts it to ARM64 MT_* flags at compile time.
 *
 * Nodes without zephyr,memory-attr are silently skipped.
 *
 * @param node_id Devicetree node identifier.
 *
 * @note This is a wrapper of @ref MMU_REGION_FLAT_ENTRY and inherits
 *       its limitation: only the first reg bank is used. DT nodes
 *       with multiple reg entries are not fully covered.
 */
#define MMU_REGION_DT_FLAT_ENTRY_FROM_DT(node_id)			\
	IF_ENABLED(DT_NODE_HAS_PROP(node_id, zephyr_memory_attr),	\
		(MMU_REGION_FLAT_ENTRY(DT_NODE_FULL_NAME(node_id),	\
			DT_REG_ADDR(node_id),				\
			DT_REG_SIZE(node_id),				\
			DT_MEM_ATTR_TO_MT(				\
				DT_PROP(node_id, zephyr_memory_attr))),))

/**
 * @brief Mask of all zephyr,memory-attr bits supported by the ARM64 MMU.
 *
 * Only DT_MEM_CACHEABLE and ATTR_ARM64_CACHE_WB are supported.
 */
#define _DT_MEM_ARM64_SUPPORTED_MASK \
	(DT_MEM_CACHEABLE | DT_MEM_ARM64(ATTR_ARM64_CACHE_WB))

/**
 * @brief Check whether a DT memory attribute value is valid for the ARM64 MMU.
 *
 * @param dt_attr The DT memory attribute value.
 * @return true if only supported bits are set.
 */
#define DT_MEM_ARM64_MMU_IS_VALID(dt_attr) \
	(((dt_attr) & ~_DT_MEM_ARM64_SUPPORTED_MASK) == 0)

/**
 * @brief BUILD_ASSERT that a DT node's zephyr,memory-attr is valid for ARM64.
 *
 * Nodes without the property are silently skipped.
 *
 * @param node_id Devicetree node identifier.
 */
#define ARM64_MMU_VALIDATE_DT_REGION(node_id) \
	IF_ENABLED(DT_NODE_HAS_PROP(node_id, zephyr_memory_attr), \
		(BUILD_ASSERT(DT_MEM_ARM64_MMU_IS_VALID( \
			DT_PROP(node_id, zephyr_memory_attr)), \
			"Unsupported zephyr,memory-attr for ARM64 MMU region " \
			DT_NODE_FULL_NAME(node_id));))

/**
 * @brief Auto-generate MMU region entries for all matching nodes.
 *
 * @param compat Devicetree compatible to iterate over.
 *
 * Iterates over all status = "okay" nodes of @p compat and generates
 * an MMU region entry for each node that has a zephyr,memory-attr property.
 *
 * @note This is a wrapper of @ref MMU_REGION_DT_FLAT_ENTRY_FROM_DT.
 */
#define MMU_REGION_DT_COMPAT_FOREACH_FLAT_ENTRY_FROM_DT(compat) \
	DT_FOREACH_STATUS_OKAY(compat, MMU_REGION_DT_FLAT_ENTRY_FROM_DT)

/* Kernel macros for memory attribution
 * (access permissions and cache-ability).
 *
 * The macros are to be stored in k_mem_partition_attr_t
 * objects. The format of a k_mem_partition_attr_t object
 * is an uint32_t composed by permission and attribute flags
 * located in include/arch/arm64/arm_mmu.h
 */

/* Read-Write access permission attributes */
#define K_MEM_PARTITION_P_RW_U_RW ((k_mem_partition_attr_t) \
			{MT_P_RW_U_RW})
#define K_MEM_PARTITION_P_RW_U_NA ((k_mem_partition_attr_t) \
			{MT_P_RW_U_NA})
#define K_MEM_PARTITION_P_RO_U_RO ((k_mem_partition_attr_t) \
			{MT_P_RO_U_RO})
#define K_MEM_PARTITION_P_RO_U_NA ((k_mem_partition_attr_t) \
			{MT_P_RO_U_NA})
/* Execution-allowed attributes */
#define K_MEM_PARTITION_P_RX_U_RX ((k_mem_partition_attr_t) \
			{MT_P_RX_U_RX})
/* Typedef for the k_mem_partition attribute */
typedef struct { uint32_t attrs; } k_mem_partition_attr_t;

/* Reference to the MMU configuration.
 *
 * This struct is defined and populated for each SoC (in the SoC definition),
 * and holds the build-time configuration information for the fixed MMU
 * regions enabled during kernel initialization.
 */
extern const struct arm_mmu_config mmu_config;

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_ARM64_ARM_MMU_H_ */
